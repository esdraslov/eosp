#ifndef EXT2_H
#define EXT2_H

#include "fs/vfs.h"
#include "stdlib.h"
#include "atapio.h"
#include "math.h"
#include "disk_structs.h"
#include "strings.h"
#include <stdint.h>
#include "ext.h"
#include "time.h"

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

struct ext2_dir_entry {
    uint32_t inode;         // Inode number of the file/dir
    uint16_t rec_len;       // Distance to the next directory entry
    uint8_t  name_len;      // Length of the filename string
    uint8_t  file_type;     // 1 = Regular file, 2 = Directory, etc.
    char     name[255];     // The actual name (variable length!)
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
