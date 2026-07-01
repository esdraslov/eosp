#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stdbool.h>
#include "strings.h"

typedef struct {
    uint8_t drive_id;
    uint8_t partition; // aka slot on MBR
} partitionid_t;

struct file {
    char filename[64];
    bool isdir;
    uint32_t starting_lba; // READ METADATA
    uint8_t offset;
    partitionid_t partition;
    struct file *parent;
}; // removed packed attribute so some systems don't complain about it

partitionid_t str_partid(char *str);

static struct file _fbuffer[128];

struct file resolve_path(const char *path, struct file *parents);

#endif
