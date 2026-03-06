#include "fat32.h"
#include "ata.h"
#include "../drivers/vga.h"

static fat32_fs_t fat32_fs;
static uint8_t fat32_buffer[512];  

static int strcmp_83(const char *a, const char *b) {
    while (*a && *b) {
        char ac = *a;
        char bc = *b;
        if (ac >= 'a' && ac <= 'z') ac -= 32;
        if (bc >= 'a' && bc <= 'z') bc -= 32;
        if (ac != bc) return ac - bc;
        a++;
        b++;
    }
    return *a - *b;
}

static int is_eoc(uint32_t cluster) {
    return cluster >= 0x0FFFFFF8;
}

int fat32_init(void) {
    return fat32_read_boot_sector();
}

int fat32_read_boot_sector(void) {
    uint8_t buffer[512];
    
    if (!ata_read_sector(0, buffer)) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("FAT32: Failed to read boot sector\n", color);
        return 0;
    }
    
    fat32_boot_sector_t *boot = (fat32_boot_sector_t *)buffer;
    
    if (boot->bytes_per_sect != 512) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("FAT32: Invalid sector size\n", color);
        return 0;
    }
    
    fat32_fs.boot_sector = *boot;
    fat32_fs.bytes_per_sector = boot->bytes_per_sect;
    fat32_fs.sectors_per_cluster = boot->sects_per_cluster;
    fat32_fs.root_cluster = boot->root_cluster;
    
    // FAT32 root cluster is typically 2
    if (fat32_fs.root_cluster == 0) {
        fat32_fs.root_cluster = 2;
    }
    
    fat32_fs.fat_start_sector = boot->reserved_sects;
    fat32_fs.data_start_sector = boot->reserved_sects + 
                                  (boot->num_fats * boot->sects_per_fat);
    
    int color = vga_color(VGA_LIGHT_GREEN, VGA_BLUE);
    print_msg("FAT32: Initialized successfully\n", color);
    
    return 1;
}

uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat32_fs.fat_start_sector + (fat_offset / 512);
    uint32_t offset_in_sector = fat_offset % 512;
    
    if (!ata_read_sector(fat_sector, fat32_buffer)) {
        return 0xFFFFFFFF;
    }
    
    uint32_t next_cluster = *(uint32_t *)(fat32_buffer + offset_in_sector);
    return next_cluster & 0x0FFFFFFF; 
}

static int read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((cluster - 2) * fat32_fs.sectors_per_cluster);
    
    for (int i = 0; i < fat32_fs.sectors_per_cluster; i++) {
        if (!ata_read_sector(sector + i, buffer + (i * 512))) {
            return 0;
        }
    }
    
    return 1;
}

int fat32_list_dir(uint32_t cluster) {
    uint32_t current_cluster = cluster;
    int found = 0;
    
    int color = vga_color(VGA_WHITE, VGA_BLUE);
    
    while (!is_eoc(current_cluster)) {
        uint8_t cluster_data[4096]; 
        
        if (!read_cluster(current_cluster, cluster_data)) {
            break;
        }
        
        fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
        int entries_per_cluster = (fat32_fs.sectors_per_cluster * 512) / 32;
        
        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dirent_t *entry = &entries[i];
            
            if (entry->filename[0] == 0x00) {
                return found;
            }
            
            if (entry->filename[0] == 0xE5 || entry->attributes == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            if (entry->attributes & FAT32_ATTR_VOLUME_ID) {
                continue;
            }
            
            // Skip . and .. entries
            if (entry->filename[0] == '.') {
                continue;
            }
            
            // Only display valid files and directories (skip garbage data)
            if (!(entry->attributes & (FAT32_ATTR_DIRECTORY | FAT32_ATTR_ARCHIVE))) {
                continue;
            }
            
            found++;
            
            print_msg("+-- ", color);
            
            int has_valid_chars = 0;
            for (int j = 0; j < 8 && entry->filename[j] != ' '; j++) {
                // Only print valid ASCII characters
                if (entry->filename[j] >= 32 && entry->filename[j] < 127) {
                    print_msg_char(entry->filename[j], color);
                    has_valid_chars = 1;
                }
            }
            
            // If we didn't print anything, skip this entry as it's invalid
            if (!has_valid_chars) {
                found--;
                continue;
            }
            
            if (entry->extension[0] != ' ') {
                print_msg(".", color);
                for (int j = 0; j < 3 && entry->extension[j] != ' '; j++) {
                    // Only print valid ASCII characters
                    if (entry->extension[j] >= 32 && entry->extension[j] < 127) {
                        print_msg_char(entry->extension[j], color);
                    }
                }
            }
            
            if (entry->attributes & FAT32_ATTR_DIRECTORY) {
                print_msg("/", color);
            }
            
            print_msg("\n", color);
        }
        
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    return found;
}

int fat32_find_file(uint32_t dir_cluster, const char *filename, fat32_dirent_t *out_entry) {
    uint32_t current_cluster = dir_cluster;
    
    const char *dot = filename;
    int fname_len = 0;
    while (*dot && *dot != '.') {
        fname_len++;
        dot++;
    }
    
    while (!is_eoc(current_cluster)) {
        uint8_t cluster_data[4096];
        
        if (!read_cluster(current_cluster, cluster_data)) {
            break;
        }
        
        fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
        int entries_per_cluster = (fat32_fs.sectors_per_cluster * 512) / 32;
        
        for (int i = 0; i < entries_per_cluster; i++) {
            fat32_dirent_t *entry = &entries[i];
            
            if (entry->filename[0] == 0x00) {
                return 0;
            }
            
            if (entry->filename[0] == 0xE5 || entry->attributes == FAT32_ATTR_LONG_NAME) {
                continue;
            }
            
            if (entry->attributes & FAT32_ATTR_VOLUME_ID) {
                continue;
            }
            
            char entry_name[13];
            int j = 0;
            for (int k = 0; k < 8 && entry->filename[k] != ' '; k++) {
                entry_name[j++] = entry->filename[k];
            }
            
            if (entry->extension[0] != ' ') {
                entry_name[j++] = '.';
                for (int k = 0; k < 3 && entry->extension[k] != ' '; k++) {
                    entry_name[j++] = entry->extension[k];
                }
            }
            entry_name[j] = 0;
            
            if (strcmp_83(entry_name, filename) == 0) {
                *out_entry = *entry;
                return 1;  
            }
        }
        
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    return 0;
}

int fat32_open(const char *filename, fat32_file_t *file) {
    fat32_dirent_t entry;
    
    if (!fat32_find_file(fat32_fs.root_cluster, filename, &entry)) {
        return 0;
    }
    
    if (entry.attributes & FAT32_ATTR_DIRECTORY) {
        return 0;
    }
    
    file->cluster = ((uint32_t)entry.high_cluster << 16) | entry.low_cluster;
    file->position = 0;
    file->file_size = entry.file_size;
    file->cluster_pos = 0;
    
    return 1;
}

int fat32_read(fat32_file_t *file, uint8_t *buffer, uint32_t size) {
    uint32_t bytes_read = 0;
    uint8_t cluster_data[4096];
    
    while (bytes_read < size && file->position < file->file_size) {
        if (file->cluster_pos == 0) {
            if (is_eoc(file->cluster)) {
                break;
            }
            
            if (!read_cluster(file->cluster, cluster_data)) {
                break;
            }
            
            file->cluster = fat32_get_next_cluster(file->cluster);
        }
        
        uint32_t offset = file->cluster_pos;
        uint32_t cluster_size = fat32_fs.sectors_per_cluster * 512;
        uint32_t bytes_in_cluster = cluster_size - offset;
        uint32_t to_copy = bytes_in_cluster;
        
        if (to_copy > (size - bytes_read)) {
            to_copy = size - bytes_read;
        }
        
        if (to_copy > (file->file_size - file->position)) {
            to_copy = file->file_size - file->position;
        }
        
        for (uint32_t i = 0; i < to_copy; i++) {
            buffer[bytes_read + i] = cluster_data[offset + i];
        }
        
        bytes_read += to_copy;
        file->position += to_copy;
        file->cluster_pos += to_copy;
        
        if (file->cluster_pos >= cluster_size) {
            file->cluster_pos = 0;
        }
    }
    
    return bytes_read;
}

void fat32_close(fat32_file_t *file) {
    (void)file; 
}

uint32_t fat32_alloc_cluster(void) {
    uint32_t fat_size_bytes = fat32_fs.boot_sector.sects_per_fat * 512;
    uint32_t num_clusters = fat_size_bytes / 4;
    
    for (uint32_t cluster = 2; cluster < num_clusters; cluster++) {
        uint32_t next = fat32_get_next_cluster(cluster);
        if (next == 0) { 
            return cluster;
        }
    }
    
    return 0; 
}

int fat32_update_fat_entry(uint32_t cluster, uint32_t next) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat32_fs.fat_start_sector + (fat_offset / 512);
    uint32_t offset_in_sector = fat_offset % 512;
    
    if (!ata_read_sector(fat_sector, fat32_buffer)) {
        return 0;
    }
    
    *(uint32_t *)(fat32_buffer + offset_in_sector) = next & 0x0FFFFFFF;
    
    for (int fat_num = 0; fat_num < fat32_fs.boot_sector.num_fats; fat_num++) {
        uint32_t write_sector = fat_sector + (fat_num * fat32_fs.boot_sector.sects_per_fat);
        if (!ata_write_sectors(write_sector, 1, fat32_buffer)) {
            return 0;
        }
    }
    
    return 1;
}

int fat32_write_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t sector = fat32_fs.data_start_sector + 
                     ((cluster - 2) * fat32_fs.sectors_per_cluster);
    
    for (int i = 0; i < fat32_fs.sectors_per_cluster; i++) {
        if (!ata_write_sectors(sector + i, 1, buffer + (i * 512))) {
            return 0;
        }
    }
    
    return 1;
}

int fat32_write_dirent(uint32_t cluster, uint32_t offset, fat32_dirent_t *entry) {
    uint8_t cluster_data[4096];
    
    if (!read_cluster(cluster, cluster_data)) {
        return 0;
    }
    
    fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
    entries[offset] = *entry;
    
    if (!fat32_write_cluster(cluster, cluster_data)) {
        return 0;
    }
    
    return 1;
}

static int create_dirent(uint32_t parent_cluster, const char *name, uint8_t attr, uint32_t start_cluster) {
    uint8_t cluster_data[4096];
    uint32_t current_cluster = parent_cluster;
    
    while (!is_eoc(current_cluster)) {
        if (!read_cluster(current_cluster, cluster_data)) {
            return 0;
        }
        
        fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
        int entries_per_cluster = (fat32_fs.sectors_per_cluster * 512) / 32;
        
        for (int i = 0; i < entries_per_cluster; i++) {
            if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {
                fat32_dirent_t new_entry = {};
                
                const char *dot = name;
                int name_len = 0;
                while (*dot && *dot != '.') {
                    name_len++;
                    dot++;
                }
                
                for (int j = 0; j < 8; j++) {
                    if (j < name_len) {
                        new_entry.filename[j] = name[j];
                    } else {
                        new_entry.filename[j] = ' ';
                    }
                }
                
                if (*dot == '.') {
                    dot++;
                    for (int j = 0; j < 3; j++) {
                        if (*dot && j < 3) {
                            new_entry.extension[j] = *dot++;
                        } else {
                            new_entry.extension[j] = ' ';
                        }
                    }
                }
                
                new_entry.attributes = attr;
                new_entry.reserved = 0;
                new_entry.creation_time_tenths = 0;
                new_entry.low_cluster = (uint16_t)(start_cluster & 0xFFFF);
                new_entry.high_cluster = (uint16_t)((start_cluster >> 16) & 0xFFFF);
                new_entry.file_size = 0;
                new_entry.access_date = 0x0021;
                new_entry.creation_date = 0x0021;  
                new_entry.creation_time = 0x0000;
                new_entry.write_date = 0x0021;
                new_entry.write_time = 0x0000;
                
                entries[i] = new_entry;
                
                int write_result = fat32_write_cluster(current_cluster, cluster_data);
                if (!write_result) {
                    int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
                    print_msg("FAT32: ERROR - Failed to write cluster\n", color);
                    return 0;
                }
                
                return 1;
            }
        }
        
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    return 0;
}

int fat32_mkdir(uint32_t parent_cluster, const char *dirname) {
    uint32_t new_cluster = fat32_alloc_cluster();
    if (new_cluster == 0) {
        int color = vga_color(VGA_LIGHT_RED, VGA_BLUE);
        print_msg("FAT32: No free clusters\n", color);
        return 0;
    }
    
    if (!fat32_update_fat_entry(new_cluster, 0x0FFFFFFF)) {
        return 0;
    }
    
    uint8_t cluster_data[4096] = {0};
    
    fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
    for (int i = 0; i < 8; i++) {
        entries[0].filename[i] = (i == 0) ? '.' : ' ';
    }
    for (int i = 0; i < 3; i++) {
        entries[0].extension[i] = ' ';
    }
    entries[0].attributes = FAT32_ATTR_DIRECTORY;
    entries[0].low_cluster = (uint16_t)(new_cluster & 0xFFFF);
    entries[0].high_cluster = (uint16_t)((new_cluster >> 16) & 0xFFFF);
    
    for (int i = 0; i < 8; i++) {
        entries[1].filename[i] = (i < 2) ? '.' : ' ';
    }
    for (int i = 0; i < 3; i++) {
        entries[1].extension[i] = ' ';
    }
    entries[1].attributes = FAT32_ATTR_DIRECTORY;
    entries[1].low_cluster = (uint16_t)(parent_cluster & 0xFFFF);
    entries[1].high_cluster = (uint16_t)((parent_cluster >> 16) & 0xFFFF);
    
    if (!fat32_write_cluster(new_cluster, cluster_data)) {
        return 0;
    }
    
    return create_dirent(parent_cluster, dirname, FAT32_ATTR_DIRECTORY, new_cluster);
}

int fat32_create_file(uint32_t parent_cluster, const char *filename) {
    return create_dirent(parent_cluster, filename, FAT32_ATTR_ARCHIVE, 0);
}

int fat32_delete(uint32_t parent_cluster, const char *name) {
    uint32_t current_cluster = parent_cluster;
    
    while (!is_eoc(current_cluster)) {
        uint8_t cluster_data[4096];
        
        if (!read_cluster(current_cluster, cluster_data)) {
            break;
        }
        
        fat32_dirent_t *entries = (fat32_dirent_t *)cluster_data;
        int entries_per_cluster = (fat32_fs.sectors_per_cluster * 512) / 32;
        
        for (int i = 0; i < entries_per_cluster; i++) {
            if (entries[i].filename[0] == 0x00) {
                return 0; 
            }
            
            if (entries[i].filename[0] == 0xE5) {
                continue;  
            }
            
            char entry_name[13];
            int j = 0;
            for (int k = 0; k < 8 && entries[i].filename[k] != ' '; k++) {
                entry_name[j++] = entries[i].filename[k];
            }
            if (entries[i].extension[0] != ' ') {
                entry_name[j++] = '.';
                for (int k = 0; k < 3 && entries[i].extension[k] != ' '; k++) {
                    entry_name[j++] = entries[i].extension[k];
                }
            }
            entry_name[j] = 0;
            
            if (strcmp_83(entry_name, name) == 0) {
                entries[i].filename[0] = 0xE5;
                
                if (!fat32_write_cluster(current_cluster, cluster_data)) {
                    return 0;
                }
                
                return 1;
            }
        }
        
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    return 0;
}

fat32_fs_t* fat32_get_fs(void) {
    return &fat32_fs;
}
