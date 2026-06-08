#ifndef FDISK_H
#define FDISK_H

#include "atapio.h"

struct fat16_bpb {
    uint8_t jmp[3];
    char     oem_name[8];       // Can be anything, e.g., "MYOWN_OS"
    uint16_t bytes_per_sector;  // Hardcoded to 512
    uint8_t  sectors_per_cluster;// Usually 4 (meaning 1 cluster = 2048 bytes)
    uint16_t reserved_sectors;  // 1 (The sector this BPB lives in)
    uint8_t  num_fats;          // 2 (Standard for redundancy)
    uint16_t root_dir_entries;  // 512 (How many files can fit in the root directory)
    uint16_t total_sectors_short;// 0 if disk is > 32MB
    uint8_t  media_type;        // 0xF8 for hard disks
    uint16_t sectors_per_fat;
} __attribute__((packed));

struct MBRPartition {
    uint8_t  drive_status;   // 0x80 = Active/Bootable, 0x00 = Inactive
    uint8_t  chs_start[3];   // Legacy Cylinder-Head-Sector starting address
    uint8_t  partition_type; // 0x06 or 0x0E for FAT16
    uint8_t  chs_end[3];     // Legacy CHS ending address
    uint32_t lba_start;      // The actual starting sector (e.g., 2048)
    uint32_t sector_count;   // Total number of sectors in this partition
} __attribute__((packed));

void init_mbr()
{
    uint16_t buffer[256] = {0};
    buffer[255] = 0xAA55; // tell this is the MBR
    ata_write_sector(0, buffer);
}

void create_partition_mbr(uint8_t slot, uint32_t start_lba, uint32_t sector_count) {
    // Basic guard rails
    if (slot > 3) {
        printf("Error: MBR only supports partition slots 0 to 3.\n");
        return;
    }

    uint16_t disk_buffer[256];
    
    // 1. Read the current state of the MBR (so we don't overwrite existing partitions!)
    ata_read_sector(0, disk_buffer);
    
    // 2. Calculate the exact array offset for the requested slot
    uint32_t buffer_index = 223 + (slot * 8);
    struct MBRPartition *part = (struct MBRPartition *)&disk_buffer[buffer_index];
    
    // 3. Fill in the definitions for this specific partition
    part->drive_status = (slot == 0) ? 0x80 : 0x00; // Make slot 0 active/bootable, others inactive
    part->partition_type = 0x06;                    // FAT16
    part->lba_start = start_lba;
    part->sector_count = sector_count;
    
    // Clear legacy CHS values
    for (int i = 0; i < 3; i++) {
        part->chs_start[i] = 0;
        part->chs_end[i] = 0;
    }

    // 4. Flush the updated MBR back to Sector 0
    ata_write_sector(0, disk_buffer);
    printf("Partition in slot %d created: Start LBA %d, Sectors: %d\n", slot, start_lba, sector_count);
}

int detect_architect()
{
    uint16_t buffer[256];
    ata_read_sector(0, buffer);
    if (buffer[255] == 0xAA55)
        return 0; // MBR
    return -1; // no architecture
}

#endif
