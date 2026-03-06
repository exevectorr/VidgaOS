#include "ata.h"
#include "io.h"

/* Poll until status bit is set */
static uint8_t ata_poll_status(uint8_t mask) {
    uint8_t status;
    int timeout = 100000;
    
    do {
        status = inb(ATA_PRIMARY_STATUS);
        timeout--;
    } while ((status & mask) == 0 && timeout > 0);
    
    return status;
}

/* Wait for drive to be ready */
static int ata_wait_ready(void) {
    uint8_t status = ata_poll_status(ATA_STATUS_READY);
    return !(status & ATA_STATUS_ERROR);
}

/* Initialize ATA controller */
void ata_init(void) {
    /* Soft reset on control port */
    outb(0x1F6, 0xE0);  /* Master drive, LBA mode */
    
    /* Wait for drive to be ready */
    ata_wait_ready();
}

/* Read single sector from disk */
int ata_read_sector(uint32_t lba, uint8_t *buffer) {
    return ata_read_sectors(lba, 1, buffer);
}

/* Read multiple sectors from disk using LBA */
int ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    /* Wait for drive to be ready */
    if (!ata_wait_ready()) {
        return 0;
    }
    
    /* Set number of sectors to read */
    outb(ATA_PRIMARY_SECTOR_CNT, count);
    
    /* Set LBA address (28-bit mode) */
    outb(ATA_PRIMARY_SECTOR_NUM, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_CYLINDER_L, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_CYLINDER_H, (uint8_t)((lba >> 16) & 0xFF));
    
    /* Set drive and mode (LBA mode, master) */
    uint8_t drive_byte = 0xE0 | ((lba >> 24) & 0x0F);
    outb(ATA_PRIMARY_DRIVE_HEAD, drive_byte);
    
    /* Send read command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);
    
    /* Read all sectors */
    for (int sec = 0; sec < count; sec++) {
        /* Wait for data ready */
        uint8_t status = ata_poll_status(ATA_STATUS_DATA_READY);
        
        if (status & ATA_STATUS_ERROR) {
            return 0;
        }
        
        /* Read 256 words (512 bytes) per sector */
        for (int i = 0; i < 256; i++) {
            uint16_t word = inw(ATA_PRIMARY_DATA);
            buffer[sec * 512 + i * 2] = (uint8_t)(word & 0xFF);
            buffer[sec * 512 + i * 2 + 1] = (uint8_t)((word >> 8) & 0xFF);
        }
    }
    
    return 1;
}

/* Write multiple sectors to disk */
int ata_write_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    /* Wait for drive to be ready */
    if (!ata_wait_ready()) {
        return 0;
    }
    
    /* Set number of sectors to write */
    outb(ATA_PRIMARY_SECTOR_CNT, count);
    
    /* Set LBA address */
    outb(ATA_PRIMARY_SECTOR_NUM, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_CYLINDER_L, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_CYLINDER_H, (uint8_t)((lba >> 16) & 0xFF));
    
    /* Set drive and mode (LBA mode, master) */
    uint8_t drive_byte = 0xE0 | ((lba >> 24) & 0x0F);
    outb(ATA_PRIMARY_DRIVE_HEAD, drive_byte);
    
    /* Send write command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    /* Write all sectors */
    for (int sec = 0; sec < count; sec++) {
        /* Wait for ready to accept data */
        uint8_t status = ata_poll_status(ATA_STATUS_DATA_READY);
        
        if (status & ATA_STATUS_ERROR) {
            return 0;
        }
        
        /* Write 256 words (512 bytes) per sector */
        for (int i = 0; i < 256; i++) {
            uint16_t word = buffer[sec * 512 + i * 2] | 
                           ((uint16_t)buffer[sec * 512 + i * 2 + 1] << 8);
            outw(ATA_PRIMARY_DATA, word);
        }
    }
    
    /* Wait for write to complete */
    ata_wait_ready();
    
    return 1;
}
