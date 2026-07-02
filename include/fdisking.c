#include "fdisking.h"
#include "fs/vfs.h"

void init_mbr(uint8_t drive_id)
{
    uint16_t buffer[256] = {0};
    buffer[255] = 0xAA55; // tell this is the MBR
    ata_write_sector(drive_id, 0, buffer);
}

void create_partition_mbr(uint8_t drive_id, uint8_t slot, uint32_t start_lba, uint32_t sector_count)
{
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
    partitionid_t p;
    p.drive_id = drive_id;
    p.partition = slot;

    printf("Formatting Partition d%ds%d (Start LBA: %d, Sectors: %d)...\n", drive_id, slot, start_lba, part->sector_count); // this printf was ai because I'm lazy to write
    
    if (fs == fat16)
    {
        format_partition_fat16(p, start_lba, part);
    }
    else if (fs == ext2)
    {
        format_partition_ext2(p, start_lba, part);
    }
}

void list_root_dir(partitionid_t part, struct file *fbuffer, enum filesystem fs)
{
    if (fs == ext2)
    {
        list_root_dir_ext2(part, fbuffer);
    }
}
