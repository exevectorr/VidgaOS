#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* ATA Port Addresses */
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERROR       0x1F1
#define ATA_PRIMARY_SECTOR_CNT  0x1F2
#define ATA_PRIMARY_SECTOR_NUM  0x1F3
#define ATA_PRIMARY_CYLINDER_L  0x1F4
#define ATA_PRIMARY_CYLINDER_H  0x1F5
#define ATA_PRIMARY_DRIVE_HEAD  0x1F6
#define ATA_PRIMARY_COMMAND     0x1F7
#define ATA_PRIMARY_STATUS      0x1F7

/* ATA Commands */
#define ATA_CMD_READ_SECTORS    0x20
#define ATA_CMD_WRITE_SECTORS   0x30
#define ATA_CMD_IDENTIFY        0xEC

/* ATA Status Bits */
#define ATA_STATUS_BUSY         0x80
#define ATA_STATUS_READY        0x40
#define ATA_STATUS_ERROR        0x01
#define ATA_STATUS_DATA_READY   0x08

/* Initialize ATA controller */
void ata_init(void);

/* Read sectors from disk */
int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buffer);

/* Write sectors to disk */
int ata_write_sectors(uint32_t lba, uint8_t count, uint8_t *buffer);

/* Low-level disk read (single sector, 512 bytes) */
int ata_read_sector(uint32_t lba, uint8_t *buffer);

#endif
