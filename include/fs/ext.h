#ifndef EXT_H
#define EXT_h

#include <stdint.h>
#define COMPAT_DIR_PREALLOC 0x1
#define COMPAT_IMAGIC_INODES 0x2 // umm looks useless
#define COMPAT_HAS_JOURNAL 0x4
#define COMPAT_EXT_ATTR 0x8
#define COMPAT_RESIZE_INODE 0x10
#define COMPAT_DIR_INDEX 0x20
#define COMPAT_LAZY_BG 0x40
#define COMPAT_EXCLUDE_INODE 0x80
#define COMPAT_EXCLUDE_BITMAP 0x100
#define COMPAT_SPARSE_SUPER2 0x200

struct ext_sb {
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

    // EXT3 AND FORWARD
    uint32_t first_inode; // non-reserved
    uint16_t inode_size;
    uint16_t block_group_num;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char v_name[16]; // volume name
    char last_mounted[64];
    uint32_t algorithm_usage_bitmap; // compression

    // EXT4
    uint8_t ext4_padding[136]; // TODO
    uint8_t PADDING[684];
} __attribute__((packed));

struct ext_inode {
    uint16_t mode;        // File type and access rights (e.g., 0x41ED for directory)
    uint16_t uid;         // Owner's User ID
    uint32_t size;        // File size in bytes
    uint32_t atime;       // Access time (Unix timestamp)
    uint32_t ctime;       // Creation time
    uint32_t mtime;       // Modification time
    uint32_t dtime;       // Deletion time
    uint16_t gid;         // Group ID
    uint16_t links_count; // How many hard links point to this inode
    uint32_t blocks;      // Number of 512-byte sectors reserved for this file's data
    uint32_t flags;       // Ext2 behavior flags
    uint32_t osd1;        // OS-specific data (can be left 0)
    uint32_t block[15];   // Pointers to data blocks! (12 direct, 3 indirect)
    uint32_t generation;  // File version (used by NFS)
    uint32_t file_acl;    // File Access Control List block
    uint32_t dir_acl;     // Directory ACL / High 32-bits of size
    uint32_t faddr;       // Fragment address (obsolete, set to 0)
    uint8_t  osd2[12];    // More OS-specific data
};

#endif
