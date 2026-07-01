#ifndef DISK_STRUCTS_H
#define DISK_STRUCTS_H

#include <stdint.h>

struct MBRPartition {
    uint8_t  drive_status;   // 0x80 = Active/Bootable, 0x00 = Inactive
    uint8_t  chs_start[3];   // Legacy Cylinder-Head-Sector starting address
    uint8_t  partition_type; // 0x06 or 0x0E for FAT16
    uint8_t  chs_end[3];     // Legacy CHS ending address
    uint32_t lba_start;      // The actual starting sector (e.g., 2048)
    uint32_t sector_count;   // Total number of sectors in this partition
} __attribute__((packed));

struct MBR {
    uint8_t boot_code[446];
    struct MBRPartition partitions[4];
    uint16_t signature;
} __attribute__((packed));

#endif
