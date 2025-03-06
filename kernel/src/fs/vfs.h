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

#ifndef _FS_VFS_H
#define _FS_VFS_H

#include <stdint.h>
#include <stddef.h>

/* File system node types */
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08
#define VFS_SOCKET      0x09

/* File access modes */
#define VFS_O_RDONLY    0x0000
#define VFS_O_WRONLY    0x0001
#define VFS_O_RDWR      0x0002
#define VFS_O_APPEND    0x0008
#define VFS_O_CREAT     0x0100
#define VFS_O_TRUNC     0x0200
#define VFS_O_EXCL      0x0400
#define VFS_O_NOFOLLOW  0x0800
#define VFS_O_DIRECTORY 0x1000

/* Seek modes */
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

/* File permissions */
#define S_IFMT  0170000 /* Mask for file type */
#define S_IFSOCK 0140000 /* Socket */
#define S_IFLNK  0120000 /* Symbolic link */
#define S_IFREG  0100000 /* Regular file */
#define S_IFBLK  0060000 /* Block device */
#define S_IFDIR  0040000 /* Directory */
#define S_IFCHR  0020000 /* Character device */
#define S_IFIFO  0010000 /* FIFO */

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700 /* User: read, write, execute */
#define S_IRUSR 00400 /* User: read */
#define S_IWUSR 00200 /* User: write */
#define S_IXUSR 00100 /* User: execute */
#define S_IRWXG 00070 /* Group: read, write, execute */
#define S_IRGRP 00040 /* Group: read */
#define S_IWGRP 00020 /* Group: write */
#define S_IXGRP 00010 /* Group: execute */
#define S_IRWXO 00007 /* Other: read, write, execute */
#define S_IROTH 00004 /* Other: read */
#define S_IWOTH 00002 /* Other: write */
#define S_IXOTH 00001 /* Other: execute */

/* Maximum filename length */
#define VFS_NAME_MAX 255

/* Forward declarations */
struct vfs_node;
struct vfs_dirent;
struct vfs_stat;

/* Function pointer types for VFS operations */
typedef int          (*vfs_open_t)(struct vfs_node*, int flags);
typedef int          (*vfs_close_t)(struct vfs_node*);
typedef size_t       (*vfs_read_t)(struct vfs_node*, uint64_t offset, size_t size, void* buffer);
typedef size_t       (*vfs_write_t)(struct vfs_node*, uint64_t offset, size_t size, const void* buffer);
typedef int          (*vfs_readdir_t)(struct vfs_node*, uint32_t index, struct vfs_dirent* dirent);
typedef struct vfs_node* (*vfs_finddir_t)(struct vfs_node*, const char* name);
typedef int          (*vfs_create_t)(struct vfs_node*, const char* name, uint16_t mode);
typedef int          (*vfs_unlink_t)(struct vfs_node*, const char* name);
typedef int          (*vfs_mkdir_t)(struct vfs_node*, const char* name, uint16_t mode);
typedef int          (*vfs_rmdir_t)(struct vfs_node*, const char* name);
typedef int          (*vfs_rename_t)(struct vfs_node*, const char* oldname, const char* newname);
typedef int          (*vfs_link_t)(struct vfs_node*, const char* oldname, const char* newname);
typedef int          (*vfs_symlink_t)(struct vfs_node*, const char* target, const char* name);
typedef int          (*vfs_readlink_t)(struct vfs_node*, char* buffer, size_t size);
typedef int          (*vfs_chmod_t)(struct vfs_node*, uint16_t mode);
typedef int          (*vfs_chown_t)(struct vfs_node*, uint32_t uid, uint32_t gid);
typedef int          (*vfs_truncate_t)(struct vfs_node*, uint64_t size);

/* VFS node operations */
typedef struct vfs_node_ops {
    vfs_open_t     open;
    vfs_close_t    close;
    vfs_read_t     read;
    vfs_write_t    write;
    vfs_readdir_t  readdir;
    vfs_finddir_t  finddir;
    vfs_create_t   create;
    vfs_unlink_t   unlink;
    vfs_mkdir_t    mkdir;
    vfs_rmdir_t    rmdir;
    vfs_rename_t   rename;
    vfs_link_t     link;
    vfs_symlink_t  symlink;
    vfs_readlink_t readlink;
    int           (*stat)(struct vfs_node*, struct vfs_stat*);
    vfs_chmod_t    chmod;
    vfs_chown_t    chown;
    vfs_truncate_t truncate;
} vfs_node_ops_t;

/* VFS node structure */
typedef struct vfs_node {
    char     name[VFS_NAME_MAX + 1]; /* Name of the node */
    uint32_t type;                   /* Type of node */
    uint32_t permissions;            /* Permissions */
    uint32_t uid;                    /* User ID */
    uint32_t gid;                    /* Group ID */
    uint64_t size;                   /* Size of file in bytes */
    uint32_t inode;                  /* Inode number */
    uint32_t links;                  /* Number of links */
    uint32_t atime;                  /* Access time */
    uint32_t mtime;                  /* Modification time */
    uint32_t ctime;                  /* Creation time */
    void*    private_data;           /* Filesystem-specific data */
    struct vfs_node* mount_point;    /* If this is a mountpoint, points to the mounted node */
    vfs_node_ops_t*  ops;            /* Operations for this node */
} vfs_node_t;

/* Directory entry structure */
typedef struct vfs_dirent {
    char     name[VFS_NAME_MAX + 1]; /* Name of the entry */
    uint32_t inode;                  /* Inode number */
    uint8_t  type;                   /* Type of the entry */
} vfs_dirent_t;

/* Stat structure */
typedef struct vfs_stat {
    uint32_t st_dev;         /* ID of device containing file */
    uint32_t st_ino;         /* Inode number */
    uint16_t st_mode;        /* File type and mode */
    uint16_t st_nlink;       /* Number of hard links */
    uint32_t st_uid;         /* User ID of owner */
    uint32_t st_gid;         /* Group ID of owner */
    uint32_t st_rdev;        /* Device ID (if special file) */
    uint64_t st_size;        /* Total size in bytes */
    uint32_t st_blksize;     /* Block size for filesystem I/O */
    uint64_t st_blocks;      /* Number of 512B blocks allocated */
    uint32_t st_atime;       /* Time of last access */
    uint32_t st_mtime;       /* Time of last modification */
    uint32_t st_ctime;       /* Time of last status change */
} vfs_stat_t;

/* VFS initialization */
int vfs_init(void);

/* VFS core functions */
int vfs_mount(const char* path, struct vfs_node* node);
int vfs_unmount(const char* path);
struct vfs_node* vfs_lookup(const char* path);
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
size_t vfs_read(int fd, void* buffer, size_t size);
size_t vfs_write(int fd, const void* buffer, size_t size);
int vfs_stat(const char* path, struct vfs_stat* stat);
int vfs_fstat(int fd, struct vfs_stat* stat);
int vfs_mkdir(const char* path, uint16_t mode);
int vfs_rmdir(const char* path);
int vfs_readdir(const char* path, uint32_t index, struct vfs_dirent* dirent);
int vfs_create(const char* path, uint16_t mode);
int vfs_unlink(const char* path);
int vfs_rename(const char* oldpath, const char* newpath);
int vfs_link(const char* oldpath, const char* newpath);
int vfs_symlink(const char* target, const char* linkpath);
int vfs_readlink(const char* path, char* buffer, size_t size);
int vfs_chmod(const char* path, uint16_t mode);
int vfs_chown(const char* path, uint32_t uid, uint32_t gid);
int vfs_truncate(const char* path, uint64_t size);
int vfs_ftruncate(int fd, uint64_t size);
uint64_t vfs_lseek(int fd, int64_t offset, int whence);

#endif /* _FS_VFS_H */