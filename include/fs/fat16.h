#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include "vfs.h"
#include "../atapio.h"
#include <stdbool.h>
#include "../disk_structs.h"
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0X20
#define LFN 0x07

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

struct FAT16DirEntry {
    uint8_t filename[11]; // just make this uint8_t
    uint8_t attributes;        // File attributes (0x10 = Directory, 0x20 = Archive)
    uint8_t reserved[10];      // Reserved for WinNT/timestamps
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t starting_cluster; // CRITICAL: This is what read_file will need!
    uint32_t file_size;        // File size in bytes
} __attribute__((packed));

void format_partition_fat16(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part);

uint32_t get_next_cluster_fat(uint16_t current_cluster, uint32_t fat_slba, partitionid_t part);

void list_dir_fat16(partitionid_t part, struct file *buff);

bool read_file_fat16(struct file f, void *ebuffer, uint32_t count, uint32_t skip);

#endif
