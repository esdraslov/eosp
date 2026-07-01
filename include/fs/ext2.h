#ifndef EXT2_H
#define EXT2_H

#include "fs/vfs.h"
#include "stdlib.h"
#include "atapio.h"
#include "math.h"
#include "disk_structs.h"
#include "strings.h"
#include <stdint.h>

struct ext2_sb {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mtime;
    uint32_t wtime;

    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_lvl;

    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    
    uint16_t def_resuid;
    uint16_t def_resgid;

    uint8_t PADDING[884];
} __attribute__((packed));

struct ext2_bgd {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t reserved[3];
};

enum ext2_creatoros {
    os_linux,
    os_hurd,
    os_masix,
    os_freebsd,
    os_lites
};

void format_partition_ext2(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part);

#endif
