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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/minstd.h>
#include <kernel/io.h>
#include <fs/vfs.h>
#include <mm/kmalloc.h>

/* Maximum number of open files */
#define MAX_OPEN_FILES 256

/* Maximum number of mountpoints */
#define MAX_MOUNTS 32

/* Maximum path length */
#define MAX_PATH_LENGTH 512

/* File descriptor structure */
typedef struct {
    struct vfs_node *node;  /* VFS node this FD refers to */
    uint32_t flags;         /* Open flags */
    uint64_t position;      /* Current file position */
    bool used;              /* Whether this FD is in use */
} file_descriptor_t;

/* Mount point structure */
typedef struct {
    char path[MAX_PATH_LENGTH];  /* Mount point path */
    struct vfs_node *node;       /* Root node of mounted filesystem */
    bool used;                   /* Whether this mount is in use */
} mount_point_t;

/* Global file descriptor table */
static file_descriptor_t fd_table[MAX_OPEN_FILES];

/* Global mount point table */
static mount_point_t mount_table[MAX_MOUNTS];

/* Root filesystem node */
static struct vfs_node *root_node = NULL;

/**
 * Initialize the VFS subsystem
 * @return 0 on success, negative on error
 */
int vfs_init(void) {
    kprintf("VFS: Initializing virtual filesystem\n");

    /* Clear file descriptor and mount tables */
    memset(fd_table, 0, sizeof(fd_table));
    memset(mount_table, 0, sizeof(mount_table));

    /* Root node is initially NULL until root filesystem is mounted */
    root_node = NULL;

    kprintf("VFS: Initialization complete\n");
    return 0;
}

/**
 * Normalize a path by removing unnecessary components
 * @param path The input path
 * @param normalized_path Buffer to store the normalized path
 * @return 0 on success, negative on error
 */
static int vfs_normalize_path(const char *path, char *normalized_path) {
    if (!path || !normalized_path) {
        return -1;
    }

    /* Initialize normalized path buffer */
    normalized_path[0] = '\0';

    /* Handle empty path */
    if (path[0] == '\0') {
        normalized_path[0] = '/';
        normalized_path[1] = '\0';
        return 0;
    }

    /* Create a temporary copy of the path for tokenization */
    char path_copy[MAX_PATH_LENGTH];
    if (strlen(path) >= MAX_PATH_LENGTH) {
        return -1; /* Path too long */
    }
    strcpy(path_copy, path);

    /* Tokenize the path and process each component */
    char *token;
    char *rest = path_copy;
    bool absolute = (path[0] == '/');

    /* Start with root for absolute paths */
    if (absolute) {
        normalized_path[0] = '/';
        normalized_path[1] = '\0';
    }

    /* Process each path component */
    while ((token = strtok_r(rest, "/", &rest)) != NULL) {
        /* Skip empty components and '.' */
        if (token[0] == '\0' || (token[0] == '.' && token[1] == '\0')) {
            continue;
        }

        /* Handle '..' (go up one directory) */
        if (token[0] == '.' && token[1] == '.' && token[2] == '\0') {
            /* Remove last component from normalized path */
            char *last_slash = strrchr(normalized_path, '/');
            if (last_slash != NULL) {
                /* Don't go above root directory */
                if (last_slash == normalized_path) {
                    last_slash[1] = '\0'; /* Keep root slash */
                } else {
                    last_slash[0] = '\0'; /* Remove slash and everything after */
                }
            }
            continue;
        }

        /* Add component to normalized path */
        size_t current_len = strlen(normalized_path);
        if (current_len > 0 && normalized_path[current_len - 1] != '/') {
            if (current_len + 1 + strlen(token) >= MAX_PATH_LENGTH) {
                return -1; /* Path too long */
            }
            strcat(normalized_path, "/");
        }

        if (strlen(normalized_path) + strlen(token) >= MAX_PATH_LENGTH) {
            return -1; /* Path too long */
        }
        strcat(normalized_path, token);
    }

    /* Handle case where normalized_path is empty */
    if (normalized_path[0] == '\0') {
        if (absolute) {
            normalized_path[0] = '/';
            normalized_path[1] = '\0';
        } else {
            normalized_path[0] = '.';
            normalized_path[1] = '\0';
        }
    }

    return 0;
}

/**
 * Find the VFS node corresponding to a path
 * @param path The path to look up
 * @return The VFS node, or NULL if not found
 */
struct vfs_node* vfs_lookup(const char *path) {
    if (!path || root_node == NULL) {
        return NULL;
    }

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return NULL;
    }

    /* Handle root path */
    if (strcmp(normalized_path, "/") == 0) {
        return root_node;
    }

    /* Skip leading slash */
    const char *current_path = normalized_path;
    if (current_path[0] == '/') {
        current_path++;
    }

    /* Start from root node */
    struct vfs_node *current_node = root_node;

    /* Tokenize the path and traverse the directory structure */
    char path_copy[MAX_PATH_LENGTH];
    strcpy(path_copy, current_path);

    char *token;
    char *rest = path_copy;

    while ((token = strtok_r(rest, "/", &rest)) != NULL) {
        /* Check if current node is a directory */
        if (current_node->type != VFS_DIRECTORY) {
            return NULL; /* Not a directory */
        }

        /* Check if node has finddir operation */
        if (!current_node->ops || !current_node->ops->finddir) {
            return NULL; /* Cannot traverse */
        }

        /* Find the next node */
        struct vfs_node *next_node = current_node->ops->finddir(current_node, token);
        if (!next_node) {
            return NULL; /* Node not found */
        }

        /* Check for mount point */
        if (next_node->mount_point) {
            next_node = next_node->mount_point;
        }

        /* Update current node */
        current_node = next_node;
    }

    return current_node;
}

/**
 * Mount a filesystem at a specified path
 * @param path The path to mount at
 * @param node The root node of the filesystem to mount
 * @return 0 on success, negative on error
 */
int vfs_mount(const char *path, struct vfs_node *node) {
    if (!node) {
        return -1;
    }

    /* If path is "/", this is the root filesystem */
    if (path == NULL || strcmp(path, "/") == 0) {
        root_node = node;
        return 0;
    }

    /* Root filesystem must be mounted first */
    if (root_node == NULL) {
        return -1;
    }

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Find mount point */
    struct vfs_node *mount_point = vfs_lookup(normalized_path);
    if (!mount_point) {
        return -1; /* Mount point not found */
    }

    /* Mount point must be a directory */
    if (mount_point->type != VFS_DIRECTORY) {
        return -1;
    }

    /* Find an empty slot in the mount table */
    int i;
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].used) {
            break;
        }
    }

    if (i == MAX_MOUNTS) {
        return -1; /* No free slots */
    }

    /* Set up the mount entry */
    strcpy(mount_table[i].path, normalized_path);
    mount_table[i].node = node;
    mount_table[i].used = true;

    /* Link mount point to mounted node */
    mount_point->mount_point = node;
    mount_point->type |= VFS_MOUNTPOINT;

    kprintf("VFS: Mounted filesystem at %s\n", normalized_path);
    return 0;
}

/**
 * Unmount a filesystem
 * @param path The path where the filesystem is mounted
 * @return 0 on success, negative on error
 */
int vfs_unmount(const char *path) {
    /* Cannot unmount root filesystem */
    if (path == NULL || strcmp(path, "/") == 0) {
        return -1;
    }

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Find the mount entry */
    int i;
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (mount_table[i].used && strcmp(mount_table[i].path, normalized_path) == 0) {
            break;
        }
    }

    if (i == MAX_MOUNTS) {
        return -1; /* Mount point not found */
    }

    /* Find the mount point node */
    struct vfs_node *mount_point = vfs_lookup(normalized_path);
    if (!mount_point) {
        return -1;
    }

    /* Unlink mount point */
    mount_point->mount_point = NULL;
    mount_point->type &= ~VFS_MOUNTPOINT;

    /* Clear the mount entry */
    mount_table[i].used = false;

    kprintf("VFS: Unmounted filesystem from %s\n", normalized_path);
    return 0;
}

/**
 * Open a file
 * @param path Path to the file
 * @param flags Open mode flags
 * @return File descriptor on success, negative on error
 */
int vfs_open(const char *path, int flags) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if the node has an open operation */
    if (!node->ops || !node->ops->open) {
        return -1; /* Cannot open */
    }

    /* Call the node's open operation */
    int result = node->ops->open(node, flags);
    if (result < 0) {
        return result;
    }

    /* Find a free file descriptor */
    int fd;
    for (fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (!fd_table[fd].used) {
            break;
        }
    }

    if (fd == MAX_OPEN_FILES) {
        if (node->ops->close) {
            node->ops->close(node);
        }
        return -1; /* No free file descriptors */
    }

    /* Set up the file descriptor */
    fd_table[fd].node = node;
    fd_table[fd].flags = flags;
    fd_table[fd].position = 0;
    fd_table[fd].used = true;

    return fd;
}

/**
 * Close a file descriptor
 * @param fd The file descriptor to close
 * @return 0 on success, negative on error
 */
int vfs_close(int fd) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;

    /* Call the node's close operation if available */
    if (node->ops && node->ops->close) {
        int result = node->ops->close(node);
        if (result < 0) {
            return result;
        }
    }

    /* Free the file descriptor */
    fd_table[fd].used = false;

    return 0;
}

/**
 * Read from a file descriptor
 * @param fd The file descriptor
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative on error
 */
size_t vfs_read(int fd, void *buffer, size_t size) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;

    /* Check if node has read operation */
    if (!node->ops || !node->ops->read) {
        return -1;
    }

    /* Call the node's read operation */
    size_t bytes_read = node->ops->read(node, fd_table[fd].position, size, buffer);

    /* Update file position */
    fd_table[fd].position += bytes_read;

    return bytes_read;
}

/**
 * Write to a file descriptor
 * @param fd The file descriptor
 * @param buffer Buffer to write from
 * @param size Number of bytes to write
 * @return Number of bytes written, or negative on error
 */
size_t vfs_write(int fd, const void *buffer, size_t size) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;

    /* Check if node has write operation */
    if (!node->ops || !node->ops->write) {
        return -1;
    }

    /* Call the node's write operation */
    size_t bytes_written = node->ops->write(node, fd_table[fd].position, size, buffer);

    /* Update file position */
    fd_table[fd].position += bytes_written;

    return bytes_written;
}

/**
 * Seek to a position in a file
 * @param fd The file descriptor
 * @param offset The offset to seek to
 * @param whence The reference position for the offset
 * @return The new file position, or negative on error
 */
uint64_t vfs_lseek(int fd, int64_t offset, int whence) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;
    uint64_t new_position;

    /* Calculate new position based on whence */
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = fd_table[fd].position + offset;
            break;
        case SEEK_END:
            new_position = node->size + offset;
            break;
        default:
            return -1;
    }

    /* Check bounds */
    if (new_position > node->size && !(fd_table[fd].flags & VFS_O_WRONLY || fd_table[fd].flags & VFS_O_RDWR)) {
        return -1; /* Cannot seek past end in read-only mode */
    }

    /* Update position */
    fd_table[fd].position = new_position;

    return new_position;
}

/**
 * Read directory entries
 * @param path Path to the directory
 * @param index Index of the entry to read
 * @param dirent Directory entry structure to fill
 * @return 0 on success, negative on error
 */
int vfs_readdir(const char *path, uint32_t index, struct vfs_dirent *dirent) {
    /* Find the directory node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* Directory not found */
    }

    /* Check if node is a directory */
    if (node->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if node has readdir operation */
    if (!node->ops || !node->ops->readdir) {
        return -1;
    }

    /* Call the node's readdir operation */
    return node->ops->readdir(node, index, dirent);
}

/**
 * Get file/directory information
 * @param path Path to the file/directory
 * @param stat Structure to fill with information
 * @return 0 on success, negative on error
 */
int vfs_stat(const char *path, struct vfs_stat *stat) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if node has stat operation */
    if (!node->ops || !node->ops->stat) {
        return -1;
    }

    /* Call the node's stat operation */
    return node->ops->stat(node, stat);
}

/**
 * Get file/directory information by file descriptor
 * @param fd File descriptor
 * @param stat Structure to fill with information
 * @return 0 on success, negative on error
 */
int vfs_fstat(int fd, struct vfs_stat *stat) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;

    /* Check if node has stat operation */
    if (!node->ops || !node->ops->stat) {
        return -1;
    }

    /* Call the node's stat operation */
    return node->ops->stat(node, stat);
}

/**
 * Create a directory
 * @param path Path to the directory to create
 * @param mode Directory permissions
 * @return 0 on success, negative on error
 */
int vfs_mkdir(const char *path, uint16_t mode) {
    /* Get parent directory path and basename */
    char parent_path[MAX_PATH_LENGTH];
    char basename[VFS_NAME_MAX + 1];

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Extract parent path and basename */
    char *last_slash = strrchr(normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(parent_path, ".");
        strcpy(basename, normalized_path);
    } else {
        if (last_slash == normalized_path) {
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, normalized_path, last_slash - normalized_path);
            parent_path[last_slash - normalized_path] = '\0';
        }
        strcpy(basename, last_slash + 1);
    }

    /* Find the parent directory node */
    struct vfs_node *parent = vfs_lookup(parent_path);
    if (!parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parent is a directory */
    if (parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if parent has mkdir operation */
    if (!parent->ops || !parent->ops->mkdir) {
        return -1;
    }

    /* Call the parent's mkdir operation */
    return parent->ops->mkdir(parent, basename, mode);
}

/**
 * Remove a directory
 * @param path Path to the directory to remove
 * @return 0 on success, negative on error
 */
int vfs_rmdir(const char *path) {
    /* Get parent directory path and basename */
    char parent_path[MAX_PATH_LENGTH];
    char basename[VFS_NAME_MAX + 1];

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Extract parent path and basename */
    char *last_slash = strrchr(normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(parent_path, ".");
        strcpy(basename, normalized_path);
    } else {
        if (last_slash == normalized_path) {
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, normalized_path, last_slash - normalized_path);
            parent_path[last_slash - normalized_path] = '\0';
        }
        strcpy(basename, last_slash + 1);
    }

    /* Find the parent directory node */
    struct vfs_node *parent = vfs_lookup(parent_path);
    if (!parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parent is a directory */
    if (parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if parent has rmdir operation */
    if (!parent->ops || !parent->ops->rmdir) {
        return -1;
    }

    /* Call the parent's rmdir operation */
    return parent->ops->rmdir(parent, basename);
}

/**
 * Create a file
 * @param path Path to the file to create
 * @param mode File permissions
 * @return 0 on success, negative on error
 */
int vfs_create(const char *path, uint16_t mode) {
    /* Get parent directory path and basename */
    char parent_path[MAX_PATH_LENGTH];
    char basename[VFS_NAME_MAX + 1];

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Extract parent path and basename */
    char *last_slash = strrchr(normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(parent_path, ".");
        strcpy(basename, normalized_path);
    } else {
        if (last_slash == normalized_path) {
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, normalized_path, last_slash - normalized_path);
            parent_path[last_slash - normalized_path] = '\0';
        }
        strcpy(basename, last_slash + 1);
    }

    /* Find the parent directory node */
    struct vfs_node *parent = vfs_lookup(parent_path);
    if (!parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parent is a directory */
    if (parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if parent has create operation */
    if (!parent->ops || !parent->ops->create) {
        return -1;
    }

    /* Call the parent's create operation */
    return parent->ops->create(parent, basename, mode);
}

/**
 * Delete a file
 * @param path Path to the file to delete
 * @return 0 on success, negative on error
 */
int vfs_unlink(const char *path) {
    /* Get parent directory path and basename */
    char parent_path[MAX_PATH_LENGTH];
    char basename[VFS_NAME_MAX + 1];

    /* Normalize the path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(path, normalized_path) < 0) {
        return -1;
    }

    /* Extract parent path and basename */
    char *last_slash = strrchr(normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(parent_path, ".");
        strcpy(basename, normalized_path);
    } else {
        if (last_slash == normalized_path) {
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, normalized_path, last_slash - normalized_path);
            parent_path[last_slash - normalized_path] = '\0';
        }
        strcpy(basename, last_slash + 1);
    }

    /* Find the parent directory node */
    struct vfs_node *parent = vfs_lookup(parent_path);
    if (!parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parent is a directory */
    if (parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if parent has unlink operation */
    if (!parent->ops || !parent->ops->unlink) {
        return -1;
    }

    /* Call the parent's unlink operation */
    return parent->ops->unlink(parent, basename);
}

/**
 * Rename a file or directory
 * @param oldpath Old path
 * @param newpath New path
 * @return 0 on success, negative on error
 */
int vfs_rename(const char *oldpath, const char *newpath) {
    /* Get parent directory paths and basenames */
    char old_parent_path[MAX_PATH_LENGTH];
    char old_basename[VFS_NAME_MAX + 1];
    char new_parent_path[MAX_PATH_LENGTH];
    char new_basename[VFS_NAME_MAX + 1];

    /* Normalize the paths */
    char old_normalized_path[MAX_PATH_LENGTH];
    char new_normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(oldpath, old_normalized_path) < 0 ||
        vfs_normalize_path(newpath, new_normalized_path) < 0) {
        return -1;
    }

    /* Extract old parent path and basename */
    char *last_slash = strrchr(old_normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(old_parent_path, ".");
        strcpy(old_basename, old_normalized_path);
    } else {
        if (last_slash == old_normalized_path) {
            strcpy(old_parent_path, "/");
        } else {
            strncpy(old_parent_path, old_normalized_path, last_slash - old_normalized_path);
            old_parent_path[last_slash - old_normalized_path] = '\0';
        }
        strcpy(old_basename, last_slash + 1);
    }

    /* Extract new parent path and basename */
    last_slash = strrchr(new_normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(new_parent_path, ".");
        strcpy(new_basename, new_normalized_path);
    } else {
        if (last_slash == new_normalized_path) {
            strcpy(new_parent_path, "/");
        } else {
            strncpy(new_parent_path, new_normalized_path, last_slash - new_normalized_path);
            new_parent_path[last_slash - new_normalized_path] = '\0';
        }
        strcpy(new_basename, last_slash + 1);
    }

    /* Find the parent directory nodes */
    struct vfs_node *old_parent = vfs_lookup(old_parent_path);
    struct vfs_node *new_parent = vfs_lookup(new_parent_path);
    if (!old_parent || !new_parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parents are directories */
    if (old_parent->type != VFS_DIRECTORY || new_parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* If the parent directories are the same, use the rename operation */
    if (old_parent == new_parent) {
        /* Check if parent has rename operation */
        if (!old_parent->ops || !old_parent->ops->rename) {
            return -1;
        }

        /* Call the parent's rename operation */
        return old_parent->ops->rename(old_parent, old_basename, new_basename);
    } else {
        /* Special case for rename across directories */
        /* This involves creating a new file and deleting the old one */
        /* Not fully implemented in this skeleton */
        return -1;
    }
}

/**
 * Create a hard link
 * @param oldpath Path to the existing file
 * @param newpath Path for the new link
 * @return 0 on success, negative on error
 */
int vfs_link(const char *oldpath, const char *newpath) {
    /* Get parent directory paths and basenames */
    char old_parent_path[MAX_PATH_LENGTH];
    char old_basename[VFS_NAME_MAX + 1];
    char new_parent_path[MAX_PATH_LENGTH];
    char new_basename[VFS_NAME_MAX + 1];

    /* Normalize the paths */
    char old_normalized_path[MAX_PATH_LENGTH];
    char new_normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(oldpath, old_normalized_path) < 0 ||
        vfs_normalize_path(newpath, new_normalized_path) < 0) {
        return -1;
    }

    /* Extract old parent path and basename */
    char *last_slash = strrchr(old_normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(old_parent_path, ".");
        strcpy(old_basename, old_normalized_path);
    } else {
        if (last_slash == old_normalized_path) {
            strcpy(old_parent_path, "/");
        } else {
            strncpy(old_parent_path, old_normalized_path, last_slash - old_normalized_path);
            old_parent_path[last_slash - old_normalized_path] = '\0';
        }
        strcpy(old_basename, last_slash + 1);
    }

    /* Extract new parent path and basename */
    last_slash = strrchr(new_normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(new_parent_path, ".");
        strcpy(new_basename, new_normalized_path);
    } else {
        if (last_slash == new_normalized_path) {
            strcpy(new_parent_path, "/");
        } else {
            strncpy(new_parent_path, new_normalized_path, last_slash - new_normalized_path);
            new_parent_path[last_slash - new_normalized_path] = '\0';
        }
        strcpy(new_basename, last_slash + 1);
    }

    /* Find the target file */
    struct vfs_node *target = vfs_lookup(old_normalized_path);
    if (!target) {
        return -1; /* Target file not found */
    }

    /* Cannot link directories */
    if (target->type == VFS_DIRECTORY) {
        return -1;
    }

    /* Find the new parent directory */
    struct vfs_node *new_parent = vfs_lookup(new_parent_path);
    if (!new_parent) {
        return -1; /* New parent directory not found */
    }

    /* Check if new parent is a directory */
    if (new_parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if new parent has link operation */
    if (!new_parent->ops || !new_parent->ops->link) {
        return -1;
    }

    /* Call the new parent's link operation */
    return new_parent->ops->link(new_parent, old_normalized_path, new_basename);
}

/**
 * Create a symbolic link
 * @param target Path that the link will point to
 * @param linkpath Path for the new link
 * @return 0 on success, negative on error
 */
int vfs_symlink(const char *target, const char *linkpath) {
    /* Get parent directory path and basename for the new link */
    char parent_path[MAX_PATH_LENGTH];
    char basename[VFS_NAME_MAX + 1];

    /* Normalize the link path */
    char normalized_path[MAX_PATH_LENGTH];
    if (vfs_normalize_path(linkpath, normalized_path) < 0) {
        return -1;
    }

    /* Extract parent path and basename */
    char *last_slash = strrchr(normalized_path, '/');
    if (last_slash == NULL) {
        strcpy(parent_path, ".");
        strcpy(basename, normalized_path);
    } else {
        if (last_slash == normalized_path) {
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, normalized_path, last_slash - normalized_path);
            parent_path[last_slash - normalized_path] = '\0';
        }
        strcpy(basename, last_slash + 1);
    }

    /* Find the parent directory node */
    struct vfs_node *parent = vfs_lookup(parent_path);
    if (!parent) {
        return -1; /* Parent directory not found */
    }

    /* Check if parent is a directory */
    if (parent->type != VFS_DIRECTORY) {
        return -1; /* Not a directory */
    }

    /* Check if parent has symlink operation */
    if (!parent->ops || !parent->ops->symlink) {
        return -1;
    }

    /* Call the parent's symlink operation */
    return parent->ops->symlink(parent, target, basename);
}

/**
 * Read the target of a symbolic link
 * @param path Path to the symlink
 * @param buffer Buffer to store the target path
 * @param size Size of the buffer
 * @return Number of bytes read, or negative on error
 */
int vfs_readlink(const char *path, char *buffer, size_t size) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if node is a symlink */
    if (node->type != VFS_SYMLINK) {
        return -1; /* Not a symlink */
    }

    /* Check if node has readlink operation */
    if (!node->ops || !node->ops->readlink) {
        return -1;
    }

    /* Call the node's readlink operation */
    return node->ops->readlink(node, buffer, size);
}

/**
 * Change file mode (permissions)
 * @param path Path to the file/directory
 * @param mode New mode
 * @return 0 on success, negative on error
 */
int vfs_chmod(const char *path, uint16_t mode) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if node has chmod operation */
    if (!node->ops || !node->ops->chmod) {
        return -1;
    }

    /* Call the node's chmod operation */
    return node->ops->chmod(node, mode);
}

/**
 * Change file owner and group
 * @param path Path to the file/directory
 * @param uid New user ID
 * @param gid New group ID
 * @return 0 on success, negative on error
 */
int vfs_chown(const char *path, uint32_t uid, uint32_t gid) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if node has chown operation */
    if (!node->ops || !node->ops->chown) {
        return -1;
    }

    /* Call the node's chown operation */
    return node->ops->chown(node, uid, gid);
}

/**
 * Change file size (truncate)
 * @param path Path to the file
 * @param size New size
 * @return 0 on success, negative on error
 */
int vfs_truncate(const char *path, uint64_t size) {
    /* Find the node */
    struct vfs_node *node = vfs_lookup(path);
    if (!node) {
        return -1; /* File not found */
    }

    /* Check if node is a regular file */
    if (node->type != VFS_FILE) {
        return -1; /* Not a file */
    }

    /* Check if node has truncate operation */
    if (!node->ops || !node->ops->truncate) {
        return -1;
    }

    /* Call the node's truncate operation */
    return node->ops->truncate(node, size);
}

/**
 * Change file size by file descriptor
 * @param fd File descriptor
 * @param size New size
 * @return 0 on success, negative on error
 */
int vfs_ftruncate(int fd, uint64_t size) {
    /* Check file descriptor */
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }

    struct vfs_node *node = fd_table[fd].node;

    /* Check if node is a regular file */
    if (node->type != VFS_FILE) {
        return -1; /* Not a file */
    }

    /* Check if node has truncate operation */
    if (!node->ops || !node->ops->truncate) {
        return -1;
    }

    /* Call the node's truncate operation */
    return node->ops->truncate(node, size);
}