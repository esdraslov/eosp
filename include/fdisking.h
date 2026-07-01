#ifndef FDISK_H
#define FDISK_H

#include "atapio.h"
#include "fs/ext2.h"
#include "fs/fat16.h"
#include "math.h"
#include "stdlib.h"
#include "fs/vfs.h"
#include "disk_structs.h"

struct partition_info {
    uint8_t drive_id;
    uint32_t partition_index;
    uint32_t lba_start;         // Cached from MBR
};

enum filesystem {
    fat16,
    ext2
};

void init_mbr(uint8_t drive_id);

void create_partition_mbr(uint8_t drive_id, uint8_t slot, uint32_t start_lba, uint32_t sector_count);

int detect_architect(uint8_t drive_id);

void format_partition_mbr(uint8_t drive_id, uint8_t slot, enum filesystem fs);

#endif
