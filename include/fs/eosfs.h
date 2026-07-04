#ifndef EOSFS_H
#define EOSFS_H

#include <cstdint>
#include <stdint.h>

struct eosfs_sb {
    uint8_t magic[8];
    uint16_t version;

    uint8_t padding[509];
} __attribute__((packed));

#endif
