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

#ifndef _FS_EXT4_EXT4_H
#define _FS_EXT4_EXT4_H

#include <stdint.h>
#include <stdbool.h>
#include <fs/vfs.h>
#include <drivers/block/block.h>

/* EXT4 magic number */
#define EXT4_SUPER_MAGIC    0xEF53

/* Superblock location */
#define EXT4_SUPERBLOCK_OFFSET 1024

/* Fixed inode numbers */
#define EXT4_ROOT_INO        2
#define EXT4_BAD_INO         1
#define EXT4_USR_QUOTA_INO   3
#define EXT4_GRP_QUOTA_INO   4
#define EXT4_BOOT_LOADER_INO 5
#define EXT4_UNDEL_DIR_INO   6
#define EXT4_JOURNAL_INO     8
#define EXT4_FIRST_INO       11

/* Filesystem compatibility feature flags */
#define EXT4_FEATURE_COMPAT_DIR_PREALLOC     0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES    0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL      0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR         0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE     0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX        0x0020

/* Incompatible feature flags */
#define EXT4_FEATURE_INCOMPAT_COMPRESSION    0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE       0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER        0x0004
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV    0x0008
#define EXT4_FEATURE_INCOMPAT_META_BG        0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS        0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT          0x0080
#define EXT4_FEATURE_INCOMPAT_MMP            0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG        0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE       0x0400
#define EXT4_FEATURE_INCOMPAT_DIRDATA        0x1000
#define EXT4_FEATURE_INCOMPAT_CSUM_SEED      0x2000
#define EXT4_FEATURE_INCOMPAT_LARGEDIR       0x4000
#define EXT4_FEATURE_INCOMPAT_INLINE_DATA    0x8000
#define EXT4_FEATURE_INCOMPAT_ENCRYPT        0x10000

/* Read-only compatibility feature flags */
#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER  0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE    0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR     0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE     0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM      0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK     0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE   0x0040
#define EXT4_FEATURE_RO_COMPAT_HAS_SNAPSHOT  0x0080
#define EXT4_FEATURE_RO_COMPAT_QUOTA         0x0100
#define EXT4_FEATURE_RO_COMPAT_BIGALLOC      0x0200
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM 0x0400
#define EXT4_FEATURE_RO_COMPAT_REPLICA       0x0800
#define EXT4_FEATURE_RO_COMPAT_READONLY      0x1000
#define EXT4_FEATURE_RO_COMPAT_PROJECT       0x2000

/* Custom write support feature flag */
#define EXT4_FEATURE_WRITE_SUPPORT           0x00010000

/* File types in directory entries */
#define EXT4_FT_UNKNOWN      0
#define EXT4_FT_REG_FILE     1
#define EXT4_FT_DIR          2
#define EXT4_FT_CHRDEV       3
#define EXT4_FT_BLKDEV       4
#define EXT4_FT_FIFO         5
#define EXT4_FT_SOCK         6
#define EXT4_FT_SYMLINK      7
#define EXT4_FT_MAX          8

/* Extent-related definitions */
#define EXT4_EXT_MAGIC       0xF30A
#define EXT4_EXTENT_HEADER_MAGIC 0xF30A

/* Write error codes */
typedef enum {
    EXT4_WRITE_ERROR_NONE,
    EXT4_WRITE_ERROR_NO_SPACE,
    EXT4_WRITE_ERROR_READ_FAIL,
    EXT4_WRITE_ERROR_WRITE_FAIL,
    EXT4_WRITE_ERROR_EXTENT_FULL,
    EXT4_WRITE_ERROR_READONLY
} ext4_write_error_t;

/* Write flags */
typedef enum {
    EXT4_WRITE_DEFAULT        = 0x00,
    EXT4_WRITE_SYNC           = 0x01,  /* Synchronous write */
    EXT4_WRITE_ATOMIC         = 0x02,  /* Atomic write operation */
    EXT4_WRITE_NO_OVERWRITE   = 0x04   /* Prevent overwriting existing data */
} ext4_write_flags_t;

/* EXT4 Superblock structure */
typedef struct ext4_superblock {
    uint32_t s_inodes_count;         /* Inodes count */
    uint32_t s_blocks_count_lo;      /* Blocks count */
    uint32_t s_r_blocks_count_lo;    /* Reserved blocks count */
    uint32_t s_free_blocks_count_lo; /* Free blocks count */
    uint32_t s_free_inodes_count;    /* Free inodes count */
    uint32_t s_first_data_block;     /* First Data Block */
    uint32_t s_log_block_size;       /* Block size */
    uint32_t s_log_cluster_size;     /* Allocation cluster size */
    uint32_t s_blocks_per_group;     /* # Blocks per group */
    uint32_t s_clusters_per_group;   /* # Clusters per group */
    uint32_t s_inodes_per_group;     /* # Inodes per group */
    uint32_t s_mtime;                /* Mount time */
    uint32_t s_wtime;                /* Write time */
    uint16_t s_mnt_count;            /* Mount count */
    uint16_t s_max_mnt_count;        /* Maximal mount count */
    uint16_t s_magic;                /* Magic signature */
    uint16_t s_state;                /* File system state */
    uint16_t s_errors;               /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level;      /* Minor revision level */
    uint32_t s_lastcheck;            /* Time of last check */
    uint32_t s_checkinterval;        /* Max. time between checks */
    uint32_t s_creator_os;           /* OS */
    uint32_t s_rev_level;            /* Revision level */
    uint16_t s_def_resuid;           /* Default uid for reserved blocks */
    uint16_t s_def_resgid;           /* Default gid for reserved blocks */

    /* EXT4 specific fields */
    uint32_t s_first_ino;            /* First non-reserved inode */
    uint16_t s_inode_size;           /* Size of inode structure */
    uint16_t s_block_group_nr;       /* Block group # of this superblock */
    uint32_t s_feature_compat;       /* Compatible feature set */
    uint32_t s_feature_incompat;     /* Incompatible feature set */
    uint32_t s_feature_ro_compat;    /* Readonly-compatible feature set */
    uint8_t  s_uuid[16];             /* 128-bit uuid for volume */
    char     s_volume_name[16];      /* Volume name */
    char     s_last_mounted[64];     /* Directory where last mounted */

    uint32_t s_blocks_count_hi;      /* Blocks count high 32-bits */
    uint16_t s_desc_size;            /* Size of group descriptor */
    uint16_t s_reserved_gdt_csum;    /* Checksum of the reserved group descriptors */
    uint32_t s_reserved[96];         /* Padding to the end of the block */
    uint32_t s_checksum;             /* Checksum */
} __attribute__((packed)) ext4_superblock_t;

/* Group descriptor */
typedef struct ext4_group_desc {
    uint32_t bg_block_bitmap_lo;      /* Block bitmap block */
    uint32_t bg_inode_bitmap_lo;      /* Inode bitmap block */
    uint32_t bg_inode_table_lo;       /* Inode table block */
    uint16_t bg_free_blocks_count_lo; /* Free blocks count */
    uint16_t bg_free_inodes_count_lo; /* Free inodes count */
    uint16_t bg_used_dirs_count_lo;   /* Directories count */
    uint16_t bg_flags;                /* EXT4_BG_flags (INODE_UNINIT, etc) */
    uint32_t bg_exclude_bitmap_lo;    /* Exclude bitmap for snapshots */
    uint16_t bg_block_bitmap_csum_lo; /* Crc32c(s_uuid+grp_num+bbitmap) LE */
    uint16_t bg_inode_bitmap_csum_lo; /* Crc32c(s_uuid+grp_num+ibitmap) LE */
    uint16_t bg_itable_unused_lo;     /* Unused inodes count */
    uint16_t bg_checksum;             /* Crc16(sb_uuid+group+desc) */

    /* 64-bit fields */
    uint32_t bg_block_bitmap_hi;      /* Blocks bitmap block MSB */
    uint32_t bg_inode_bitmap_hi;      /* Inodes bitmap block MSB */
    uint32_t bg_inode_table_hi;       /* Inodes table block MSB */
    uint16_t bg_free_blocks_count_hi; /* Free blocks count MSB */
    uint16_t bg_free_inodes_count_hi; /* Free inodes count MSB */
    uint16_t bg_used_dirs_count_hi;   /* Directories count MSB */
    uint16_t bg_itable_unused_hi;     /* Unused inodes count MSB */
    uint32_t bg_exclude_bitmap_hi;    /* Exclude bitmap block MSB */
    uint16_t bg_block_bitmap_csum_hi; /* Crc32c(s_uuid+grp_num+bbitmap) BE */
    uint16_t bg_inode_bitmap_csum_hi; /* Crc32c(s_uuid+grp_num+ibitmap) BE */
    uint32_t bg_reserved;
} __attribute__((packed)) ext4_group_desc_t;

/* EXT4 inode structure */
typedef struct ext4_inode {
    uint16_t i_mode;        /* File mode */
    uint16_t i_uid;         /* Low 16 bits of Owner Uid */
    uint32_t i_size_lo;     /* Size in bytes */
    uint32_t i_atime;       /* Access time */
    uint32_t i_ctime;       /* Inode Change time */
    uint32_t i_mtime;       /* Modification time */
    uint32_t i_dtime;       /* Deletion Time */
    uint16_t i_gid;         /* Low 16 bits of Group Id */
    uint16_t i_links_count; /* Links count */
    uint32_t i_blocks_lo;   /* Blocks count */
    uint32_t i_flags;       /* File flags */
    union {
        struct {
            uint32_t l_i_version; /* Version */
        } linux1;
        struct {
            uint32_t h_i_translator; /* Translation */
        } hurd1;
        struct {
            uint32_t m_i_reserved1; /* Reserved */
        } masix1;
    } osd1;                 /* OS dependent 1 */
    uint32_t i_block[15];   /* Pointers to blocks */
    uint32_t i_generation;  /* File version (for NFS) */
    uint32_t i_file_acl_lo; /* File ACL */
    uint32_t i_size_high;   /* Size: high 32 bits */
    uint32_t i_obso_faddr;  /* Obsoleted fragment address */
    union {
        struct {
            uint16_t l_i_blocks_high; /* Blocks count: high 16 bits */
            uint16_t l_i_file_acl_high; /* File ACL: high 16 bits */
            uint16_t l_i_uid_high;   /* Owner UID: high 16 bits */
            uint16_t l_i_gid_high;   /* Group GID: high 16 bits */
            uint16_t l_i_checksum_lo; /* crc32c(uuid+inum+inode) LE */
            uint16_t l_i_reserved;   /* Reserved */
        } linux2;
        struct {
            uint16_t h_i_reserved1;  /* Reserved */
            uint16_t h_i_mode_high;  /* Mode high bits */
            uint16_t h_i_uid_high;   /* Owner UID: high 16 bits */
            uint16_t h_i_gid_high;   /* Group GID: high 16 bits */
            uint32_t h_i_author;     /* Author */
        } hurd2;
        struct {
            uint16_t h_i_reserved1;  /* Reserved */
            uint16_t m_i_file_acl_high; /* File ACL: high 16 bits */
            uint32_t m_i_reserved2[2]; /* Reserved */
        } masix2;
    } osd2;                 /* OS dependent 2 */
} __attribute__((packed)) ext4_inode_t;

/* Extent header structure */
typedef struct ext4_extent_header {
    uint16_t eh_magic;      /* Magic number, 0xF30A */
    uint16_t eh_entries;    /* Number of valid entries */
    uint16_t eh_max;        /* Maximum number of entries */
    uint16_t eh_depth;      /* Has tree depth (0 for leaf) */
    uint32_t eh_generation; /* Generation of the tree */
} __attribute__((packed)) ext4_extent_header_t;

/* Extent index (internal nodes of the tree) */
typedef struct ext4_extent_idx {
    uint32_t ei_block;      /* Index covers logical blocks from 'block' */
    uint32_t ei_leaf_lo;    /* Pointer to the physical block of the next level */
    uint16_t ei_leaf_hi;    /* High 16 bits of physical block */
    uint16_t ei_unused;     /* Unused */
} __attribute__((packed)) ext4_extent_idx_t;

/* Extent (leaf nodes) */
typedef struct ext4_extent {
    uint32_t ee_block;      /* First logical block extent covers */
    uint16_t ee_len;        /* Number of blocks covered by extent */
    uint16_t ee_start_hi;   /* High 16 bits of physical block */
    uint32_t ee_start_lo;   /* Low 32 bits of physical block */
} __attribute__((packed)) ext4_extent_t;

/* Combined extent tree node */
typedef struct ext4_extent_node {
    ext4_extent_header_t header;
    union {
        ext4_extent_idx_t index[4]; /* Max 4 indices in a 64-byte node */
        ext4_extent_t     extent[4]; /* Max 4 extents in a 64-byte node */
    };
} __attribute__((packed)) ext4_extent_node_t;

/* Directory entry structure */
typedef struct ext4_dir_entry {
    uint32_t inode;         /* Inode number */
    uint16_t rec_len;       /* Directory entry length */
    uint8_t  name_len;      /* Name length */
    uint8_t  file_type;     /* File type */
    char     name[];        /* File name, up to EXT4_NAME_LEN (255) */
} __attribute__((packed)) ext4_dir_entry_t;

/* Directory entry structure for hash tree directories */
typedef struct ext4_dir_entry_2 {
    uint32_t inode;         /* Inode number */
    uint16_t rec_len;       /* Directory entry length */
    uint8_t  name_len;      /* Name length */
    uint8_t  file_type;     /* File type */
    char     name[];        /* File name, up to EXT4_NAME_LEN (255) */
} __attribute__((packed)) ext4_dir_entry_2_t;

/* Write result structure */
typedef struct {
    ext4_write_error_t error_code;
    uint64_t error_block;
    uint64_t bytes_written;
} ext4_write_result_t;

/* EXT4 filesystem in-memory structure */
typedef struct ext4_fs {
    block_device_t *device;       /* Block device containing the filesystem */
    ext4_superblock_t sb;         /* Superblock data */
    uint32_t block_size;          /* Block size in bytes */
    uint64_t block_count;         /* Total number of blocks */
    uint32_t groups_count;        /* Number of block groups */
    uint32_t inodes_per_group;    /* Number of inodes per group */
    uint32_t blocks_per_group;    /* Number of blocks per group */
    ext4_group_desc_t *group_desc_table; /* Group descriptor table */
    struct vfs_node *root_node;   /* VFS node for the root directory */

    /* Write support metadata */
    uint64_t free_blocks;         /* Number of free blocks */
    uint64_t free_inodes;         /* Number of free inodes */

    /* Optional write journal */
    void *write_journal;          /* Placeholder for write journaling */
} ext4_fs_t;

/* In-memory inode information */
typedef struct ext4_inode_info {
    ext4_inode_t raw_inode;      /* Raw inode data from disk */
    uint32_t inode_num;          /* Inode number */
    ext4_fs_t *fs;               /* Filesystem this inode belongs to */

    /* Additional metadata for write support */
    uint64_t allocated_blocks;   /* Number of blocks allocated */
    uint64_t last_write_time;    /* Timestamp of last write */
} ext4_inode_info_t;

/* Function prototypes for filesystem operations */
int ext4_init(void);
int ext4_mount(block_device_t *device, struct vfs_node **root_node);
void ext4_unmount(struct vfs_node *root_node);

/* Read operations */
int ext4_read_inode(ext4_fs_t *fs, uint32_t inode_num, ext4_inode_t *inode);
int ext4_read_block(ext4_fs_t *fs, uint64_t block_num, void *buffer);
int ext4_read_extent_block(ext4_fs_t *fs, ext4_inode_t *inode,
                           uint64_t block_num, uint64_t *phys_block);
int ext4_read_file_block(ext4_fs_t *fs, ext4_inode_t *inode,
                         uint64_t block_num, void *buffer);
int ext4_read_file_data(ext4_fs_t *fs, ext4_inode_t *inode,
                        uint64_t offset, uint64_t size, void *buffer);

/* Write operations */
int ext4_allocate_block(ext4_fs_t *fs, uint32_t group_hint, uint64_t *block_num);
int ext4_write_extent_block(ext4_fs_t *fs, ext4_inode_t *inode,
                            uint64_t block_num, const void *buffer);
int ext4_write_file_data(ext4_fs_t *fs, ext4_inode_t *inode,
                         uint64_t offset, uint64_t size, const void *buffer);
int ext4_write_file_data_ex(ext4_fs_t *fs, ext4_inode_t *inode,
                            uint64_t offset, uint64_t size,
                            const void *buffer, ext4_write_flags_t flags);

/* Filesystem management functions */
int ext4_verify_filesystem_consistency(ext4_fs_t *fs);
int ext4_repair_filesystem(ext4_fs_t *fs, int repair_level);
int ext4_update_group_descriptor(ext4_fs_t *fs, uint32_t group_num,
                                 ext4_group_desc_t *updated_desc);

/* Utility macros */
#define EXT4_HAS_WRITE_SUPPORT(sb) \
    ((sb)->s_feature_incompat & EXT4_FEATURE_WRITE_SUPPORT)

/* Logging and debugging */
void ext4_log_write_operation(ext4_fs_t *fs, uint64_t inode_num,
                              uint64_t offset, uint64_t size);

/* Register the filesystem driver */
void ext4_register_driver(void);

#endif /* _FS_EXT4_EXT4_H */