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

struct FAT16DirEntry {
    uint8_t filename[11]; // just make this uint8_t
    uint8_t attributes;        // File attributes (0x10 = Directory, 0x20 = Archive)
    uint8_t reserved[10];      // Reserved for WinNT/timestamps
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t starting_cluster; // CRITICAL: This is what read_file will need!
    uint32_t file_size;        // File size in bytes
} __attribute__((packed));

struct MBR {
    uint8_t boot_code[446];
    struct MBRPartition partitions[4];
    uint16_t signature;
} __attribute__((packed));

typedef struct {
    uint8_t drive_id;
    uint8_t partition; // aka slot on MBR
} partitionid_t;

enum filesystem {
    fat16 // currently only this
};

void init_mbr(uint8_t drive_id)
{
    uint16_t buffer[256] = {0};
    buffer[255] = 0xAA55; // tell this is the MBR
    ata_write_sector(drive_id, 0, buffer);
}

void create_partition_mbr(uint8_t drive_id, uint8_t slot, uint32_t start_lba, uint32_t sector_count) {
    // Basic guard rails
    if (slot > 3) {
        printf("Error: MBR only supports partition slots 0 to 3.\n");
        return;
    }

    uint16_t disk_buffer[256];
    
    // 1. Read the current state of the MBR (so we don't overwrite existing partitions!)
    ata_read_sector(drive_id, 0, disk_buffer);
    
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
    ata_write_sector(drive_id, 0, disk_buffer);
    printf("Partition d%ds%d created: Start LBA %d, Sectors: %d\n", drive_id, slot, start_lba, sector_count);
}

int detect_architect(uint8_t drive_id)
{
    uint16_t buffer[256];
    ata_read_sector(drive_id, 0, buffer);
    if (buffer[255] == 0xAA55)
        return 0; // MBR
    return -1; // no architecture
}


void format_partition_mbr(uint8_t drive_id, uint8_t slot, enum filesystem fs)
{
    uint16_t buffer[256];
    ata_read_sector(drive_id, 0, buffer); // WHERE IS THAT PARTITION??
    uint32_t buffer_index = 223 + (slot * 8);
    struct MBRPartition *part = (struct MBRPartition *)&buffer[buffer_index];

    if (part->sector_count == 0 || part->partition_type == 0)
    {
        printf("PARTITION on slot %d EMPTY", slot);
        return;
    }

    uint32_t start_lba = part->lba_start;
    uint32_t total_sectors = part->sector_count;

    printf("Formatting Partition d%ds%d (Start LBA: %d, Sectors: %d) to FAT16...\n", drive_id, slot, start_lba, total_sectors); // this printf was ai because I'm lazy to write
    
    if (fs == fat16)
    {
        struct fat16_bpb bpb;
        bpb.oem_name[0] = 'M';
        bpb.oem_name[1] = 'S';
        bpb.oem_name[2] = 'W';
        bpb.oem_name[3] = 'I';
        bpb.oem_name[4] = 'N';
        bpb.oem_name[5] = '4';
        bpb.oem_name[6] = '.';
        bpb.oem_name[7] = '1';

        bpb.bytes_per_sector = 512; // hardcoded
        bpb.sectors_per_cluster = 4; // hardcoded too because I want conviniency
        bpb.reserved_sectors = 1; // on FAT16, it's always one
        bpb.num_fats = 2; // redudancy
        bpb.root_dir_entries = 512; // compatibility, it's 512
        
        uint32_t net_sectors = total_sectors - 1 - 32;
        uint32_t numerator = 2 * (net_sectors / 4);
        uint32_t denominator = (512 / 2) + (2 / 4);
        bpb.sectors_per_fat = (numerator + denominator - 1) / denominator;

        bpb.total_sectors_short = total_sectors;

        bpb.media_type = 0xF8; // because it's a hard drive
			      
        bpb.jmp[0] = 0xEB; // jmp 0x3C
        bpb.jmp[1] = 0x3C;
        bpb.jmp[2] = 0x90;

        uint16_t sector_buffer[256] = {0}; // 512 bytes total, all zeroed out

        // Copy your stack-allocated struct into the very front of the buffer
        memcpy(sector_buffer, &bpb, sizeof(struct fat16_bpb));

        // Assign the final magic boot signature (0xAA55) to the last 2 bytes
        sector_buffer[255] = 0xAA55;

        // Commit it to disk!
        ata_write_sector(drive_id, start_lba, sector_buffer);
    }
}

void list_dir_fat16(partitionid_t part)
{
    uint16_t pbuffer[256];
    ata_read_sector(part.drive_id, 0, pbuffer);

    uint32_t buffer_index = 223 + (part.partition * 8);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&pbuffer[buffer_index];

    uint16_t buffer[256];
    ata_read_sector(part.drive_id, mbrpart->lba_start, buffer);

    struct fat16_bpb *bpb = (struct fat16_bpb *)buffer;
    uint32_t root_dir_lba = mbrpart->lba_start + bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat);
    
    // We calculate how many total sectors the root directory takes up
    // (Usually 512 entries * 32 bytes per entry = 16384 bytes -> 32 sectors)
    uint32_t root_dir_sectors = (bpb->root_dir_entries * 32) / 512;

    uint8_t dir_buffer[512];
    int end_of_directory = 0;
    printf("DEBUG: Part Start: %d\n", mbrpart->lba_start);
    printf("DEBUG: Reserved Sec: %d\n", bpb->reserved_sectors);
    printf("DEBUG: Num FATs: %d\n", bpb->num_fats);
    printf("DEBUG: Sec Per FAT: %d\n", bpb->sectors_per_fat);
    printf("DEBUG: Target Root LBA: %d\n", root_dir_lba);

    // OUTER LOOP: Step through each sector of the root directory region
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        
        // Read the current root directory sector
        ata_read_sector(part.drive_id, root_dir_lba + i, (uint16_t *)dir_buffer);
        
        for (int j = 0;j < 16;j++)
        {
            //printf("entry %d sector %d\n", j, i);
            struct FAT16DirEntry *entry = (struct FAT16DirEntry *)&dir_buffer[j*32];

            if (entry->filename[0]==0)
            {
                end_of_directory = 1;
                break;
            }

            if (entry->filename[0] == 0xE5)
                continue;

            if (entry->attributes == 0x0F)
                continue; // ignore LFN records

            //ummm now just print right
            char safe_fn[12];
            int i = 0;
            while (i < 12)
            {
                safe_fn[i] = (char)entry->filename[i];
                i++;
            }
            safe_fn[i] = '\0';

            printf("file %s\n", safe_fn);
        }
        
        // Break out of the outer sector loop if the inner loop hit 0x00
        if (end_of_directory) {
            break;
        }
    }
}

partitionid_t str_partid(char *str)
{
    partitionid_t part;
    part.drive_id = 0xFF;
    part.partition = 0xFF;
    if (str[0] == 'd') // ensure correct format
    {
        char buff[10];
        int i = 0;
        while ((str[i+1] >= '0' && str[i+1] <= '9') && i < 9) // get numbers
        {
            buff[i] = str[i+1];
            i++;
        }
        buff[i] = '\0';
        int drive_id = atoi(buff);
        if (str[i+1] == 's')
        {
            char buff[10];
            int j = 0;
            while ((str[j+i+1] >= '0' && str[j+i+1] <= '9') && j < 9)
            {
                buff[j] = str[j+i+1];
                j++;
            }
            buff[j] = '\0';
            int partnum = atoi(buff); // aka slot
            part.drive_id = drive_id;
            part.partition = partnum;
        }
    }
    return part;
}

#endif
