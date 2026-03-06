#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include "fat32.h"

/* Maximum path length */
#define VFS_MAX_PATH 256
#define VFS_MAX_FNAME 255

/* File/Directory info */
typedef struct {
    char name[256];
    uint32_t size;
    uint32_t cluster;
    uint8_t is_dir;
    uint8_t is_readonly;
} vfs_entry_t;

/* Current working directory context */
typedef struct {
    uint32_t cluster;             /* Current directory cluster */
    char path[VFS_MAX_PATH];      /* Current path */
} vfs_cwd_t;

/* Initialize VFS */
void vfs_init(void);

/* Get current working directory */
vfs_cwd_t* vfs_get_cwd(void);

/* Set current working directory */
int vfs_cd(const char *path);

/* List directory contents */
int vfs_ls(const char *path);

/* Create directory */
int vfs_mkdir(const char *path);

/* Create file */
int vfs_create_file(const char *path);

/* Delete file or directory */
int vfs_delete(const char *path);

/* Copy file */
int vfs_copy(const char *src, const char *dst);

/* Find file with path resolution */
int vfs_find(const char *path, vfs_entry_t *out_entry);

/* Get absolute path from relative/absolute path */
void vfs_resolve_path(const char *relpath, char *abspath);

/* Get parent directory path */
void vfs_get_parent_path(const char *path, char *parent);

/* Get filename from path */
const char* vfs_get_filename(const char *path);

/* Check if path is absolute */
int vfs_is_absolute(const char *path);

#endif
