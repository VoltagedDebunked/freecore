/*
 * FreeCore - A free operating system kernel
 * Copyright (C) 2025 FreeCore Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <lib/minstd.h>
#include <kernel/io.h>
#include <fs/ext4/ext4.h>
#include <drivers/driversys.h>
#include <fs/vfs.h>
#include <drivers/block/block.h>
#include <mm/kmalloc.h>

/* Forward declarations */
static int ext4_open(struct vfs_node *node, int flags);
static int ext4_close(struct vfs_node *node);
static size_t ext4_read(struct vfs_node *node, uint64_t offset, size_t size, void *buffer);
static size_t ext4_write(struct vfs_node *node, uint64_t offset, size_t size, const void *buffer);
static int ext4_readdir(struct vfs_node *node, uint32_t index, struct vfs_dirent *dirent);
static struct vfs_node* ext4_finddir(struct vfs_node *node, const char *name);
static int ext4_stat(struct vfs_node *node, struct vfs_stat *stat);

/* Filesystem operations structure */
static vfs_node_ops_t ext4_ops = {
    .open = ext4_open,
    .close = ext4_close,
    .read = ext4_read,
    .write = ext4_write,
    .readdir = ext4_readdir,
    .finddir = ext4_finddir,
    .stat = ext4_stat
};

/* Driver hooks */
static int ext4_probe_driver(device_driver_t *driver);
static int ext4_remove_driver(device_driver_t *driver);

/* Define the EXT4 driver */
static driver_ops_t ext4_driver_ops = {
    .probe = ext4_probe_driver,
    .remove = ext4_remove_driver,
    .suspend = NULL,
    .resume = NULL
};

static device_driver_t ext4_driver = {
    .name = "ext4_fs",
    .device_class = DEVICE_CLASS_STORAGE,
    .state = DRIVER_STATE_UNLOADED,
    .ops = &ext4_driver_ops,
    .private_data = NULL
};

/**
 * Read a block from the filesystem
 * @param fs The filesystem to read from
 * @param block_num The block number to read
 * @param buffer The buffer to read into
 * @return 0 on success, negative on error
 */
int ext4_read_block(ext4_fs_t *fs, uint64_t block_num, void *buffer) {
    if (!fs || !buffer || !fs->device) {
        return -1;
    }

    uint64_t offset = block_num * fs->block_size;
    size_t size = fs->block_size;

    /* Read the block using the block device interface */
    return fs->device->ops->read(fs->device, offset, size, buffer);
}

/**
 * Read an inode from the filesystem
 * @param fs The filesystem to read from
 * @param inode_num The inode number to read
 * @param inode The buffer to read the inode into
 * @return 0 on success, negative on error
 */
int ext4_read_inode(ext4_fs_t *fs, uint32_t inode_num, ext4_inode_t *inode) {
    if (!fs || !inode || inode_num < 1) {
        return -1;
    }

    /* Calculate which block group the inode belongs to */
    uint32_t inodes_per_group = fs->inodes_per_group;
    uint32_t group = (inode_num - 1) / inodes_per_group;

    /* If the group is beyond the number of groups, it's an invalid inode */
    if (group >= fs->groups_count) {
        kerr("EXT4: Invalid inode number %u (group %u >= groups_count %u)\n",
            inode_num, group, fs->groups_count);
        return -1;
    }

    /* Get the group descriptor for this inode */
    ext4_group_desc_t *gdesc = &fs->group_desc_table[group];

    /* Calculate inode table location and index */
    uint64_t inode_table_block = gdesc->bg_inode_table_lo |
                               ((uint64_t)gdesc->bg_inode_table_hi << 32);
    uint32_t index = (inode_num - 1) % inodes_per_group;

    /* Calculate the block containing this inode */
    uint64_t inode_block = inode_table_block +
                           (index * fs->sb.s_inode_size) / fs->block_size;

    /* Read the block containing the inode */
    uint8_t *block_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buffer) {
        kerr("EXT4: Failed to allocate memory for inode block\n");
        return -1;
    }

    int result = ext4_read_block(fs, inode_block, block_buffer);
    if (result < 0) {
        kerr("EXT4: Failed to read inode block %llu\n", inode_block);
        kfree(block_buffer);
        return result;
    }

    /* Calculate the offset of the inode within the block */
    uint32_t offset = (index * fs->sb.s_inode_size) % fs->block_size;

    /* Copy the inode data */
    memcpy(inode, block_buffer + offset, fs->sb.s_inode_size);

    kfree(block_buffer);
    return 0;
}

/**
 * Read file data using extent-based addressing
 * @param fs The filesystem
 * @param inode The inode to read from
 * @param block_num The logical block number to read
 * @param phys_block Output parameter for the physical block number
 * @return 0 on success, negative on error
 */
int ext4_read_extent_block(ext4_fs_t *fs, ext4_inode_t *inode, uint64_t block_num, uint64_t *phys_block) {
    /* Make sure we're using extent-based addressing */
    if (!(fs->sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS)) {
        kerr("EXT4: This filesystem doesn't use extents\n");
        return -1;
    }

    /* Check if the inode is using extents (flag in i_flags) */
    if (!(inode->i_flags & 0x80000)) { /* EXT4_EXTENTS_FL */
        kerr("EXT4: Inode is not using extents\n");
        return -1;
    }

    /* Treat i_block as an extent header */
    ext4_extent_node_t *node = (ext4_extent_node_t *)&inode->i_block;

    /* Verify the extent header's magic number */
    if (node->header.eh_magic != EXT4_EXTENT_HEADER_MAGIC) {
        kerr("EXT4: Invalid extent header magic: 0x%x\n", node->header.eh_magic);
        return -1;
    }

    /* Create a buffer for reading extent nodes */
    ext4_extent_node_t *extent_buffer = (ext4_extent_node_t *)kmalloc(fs->block_size);
    if (!extent_buffer) {
        kerr("EXT4: Failed to allocate memory for extent buffer\n");
        return -1;
    }

    /* Copy the root node to the buffer */
    memcpy(extent_buffer, node, sizeof(ext4_extent_node_t));

    /* Traverse the extent tree to find the block */
    int depth = extent_buffer->header.eh_depth;
    while (depth > 0) {
        /* In an internal node, find the index that contains our block */
        int i;
        for (i = 0; i < extent_buffer->header.eh_entries; i++) {
            if (i < extent_buffer->header.eh_entries - 1 &&
                block_num >= extent_buffer->index[i + 1].ei_block) {
                continue;
            }
            break;
        }

        if (i >= extent_buffer->header.eh_entries) {
            kerr("EXT4: Block number %llu not found in extent tree\n", block_num);
            kfree(extent_buffer);
            return -1;
        }

        /* Read the next node */
        uint64_t node_block = extent_buffer->index[i].ei_leaf_lo |
                            ((uint64_t)extent_buffer->index[i].ei_leaf_hi << 32);

        int result = ext4_read_block(fs, node_block, extent_buffer);
        if (result < 0) {
            kerr("EXT4: Failed to read extent node block %llu\n", node_block);
            kfree(extent_buffer);
            return result;
        }

        /* Verify the extent header's magic number */
        if (extent_buffer->header.eh_magic != EXT4_EXTENT_HEADER_MAGIC) {
            kerr("EXT4: Invalid extent header magic in node: 0x%x\n", extent_buffer->header.eh_magic);
            kfree(extent_buffer);
            return -1;
        }

        depth--;
    }

    /* Now we're at a leaf node, find the extent that contains our block */
    int i;
    for (i = 0; i < extent_buffer->header.eh_entries; i++) {
        uint32_t ee_block = extent_buffer->extent[i].ee_block;
        uint16_t ee_len = extent_buffer->extent[i].ee_len;

        if (block_num >= ee_block && block_num < ee_block + ee_len) {
            /* Found the extent containing our block */
            uint64_t extent_start = extent_buffer->extent[i].ee_start_lo |
                                 ((uint64_t)extent_buffer->extent[i].ee_start_hi << 32);

            /* Calculate the physical block number */
            *phys_block = extent_start + (block_num - ee_block);
            kfree(extent_buffer);
            return 0;
        }
    }

    kerr("EXT4: Block number %llu not found in extent leaf\n", block_num);
    kfree(extent_buffer);
    return -1;
}

/**
 * Read a file block using either extent or indirect addressing
 * @param fs The filesystem
 * @param inode The inode to read from
 * @param block_num The logical block number to read
 * @param buffer The buffer to read into
 * @return 0 on success, negative on error
 */
int ext4_read_file_block(ext4_fs_t *fs, ext4_inode_t *inode, uint64_t block_num, void *buffer) {
    uint64_t phys_block = 0;
    int result;

    /* Check if block number is within file size */
    uint64_t file_size = inode->i_size_lo | ((uint64_t)inode->i_size_high << 32);
    uint64_t max_block = (file_size + fs->block_size - 1) / fs->block_size;

    if (block_num >= max_block) {
        /* Reading past end of file */
        memset(buffer, 0, fs->block_size);
        return 0;
    }

    /* Determine how to read the block based on filesystem features */
    if (fs->sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS) {
        /* Use extent-based addressing */
        result = ext4_read_extent_block(fs, inode, block_num, &phys_block);
        if (result < 0) {
            return result;
        }
    } else {
        /* Use traditional indirect block addressing (not implemented in this sample) */
        kerr("EXT4: Traditional indirect block addressing not implemented\n");
        return -1;
    }

    /* Read the physical block */
    return ext4_read_block(fs, phys_block, buffer);
}

/**
 * Read file data from an inode
 * @param fs The filesystem
 * @param inode The inode to read from
 * @param offset The offset within the file to read from
 * @param size The number of bytes to read
 * @param buffer The buffer to read into
 * @return Number of bytes read, or negative on error
 */
int ext4_read_file_data(ext4_fs_t *fs, ext4_inode_t *inode, uint64_t offset, uint64_t size, void *buffer) {
    if (!fs || !inode || !buffer) {
        return -1;
    }

    /* Check if offset is beyond file size */
    uint64_t file_size = inode->i_size_lo | ((uint64_t)inode->i_size_high << 32);
    if (offset >= file_size) {
        return 0; /* EOF */
    }

    /* Adjust size if it would go past end of file */
    if (offset + size > file_size) {
        size = file_size - offset;
    }

    /* Allocate a temporary buffer for reading blocks */
    uint8_t *block_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!block_buffer) {
        kerr("EXT4: Failed to allocate memory for block buffer\n");
        return -1;
    }

    /* Calculate starting block and offset within block */
    uint64_t start_block = offset / fs->block_size;
    uint32_t start_offset = offset % fs->block_size;

    /* Calculate total number of blocks to read */
    uint64_t end_pos = offset + size;
    uint64_t end_block = (end_pos - 1) / fs->block_size;
    uint64_t num_blocks = end_block - start_block + 1;

    /* Read blocks one by one */
    uint64_t bytes_read = 0;
    uint8_t *dest = (uint8_t *)buffer;

    for (uint64_t i = 0; i < num_blocks; i++) {
        uint64_t current_block = start_block + i;
        int result = ext4_read_file_block(fs, inode, current_block, block_buffer);
        if (result < 0) {
            kerr("EXT4: Failed to read file block %llu\n", current_block);
            kfree(block_buffer);
            return result;
        }

        /* Calculate how much data to copy from this block */
        uint32_t block_offset = (i == 0) ? start_offset : 0;
        uint32_t bytes_to_copy = fs->block_size - block_offset;

        if (bytes_read + bytes_to_copy > size) {
            bytes_to_copy = size - bytes_read;
        }

        /* Copy data from block buffer to destination buffer */
        memcpy(dest + bytes_read, block_buffer + block_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;

        if (bytes_read >= size) {
            break;
        }
    }

    kfree(block_buffer);
    return bytes_read;
}

/**
 * Create a VFS node from an ext4 inode
 * @param fs The filesystem
 * @param inode_num The inode number
 * @param node The VFS node to fill
 * @return 0 on success, negative on error
 */
static int ext4_create_vfs_node(ext4_fs_t *fs, uint32_t inode_num, struct vfs_node *node) {
    ext4_inode_t inode;
    int result = ext4_read_inode(fs, inode_num, &inode);
    if (result < 0) {
        return result;
    }

    /* Allocate inode info */
    ext4_inode_info_t *info = (ext4_inode_info_t *)kmalloc(sizeof(ext4_inode_info_t));
    if (!info) {
        return -1;
    }

    /* Copy inode data */
    memcpy(&info->raw_inode, &inode, sizeof(ext4_inode_t));
    info->inode_num = inode_num;
    info->fs = fs;

    /* Set up the VFS node */
    memset(node, 0, sizeof(struct vfs_node));
    node->inode = inode_num;
    node->size = inode.i_size_lo | ((uint64_t)inode.i_size_high << 32);
    node->private_data = info;
    node->ops = &ext4_ops;

    /* Set the node type based on the inode mode */
    if (S_ISDIR(inode.i_mode)) {
        node->type = VFS_DIRECTORY;
    } else if (S_ISREG(inode.i_mode)) {
        node->type = VFS_FILE;
    } else if (S_ISLNK(inode.i_mode)) {
        node->type = VFS_SYMLINK;
    } else if (S_ISCHR(inode.i_mode)) {
        node->type = VFS_CHARDEVICE;
    } else if (S_ISBLK(inode.i_mode)) {
        node->type = VFS_BLOCKDEVICE;
    } else if (S_ISFIFO(inode.i_mode)) {
        node->type = VFS_PIPE;
    } else if (S_ISSOCK(inode.i_mode)) {
        node->type = VFS_SOCKET;
    } else {
        node->type = VFS_FILE;
    }

    return 0;
}

/**
 * Mount an ext4 filesystem
 * @param device The block device containing the filesystem
 * @param root_node Pointer to store the root node
 * @return 0 on success, negative on error
 */
int ext4_mount(block_device_t *device, struct vfs_node **root_node) {
    if (!device || !root_node) {
        return -1;
    }

    kprintf("EXT4: Mounting filesystem on device %s\n", device->name);

    /* Allocate filesystem structure */
    ext4_fs_t *fs = (ext4_fs_t *)kmalloc(sizeof(ext4_fs_t));
    if (!fs) {
        kerr("EXT4: Failed to allocate memory for filesystem\n");
        return -1;
    }

    /* Initialize the filesystem structure */
    memset(fs, 0, sizeof(ext4_fs_t));
    fs->device = device;

    /* Read the superblock */
    uint8_t *sb_buffer = (uint8_t *)kmalloc(sizeof(ext4_superblock_t));
    if (!sb_buffer) {
        kerr("EXT4: Failed to allocate memory for superblock\n");
        kfree(fs);
        return -1;
    }

    int result = device->ops->read(device, EXT4_SUPERBLOCK_OFFSET, sizeof(ext4_superblock_t), sb_buffer);
    if (result < 0) {
        kerr("EXT4: Failed to read superblock\n");
        kfree(sb_buffer);
        kfree(fs);
        return result;
    }

    /* Copy superblock data */
    memcpy(&fs->sb, sb_buffer, sizeof(ext4_superblock_t));
    kfree(sb_buffer);

    /* Verify the superblock magic */
    if (fs->sb.s_magic != EXT4_SUPER_MAGIC) {
        kerr("EXT4: Invalid superblock magic: 0x%x\n", fs->sb.s_magic);
        kfree(fs);
        return -1;
    }

    /* Calculate block size and other parameters */
    fs->block_size = 1024 << fs->sb.s_log_block_size;
    fs->block_count = fs->sb.s_blocks_count_lo | ((uint64_t)fs->sb.s_blocks_count_hi << 32);
    fs->inodes_per_group = fs->sb.s_inodes_per_group;
    fs->blocks_per_group = fs->sb.s_blocks_per_group;

    /* Calculate number of block groups */
    fs->groups_count = (fs->block_count + fs->blocks_per_group - 1) / fs->blocks_per_group;

    kprintf("EXT4: Filesystem info:\n");
    kprintf("      Block size: %u bytes\n", fs->block_size);
    kprintf("      Block count: %llu\n", fs->block_count);
    kprintf("      Inodes per group: %u\n", fs->inodes_per_group);
    kprintf("      Block groups: %u\n", fs->groups_count);

    /* Calculate size of group descriptor table */
    uint32_t gdesc_size = fs->sb.s_desc_size;
    if (gdesc_size == 0) {
        /* Default size for older ext4 filesystems */
        gdesc_size = 32;
    }

    uint32_t gdesc_table_size = fs->groups_count * gdesc_size;
    uint32_t gdesc_blocks = (gdesc_table_size + fs->block_size - 1) / fs->block_size;

    /* Allocate memory for the group descriptor table */
    fs->group_desc_table = (ext4_group_desc_t *)kmalloc(gdesc_blocks * fs->block_size);
    if (!fs->group_desc_table) {
        kerr("EXT4: Failed to allocate memory for group descriptor table\n");
        kfree(fs);
        return -1;
    }

    /* Read the group descriptor table */
    uint64_t gdesc_start_block = fs->sb.s_first_data_block + 1; /* Superblock is at block 0 or 1 */

    for (uint32_t i = 0; i < gdesc_blocks; i++) {
        result = ext4_read_block(fs, gdesc_start_block + i,
                               ((uint8_t *)fs->group_desc_table) + (i * fs->block_size));
        if (result < 0) {
            kerr("EXT4: Failed to read group descriptor block %u\n", gdesc_start_block + i);
            kfree(fs->group_desc_table);
            kfree(fs);
            return result;
        }
    }

    /* Create the root node */
    *root_node = (struct vfs_node *)kmalloc(sizeof(struct vfs_node));
    if (!*root_node) {
        kerr("EXT4: Failed to allocate memory for root node\n");
        kfree(fs->group_desc_table);
        kfree(fs);
        return -1;
    }

    result = ext4_create_vfs_node(fs, EXT4_ROOT_INO, *root_node);
    if (result < 0) {
        kerr("EXT4: Failed to create root node\n");
        kfree(*root_node);
        kfree(fs->group_desc_table);
        kfree(fs);
        return result;
    }

    /* Store the root node in the filesystem structure */
    fs->root_node = *root_node;

    kprintf("EXT4: Filesystem mounted successfully\n");
    return 0;
}

/**
 * Unmount an ext4 filesystem
 * @param root_node The root node of the filesystem
 */
void ext4_unmount(struct vfs_node *root_node) {
    if (!root_node || !root_node->private_data) {
        return;
    }

    ext4_inode_info_t *info = (ext4_inode_info_t *)root_node->private_data;
    ext4_fs_t *fs = info->fs;

    if (fs) {
        kprintf("EXT4: Unmounting filesystem\n");

        /* Free filesystem resources */
        if (fs->group_desc_table) {
            kfree(fs->group_desc_table);
        }

        kfree(fs);
    }

    /* Free the inode info */
    kfree(info);

    /* Clear the root node */
    root_node->private_data = NULL;
}

/**
 * Find a directory entry by name
 * @param fs The filesystem
 * @param dir_inode The directory inode
 * @param name The name to find
 * @param inode_num Pointer to store the found inode number
 * @return 0 on success, negative on error
 */
static int ext4_find_dir_entry(ext4_fs_t *fs, ext4_inode_t *dir_inode, const char *name, uint32_t *inode_num) {
    if (!fs || !dir_inode || !name || !inode_num) {
        return -1;
    }

    /* Allocate buffer for reading directory blocks */
    uint8_t *dir_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!dir_buffer) {
        kerr("EXT4: Failed to allocate memory for directory buffer\n");
        return -1;
    }

    /* Calculate directory size */
    uint64_t dir_size = dir_inode->i_size_lo | ((uint64_t)dir_inode->i_size_high << 32);
    uint64_t num_blocks = (dir_size + fs->block_size - 1) / fs->block_size;

    /* Iterate through all blocks in the directory */
    for (uint64_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        int result = ext4_read_file_block(fs, dir_inode, block_idx, dir_buffer);
        if (result < 0) {
            kerr("EXT4: Failed to read directory block %llu\n", block_idx);
            kfree(dir_buffer);
            return result;
        }

        /* Scan the directory entries in this block */
        uint32_t offset = 0;
        while (offset < fs->block_size) {
            ext4_dir_entry_2_t *entry = (ext4_dir_entry_2_t *)(dir_buffer + offset);

            /* Check if we've reached the end of the entries */
            if (entry->rec_len == 0) {
                break;
            }

            /* Check if this is a valid entry */
            if (entry->inode != 0) {
                /* Compare the name, accounting for name_len */
                if (entry->name_len == strlen(name) &&
                    strncmp(entry->name, name, entry->name_len) == 0) {
                    /* Found it! */
                    *inode_num = entry->inode;
                    kfree(dir_buffer);
                    return 0;
                }
            }

            /* Move to the next entry */
            offset += entry->rec_len;
        }
    }

    /* Entry not found */
    kfree(dir_buffer);
    return -1;
}

/**
 * VFS open function
 */
static int ext4_open(struct vfs_node *node, int flags) {
    /* Nothing special to do here for now */
    return 0;
}

/**
 * VFS close function
 */
static int ext4_close(struct vfs_node *node) {
    /* Nothing special to do here for now */
    return 0;
}

/**
 * VFS read function
 */
static size_t ext4_read(struct vfs_node *node, uint64_t offset, size_t size, void *buffer) {
    if (!node || !buffer || !node->private_data) {
        return 0;
    }

    ext4_inode_info_t *info = (ext4_inode_info_t *)node->private_data;
    ext4_fs_t *fs = info->fs;

    return ext4_read_file_data(fs, &info->raw_inode, offset, size, buffer);
}

/**
 * VFS write function
 */
static size_t ext4_write(struct vfs_node *node, uint64_t offset, size_t size, const void *buffer) {
    /* Read-only implementation for now */
    return 0;
}

/**
 * VFS readdir function
 */
static int ext4_readdir(struct vfs_node *node, uint32_t index, struct vfs_dirent *dirent) {
    if (!node || !dirent || !node->private_data || node->type != VFS_DIRECTORY) {
        return -1;
    }

    ext4_inode_info_t *info = (ext4_inode_info_t *)node->private_data;
    ext4_fs_t *fs = info->fs;
    ext4_inode_t *dir_inode = &info->raw_inode;

    /* Allocate buffer for reading directory blocks */
    uint8_t *dir_buffer = (uint8_t *)kmalloc(fs->block_size);
    if (!dir_buffer) {
        kerr("EXT4: Failed to allocate memory for directory buffer\n");
        return -1;
    }

    /* Calculate directory size */
    uint64_t dir_size = dir_inode->i_size_lo | ((uint64_t)dir_inode->i_size_high << 32);
    uint64_t num_blocks = (dir_size + fs->block_size - 1) / fs->block_size;

    uint32_t entry_idx = 0;

    /* Iterate through all blocks in the directory */
    for (uint64_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        int result = ext4_read_file_block(fs, dir_inode, block_idx, dir_buffer);
        if (result < 0) {
            kerr("EXT4: Failed to read directory block %llu\n", block_idx);
            kfree(dir_buffer);
            return result;
        }

        /* Scan the directory entries in this block */
        uint32_t offset = 0;
        while (offset < fs->block_size) {
            ext4_dir_entry_2_t *entry = (ext4_dir_entry_2_t *)(dir_buffer + offset);

            /* Check if we've reached the end of the entries */
            if (entry->rec_len == 0) {
                break;
            }

            /* Check if this is a valid entry */
            if (entry->inode != 0) {
                if (entry_idx == index) {
                    /* Copy the entry information to the dirent structure */
                    dirent->inode = entry->inode;

                    /* Convert file type */
                    switch (entry->file_type) {
                        case EXT4_FT_REG_FILE:
                            dirent->type = VFS_FILE;
                            break;
                        case EXT4_FT_DIR:
                            dirent->type = VFS_DIRECTORY;
                            break;
                        case EXT4_FT_SYMLINK:
                            dirent->type = VFS_SYMLINK;
                            break;
                        case EXT4_FT_CHRDEV:
                            dirent->type = VFS_CHARDEVICE;
                            break;
                        case EXT4_FT_BLKDEV:
                            dirent->type = VFS_BLOCKDEVICE;
                            break;
                        case EXT4_FT_FIFO:
                            dirent->type = VFS_PIPE;
                            break;
                        case EXT4_FT_SOCK:
                            dirent->type = VFS_SOCKET;
                            break;
                        default:
                            dirent->type = VFS_FILE;
                            break;
                    }

                    /* Copy the name */
                    uint32_t name_len = entry->name_len;
                    if (name_len > VFS_NAME_MAX) {
                        name_len = VFS_NAME_MAX;
                    }
                    memcpy(dirent->name, entry->name, name_len);
                    dirent->name[name_len] = '\0';

                    kfree(dir_buffer);
                    return 0;
                }

                entry_idx++;
            }

            /* Move to the next entry */
            offset += entry->rec_len;
        }
    }

    /* No more entries */
    kfree(dir_buffer);
    return -1;
}

/**
 * VFS finddir function
 */
static struct vfs_node* ext4_finddir(struct vfs_node *node, const char *name) {
    if (!node || !name || !node->private_data || node->type != VFS_DIRECTORY) {
        return NULL;
    }

    ext4_inode_info_t *info = (ext4_inode_info_t *)node->private_data;
    ext4_fs_t *fs = info->fs;
    ext4_inode_t *dir_inode = &info->raw_inode;

    /* Find the directory entry */
    uint32_t inode_num;
    int result = ext4_find_dir_entry(fs, dir_inode, name, &inode_num);
    if (result < 0) {
        return NULL;
    }

    /* Create a new VFS node for the found inode */
    struct vfs_node *found_node = (struct vfs_node *)kmalloc(sizeof(struct vfs_node));
    if (!found_node) {
        kerr("EXT4: Failed to allocate memory for node\n");
        return NULL;
    }

    result = ext4_create_vfs_node(fs, inode_num, found_node);
    if (result < 0) {
        kfree(found_node);
        return NULL;
    }

    return found_node;
}

/**
 * VFS stat function
 */
static int ext4_stat(struct vfs_node *node, struct vfs_stat *stat) {
    if (!node || !stat || !node->private_data) {
        return -1;
    }

    ext4_inode_info_t *info = (ext4_inode_info_t *)node->private_data;
    ext4_inode_t *inode = &info->raw_inode;

    /* Fill in the stat structure */
    stat->st_dev = 0; /* Device ID */
    stat->st_ino = info->inode_num;
    stat->st_mode = inode->i_mode;
    stat->st_nlink = inode->i_links_count;
    stat->st_uid = inode->i_uid | ((uint32_t)inode->osd2.linux2.l_i_uid_high << 16);
    stat->st_gid = inode->i_gid | ((uint32_t)inode->osd2.linux2.l_i_gid_high << 16);
    stat->st_rdev = 0; /* Device ID (if special file) */
    stat->st_size = inode->i_size_lo | ((uint64_t)inode->i_size_high << 32);
    stat->st_blksize = info->fs->block_size;
    stat->st_blocks = inode->i_blocks_lo | ((uint64_t)inode->osd2.linux2.l_i_blocks_high << 32);
    stat->st_atime = inode->i_atime;
    stat->st_mtime = inode->i_mtime;
    stat->st_ctime = inode->i_ctime;

    return 0;
}

/**
 * Initialize ext4 filesystem support
 * @return 0 on success, negative on error
 */
int ext4_init(void) {
    kprintf("EXT4: Initializing filesystem driver\n");
    return 0;
}

/**
 * Probe function for driver registration
 * @param driver The driver being probed
 * @return 0 on success, negative on error
 */
static int ext4_probe_driver(device_driver_t *driver) {
    return ext4_init();
}

/**
 * Remove function for driver unregistration
 * @param driver The driver being removed
 * @return 0 on success, negative on error
 */
static int ext4_remove_driver(device_driver_t *driver) {
    /* Nothing to do for now */
    return 0;
}

/**
 * Register the ext4 filesystem driver
 */
void ext4_register_driver(void) {
    device_driver_register(&ext4_driver);
}