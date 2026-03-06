#include "vfs.h"
#include "fat32.h"
#include "../drivers/vga.h"

static vfs_cwd_t vfs_cwd;

static void strcpy(char *dst, const char *src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

static int strcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return *a - *b;
}

static int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

static void strcat(char *dst, const char *src) {
    while (*dst) dst++;
    strcpy(dst, src);
}

static void normalize_path(const char *path, char *normalized) {
    if (strcmp(path, "/") == 0) {
        strcpy(normalized, "/");
        return;
    }
    
    char temp[VFS_MAX_PATH];
    strcpy(temp, "");
    
    const char *p = path;
    
    if (*p == '/') p++;
    
    while (*p) {
        char component[256];
        int i = 0;
        
        while (*p && *p != '/') {
            component[i++] = *p++;
        }
        component[i] = 0;
        
        if (*p == '/') p++;
        
        if (strcmp(component, ".") == 0) {
            continue;
        }
        
        if (strcmp(component, "..") == 0) {
            int len = strlen(temp);
            if (len > 1) {
                for (int j = len - 1; j >= 0; j--) {
                    if (temp[j] == '/') {
                        temp[j] = 0;
                        break;
                    }
                }
            }
            continue;
        }
        
        if (strlen(temp) == 0 || temp[strlen(temp) - 1] != '/') {
            strcat(temp, "/");
        }
        strcat(temp, component);
    }
    
    if (strlen(temp) == 0) {
        strcpy(normalized, "/");
    } else {
        strcpy(normalized, temp);
    }
}

void vfs_init(void) {
    vfs_cwd.cluster = fat32_get_fs()->root_cluster;
    strcpy(vfs_cwd.path, "/");
}

vfs_cwd_t* vfs_get_cwd(void) {
    return &vfs_cwd;
}

int vfs_is_absolute(const char *path) {
    return path[0] == '/';
}

const char* vfs_get_filename(const char *path) {
    const char *filename = path;
    const char *p = path;
    
    while (*p) {
        if (*p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    return filename;
}

void vfs_get_parent_path(const char *path, char *parent) {
    int len = strlen(path);
    
    int last_slash = 0;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }
    
    if (last_slash == 0) {
        strcpy(parent, "/");
    } else {
        for (int i = 0; i < last_slash; i++) {
            parent[i] = path[i];
        }
        parent[last_slash] = 0;
    }
}

void vfs_resolve_path(const char *relpath, char *abspath) {
    if (vfs_is_absolute(relpath)) {
        strcpy(abspath, relpath);
        return;
    }
    
    strcpy(abspath, vfs_cwd.path);
    
    if (abspath[strlen(abspath) - 1] != '/') {
        strcat(abspath, "/");
    }
    
    strcat(abspath, relpath);
}

static int find_in_cluster(uint32_t cluster, const char *name, fat32_dirent_t *out_entry) {
    return fat32_find_file(cluster, name, out_entry);
}

static int navigate_path(const char *path, uint32_t *out_cluster) {
    char abspath[VFS_MAX_PATH];
    vfs_resolve_path(path, abspath);
    
    if (strcmp(abspath, "/") == 0) {
        *out_cluster = fat32_get_fs()->root_cluster;
        return 1;
    }
    
    uint32_t current_cluster = fat32_get_fs()->root_cluster;
    const char *p = abspath;
    
    if (*p == '/') p++;
    
    while (*p) {
        char component[256];
        int i = 0;
        
        while (*p && *p != '/') {
            component[i++] = *p++;
        }
        component[i] = 0;
        
        if (*p == '/') p++;
        
        if (strcmp(component, "..") == 0) {
            fat32_dirent_t entry;
            if (find_in_cluster(current_cluster, "..", &entry)) {
                current_cluster = ((uint32_t)entry.high_cluster << 16) | entry.low_cluster;
            } else {
                current_cluster = fat32_get_fs()->root_cluster;
            }
            continue;
        }
        
        if (strcmp(component, ".") == 0) {
            continue;
        }
        
        fat32_dirent_t entry;
        if (!find_in_cluster(current_cluster, component, &entry)) {
            return 0;
        }
   
        if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
            return 0;
        }
        
        current_cluster = ((uint32_t)entry.high_cluster << 16) | entry.low_cluster;
    }
    
    *out_cluster = current_cluster;
    return 1;
}

int vfs_ls(const char *path) {
    uint32_t cluster;
    
    if (!navigate_path(path, &cluster)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Directory not found\n", color);
        return 0;
    }
    
    int color = vga_color(VGA_WHITE, VGA_BLUE);
    if (strcmp(path, "/") == 0) {
        print_msg("root/\n", color);
    } else {
        print_msg("Files:\n", color);
    }
    fat32_list_dir(cluster);
    
    return 1;
}

int vfs_mkdir(const char *path) {
    char parent_path[VFS_MAX_PATH];
    const char *dirname;
    uint32_t parent_cluster;
    
    vfs_resolve_path(path, parent_path);
    dirname = vfs_get_filename(parent_path);
    
    vfs_get_parent_path(parent_path, parent_path);
    
    if (!navigate_path(parent_path, &parent_cluster)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Parent directory not found\n", color);
        return 0;
    }
    
    if (fat32_mkdir(parent_cluster, dirname)) {
        int color = vga_color(VGA_LIGHT_GREEN, VGA_BLUE);
        print_msg("Directory created: ", color);
        print_msg((char *)dirname, color);
        print_msg("\n", color);
        return 1;
    } else {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Could not create directory\n", color);
        return 0;
    }
}

int vfs_create_file(const char *path) {
    char parent_path[VFS_MAX_PATH];
    const char *filename;
    uint32_t parent_cluster;
    
    vfs_resolve_path(path, parent_path);
    filename = vfs_get_filename(parent_path);
    
    vfs_get_parent_path(parent_path, parent_path);
    
    if (!navigate_path(parent_path, &parent_cluster)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Parent directory not found\n", color);
        return 0;
    }
    
    if (fat32_create_file(parent_cluster, filename)) {
        int color = vga_color(VGA_LIGHT_GREEN, VGA_BLUE);
        print_msg("File created: ", color);
        print_msg((char *)filename, color);
        print_msg("\n", color);
        return 1;
    } else {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Could not create file\n", color);
        return 0;
    }
}

int vfs_delete(const char *path) {
    char target_path[VFS_MAX_PATH];
    const char *name;
    uint32_t parent_cluster;
    
    vfs_resolve_path(path, target_path);
    name = vfs_get_filename(target_path);
    
    vfs_get_parent_path(target_path, target_path);
    
    if (!navigate_path(target_path, &parent_cluster)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Parent directory not found\n", color);
        return 0;
    }
    
    if (fat32_delete(parent_cluster, name)) {
        int color = vga_color(VGA_LIGHT_GREEN, VGA_BLUE);
        print_msg("Deleted: ", color);
        print_msg((char *)name, color);
        print_msg("\n", color);
        return 1;
    } else {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Could not delete\n", color);
        return 0;
    }
}

int vfs_copy(const char *src, const char *dst) {
    int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
    print_msg("Copy not yet implemented\n", color);
    (void)src;
    (void)dst;
    return 0;
}

int vfs_find(const char *path, vfs_entry_t *out_entry) {
    uint32_t target_cluster;
    char target_path[VFS_MAX_PATH];
    const char *name;
    
    vfs_resolve_path(path, target_path);
    name = vfs_get_filename(target_path);
    
    vfs_get_parent_path(target_path, target_path);
    
    if (!navigate_path(target_path, &target_cluster)) {
        return 0;
    }
    
    fat32_dirent_t entry;
    if (!find_in_cluster(target_cluster, name, &entry)) {
        return 0;
    }
    
    out_entry->cluster = ((uint32_t)entry.high_cluster << 16) | entry.low_cluster;
    out_entry->size = entry.file_size;
    out_entry->is_dir = (entry.attributes & FAT32_ATTR_DIRECTORY) != 0;
    out_entry->is_readonly = (entry.attributes & FAT32_ATTR_READ_ONLY) != 0;
    
    return 1;
}

int vfs_cd(const char *path) {
    uint32_t new_cluster;
    char new_path[VFS_MAX_PATH];
    char normalized_path[VFS_MAX_PATH];
    
    vfs_resolve_path(path, new_path);
    
    if (!navigate_path(new_path, &new_cluster)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("Error: Directory not found\n", color);
        return 0;
    }
    
    normalize_path(new_path, normalized_path);
    
    vfs_cwd.cluster = new_cluster;
    strcpy(vfs_cwd.path, normalized_path);
    
    return 1;
}
