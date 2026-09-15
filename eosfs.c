///////////////////////////////////////////
//  Tool to manage EOSFS images on Linux //
///////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
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
    } first_content_block;
};

int main()
{
    FILE *f = fopen("a.img", "wb");
    struct eosfs_sb sb = { {0x46, 0x53, 0x44, 0x00,
                         0x93, 0x88, 0x26}, 1, 1, 0, {0} };
    struct eosfs_gd gd = { 1, 0 };
    struct eosfs_dir_entry e = { 0, "bahhh", { 0, 0 } };
    fseek(f, )

    return 0;
}
