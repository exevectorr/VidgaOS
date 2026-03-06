#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

/* FAT32 Boot Sector Structure */
typedef struct {
    uint8_t  jmp[3];              /* 0x00: Jump instruction */
    uint8_t  oem[8];              /* 0x03: OEM name */
    uint16_t bytes_per_sect;      /* 0x0B: Bytes per sector (512) */
    uint8_t  sects_per_cluster;   /* 0x0D: Sectors per cluster */
    uint16_t reserved_sects;      /* 0x0E: Reserved sectors */
    uint8_t  num_fats;            /* 0x10: Number of FAT tables */
    uint16_t max_root_entries;    /* 0x11: Max root entries (0 for FAT32) */
    uint16_t total_sects_16;      /* 0x13: Total sectors 16-bit (0 for FAT32) */
    uint8_t  media_desc;          /* 0x15: Media descriptor */
    uint16_t sects_per_fat_16;    /* 0x16: Sectors per FAT 16-bit (0 for FAT32) */
    uint16_t sects_per_track;     /* 0x18: Sectors per track */
    uint16_t num_heads;           /* 0x1A: Number of heads */
    uint32_t hidden_sects;        /* 0x1C: Hidden sectors */
    uint32_t total_sects_32;      /* 0x20: Total sectors 32-bit */
    uint32_t sects_per_fat;       /* 0x24: Sectors per FAT 32-bit */
    uint16_t ext_flags;           /* 0x28: Extended flags */
    uint16_t fs_version;          /* 0x2A: Filesystem version */
    uint32_t root_cluster;        /* 0x2C: Root directory cluster */
    uint16_t fsinfo_sector;       /* 0x30: FSInfo sector number */
    uint16_t backup_boot_sect;    /* 0x32: Backup boot sector */
    uint8_t  reserved[12];        /* 0x34: Reserved */
    uint8_t  drive_num;           /* 0x40: Drive number */
    uint8_t  reserved_1;          /* 0x41: Reserved */
    uint8_t  boot_sig;            /* 0x42: Boot signature */
    uint32_t volume_id;           /* 0x43: Volume ID */
    uint8_t  volume_label[11];    /* 0x47: Volume label */
    uint8_t  fs_type[8];          /* 0x52: Filesystem type */
} __attribute__((packed)) fat32_boot_sector_t;

/* FAT32 Directory Entry Structure */
typedef struct {
    uint8_t  filename[8];         /* Short filename */
    uint8_t  extension[3];        /* File extension */
    uint8_t  attributes;          /* File attributes */
    uint8_t  reserved;            /* Reserved */
    uint8_t  creation_time_tenths;/* Creation time (10ths of second) */
    uint16_t creation_time;       /* Creation time */
    uint16_t creation_date;       /* Creation date */
    uint16_t access_date;         /* Last access date */
    uint16_t high_cluster;        /* High 16 bits of cluster */
    uint16_t write_time;          /* Last write time */
    uint16_t write_date;          /* Last write date */
    uint16_t low_cluster;         /* Low 16 bits of cluster */
    uint32_t file_size;           /* File size in bytes */
} __attribute__((packed)) fat32_dirent_t;

/* File attributes */
#define FAT32_ATTR_READ_ONLY    0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

/* FAT32 Filesystem context */
typedef struct {
    fat32_boot_sector_t boot_sector;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t root_cluster;
    uint8_t sectors_per_cluster;
    uint32_t bytes_per_sector;
} fat32_fs_t;

/* File handle for reading */
typedef struct {
    uint32_t cluster;             /* Current cluster */
    uint32_t position;            /* Position in file */
    uint32_t file_size;           /* Total file size */
    uint32_t cluster_pos;         /* Position in current cluster */
} fat32_file_t;

/* Initialize FAT32 filesystem */
int fat32_init(void);

/* Read boot sector and verify FAT32 */
int fat32_read_boot_sector(void);

/* Get next cluster in chain */
uint32_t fat32_get_next_cluster(uint32_t cluster);

/* List files in directory */
int fat32_list_dir(uint32_t cluster);

/* Find file in directory */
int fat32_find_file(uint32_t dir_cluster, const char *filename, fat32_dirent_t *out_entry);

/* Open file for reading */
int fat32_open(const char *filename, fat32_file_t *file);

/* Read from file */
int fat32_read(fat32_file_t *file, uint8_t *buffer, uint32_t size);

/* Close file */
void fat32_close(fat32_file_t *file);

/* Create directory */
int fat32_mkdir(uint32_t parent_cluster, const char *dirname);

/* Create file */
int fat32_create_file(uint32_t parent_cluster, const char *filename);

/* Delete file or directory */
int fat32_delete(uint32_t parent_cluster, const char *name);

/* Allocate a new cluster */
uint32_t fat32_alloc_cluster(void);

/* Write data to file/cluster */
int fat32_write_cluster(uint32_t cluster, uint8_t *buffer);

/* Update FAT chain */
int fat32_update_fat_entry(uint32_t cluster, uint32_t next);

/* Write directory entry */
int fat32_write_dirent(uint32_t cluster, uint32_t offset, fat32_dirent_t *entry);

/* Get filesystem info */
fat32_fs_t* fat32_get_fs(void);

#endif
