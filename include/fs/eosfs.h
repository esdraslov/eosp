#ifndef EOSFS_H
#define EOSFS_H

#include "atapio.h"
#include "disk_structs.h"
#include "vfs.h"
#include "math.h"
#include <stdint.h>
#define EOSFS_MAX_FILENAME_LEN 64
#define EOSFS_DIR 1

struct eosfs_sb {
    uint8_t magic[8];
    uint16_t version;
    uint32_t groups;
    uint32_t group_bitmap;

    uint8_t padding[509];
} __attribute__((packed));

// group descriptor
struct eosfs_gd {
    uint8_t blocks;
    uint32_t block_bitmap;
} __attribute__((packed));

struct eosfs_dir_entry {
    uint8_t flags;
    char filename[EOSFS_MAX_FILENAME_LEN];
    struct {
        uint32_t gid;
        uint32_t bid;
    } __attribute__((packed)) first_content_block;
} __attribute__((packed));

void format_partition_eosfs(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part);
void list_root_dir_eosfs(partitionid_t part, struct file *buff);
bool read_file_eosfs(struct file f, void *ebuffer, uint32_t count, uint32_t skip);

#endif
