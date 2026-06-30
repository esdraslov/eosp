#ifndef FDISK_H
#define FDISK_H

#include "atapio.h"
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0X20
#define LFN 0x07

typedef struct {
    uint8_t drive_id;
    uint8_t partition; // aka slot on MBR
} partitionid_t;

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

struct file {
    char filename[64];
    bool isdir;
    uint32_t starting_lba; // READ METADATA
    uint8_t offset;
    partitionid_t partition;
    struct file *parent;
}; // removed packed attribute so some systems don't complain about it

struct MBR {
    uint8_t boot_code[446];
    struct MBRPartition partitions[4];
    uint16_t signature;
} __attribute__((packed));

struct partition_info {
    uint8_t drive_id;
    uint32_t partition_index;
    uint32_t lba_start;         // Cached from MBR
    
    // Cached filesystem metadata so you never have to re-read the BPB sector!
    uint32_t data_region_start; // Your 'drs_lba'
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint32_t sectors_per_fat;
    uint8_t num_fats;
};

enum filesystem {
    fat16 // currently only this
};

struct partition_info bpb_cache[4];

uint32_t get_next_cluster_fat(uint16_t current_cluster, uint32_t fat_slba, partitionid_t part)
{
    uint32_t fat_offset = current_cluster * 2;
    uint32_t fat_lba = fat_slba + (fat_offset / 512);
    uint32_t entry_index = (fat_offset % 512) / 2;
    uint16_t buffer[256];
    ata_read_sector(part.drive_id, fat_lba, buffer);
    return buffer[entry_index];
}

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

void list_dir_fat16(partitionid_t part, struct file *buff)
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
    // printf("DEBUG: Part Start: %d\n", mbrpart->lba_start);
    // printf("DEBUG: Reserved Sec: %d\n", bpb->reserved_sectors);
    // printf("DEBUG: Num FATs: %d\n", bpb->num_fats);
    // printf("DEBUG: Sec Per FAT: %d\n", bpb->sectors_per_fat);
    // printf("DEBUG: Target Root LBA: %d\n", root_dir_lba);
    // printf("sectors per cluster %d\n", bpb->sectors_per_cluster);

    int l = 0;
    // OUTER LOOP: Step through each sector of the root directory region
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        
        // Read the current root directory sector
        ata_read_sector(part.drive_id, root_dir_lba + i, (uint16_t *)dir_buffer);
        
        for (int j = 0;j < 16;j++)
        {
            //printf("entry %d sector %d\n", j, i);
            struct FAT16DirEntry *entry = (struct FAT16DirEntry *)&dir_buffer[j*32];

            // printf("filename: %s", entry->filename);

            if (entry->filename[0]==0)
            {
                end_of_directory = 1;
                break;
            }

            if (entry->filename[0] == 0xE5)
                continue;

            if (entry->attributes == LFN)
                continue; // ignore LFN records

            //ummm now just print right
            int k = 0;
            while (k < 12)
            {
                if (entry->filename[k] == ' ')
                {
                    k++;
                    continue;
                }
                if (k >= 8)
                {
                    buff[l].filename[k+1] = (char)entry->filename[k];
                } else {
                    buff[l].filename[k] = (char)entry->filename[k];
                }
                k++;
            }
            buff[l].filename[k] = '\0';
            buff[l].filename[8] = '.';

            buff[l].isdir = entry->attributes & ATTR_DIRECTORY;
            buff[l].starting_lba = root_dir_lba + i;
            buff[l].offset = j*32; // i hope evrything is correct?
            buff[l].partition = part;
            l++;
        }
        
        // Break out of the outer sector loop if the inner loop hit 0x00
        if (end_of_directory) {
            break;
        }
    }
}

bool read_file_fat16(struct file f, void *ebuffer, uint32_t count, uint32_t skip)
{ // THIS WORKED FIRST TRY (single-cluster read)
    uint16_t buffer[256];
    ata_read_sector(f.partition.drive_id, f.starting_lba, buffer);

    struct FAT16DirEntry *entry = (struct FAT16DirEntry *)&buffer[f.offset];

    uint16_t fcluster = entry->starting_cluster;
    uint32_t fsize = entry->file_size;

    // get the BPB (I never cache this, but I should)
    uint16_t pbuffer[256];
    ata_read_sector(f.partition.drive_id, 0, pbuffer);

    uint32_t buffer_index = 223 + (f.partition.partition * 8);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&pbuffer[buffer_index];

    uint16_t mbrbuffer[256];
    ata_read_sector(f.partition.drive_id, mbrpart->lba_start, mbrbuffer);

    struct fat16_bpb *bpb = (struct fat16_bpb *)mbrbuffer;
    // get the DRS
    uint32_t fat_lba = mbrpart->lba_start + bpb->reserved_sectors;

    uint32_t root_dir_lba = fat_lba + (bpb->num_fats * bpb->sectors_per_fat);

    uint32_t root_dir_sectors = (bpb->root_dir_entries * 32) / bpb->bytes_per_sector;

    uint32_t drs_lba = root_dir_lba + root_dir_sectors;

    uint32_t cskip = (skip / 256) / bpb->sectors_per_cluster;
    uint32_t askip = (skip / 256) - (cskip * bpb->sectors_per_cluster);
    uint32_t rskip = skip - (askip * 256) - ((cskip * bpb->sectors_per_cluster)*256);
    for (uint32_t i = 0; i < cskip; i++)
    {
        fcluster = get_next_cluster_fat(fcluster, fat_lba, f.partition);
        if (fcluster >= 0xFFF8)
            return true;
    }
    uint16_t test_cluster = fcluster;
    printf("Cluster Chain: %d", test_cluster, fat_lba);
    for (int c = 0; c < 4; c++) {
        test_cluster = get_next_cluster_fat(test_cluster, fat_lba, f.partition);
        printf(" -> %d", test_cluster);
    }
    printf("\n");


    printf("cskip: %d\naskip: %d\nrskip: %d\n", cskip, askip, rskip);

    uint32_t tlba = drs_lba + ((fcluster - 2) * bpb->sectors_per_cluster);

    uint32_t actual_tlba = tlba + askip;

    uint32_t counted = 0;
    bool end_read = false;
    uint16_t *output = (uint16_t *)ebuffer;
    uint32_t ncluster = fcluster;
    uint32_t toread = askip;
    for (int i = 0; i < (count / 256); i++)
    {
        actual_tlba = drs_lba +((ncluster - 2) * bpb->sectors_per_cluster);
        uint16_t tmpbuffer[256];
        ata_read_sector(f.partition.drive_id, actual_tlba + toread, tmpbuffer);
        toread++;

        for (int j = 0; j < 256; j++)
        {
            // if (rskip-- > 0)
            //     continue;
            if (counted++ >= count)
            {
                end_read = true;
                break;
            }
            output[j] = tmpbuffer[j];
        }
        if (end_read)
            break;

        output += 256;
        if (toread >= bpb->sectors_per_cluster)
        {
            ncluster = get_next_cluster_fat(ncluster, fat_lba, f.partition);
            toread = 0;
            printf("cluster %d\n", ncluster);

            if (ncluster >= 0xFFF8)
                return true;
        }
    }
    return false;
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

static struct file fbuffer[128];

struct file resolve_path(const char *path, struct file *parents)
{
    const char *current_pos = path;
    char token[256];

    partitionid_t part;
    bool isrelative = false;
    part.drive_id = 0xFE;
    part.partition = 0xFE;

    struct file f;
    f.starting_lba = 0;
    f.offset = 0;
    f.partition = part;
    f.isdir = false;
    f.parent = NULL;

    int depth = 0;

    while (next_path_token(&current_pos, token))
    {
        if (part.drive_id == 0xFE)
        {
            part = str_partid(token);
            if (part.drive_id == 0xFF)
                isrelative = true;

            f.partition = part;
        }
        else
        {
            if (depth > 0)
            {
                f.isdir = true;
                parents[depth-1] = f;
                f.parent = &parents[depth-1];
                strcpy(f.filename, token);
                f.isdir = false;
            }
            else {
                strcpy(f.filename, token);
                f.isdir = false;
            }
            depth++;
        }
    }
    return f;
}

#endif
