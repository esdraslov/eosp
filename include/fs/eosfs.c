#include "eosfs.h"
#include <stdint.h>

void format_partition_eosfs(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part)
{
    uint16_t buffer[256];
    memset(&buffer, 0, sizeof(buffer));

    struct eosfs_sb *sb = (struct eosfs_sb *)&buffer;

    // 0x46534400938826
    sb->magic[0] = 0x45;
    sb->magic[1] = 0x46;
    sb->magic[2] = 0x53;
    sb->magic[3] = 0x44;
    sb->magic[4] = 0x00;
    sb->magic[5] = 0x93;
    sb->magic[6] = 0x88;
    sb->magic[7] = 0x26;
    sb->version = 1;
    sb->groups = 1;

    ata_write_sector(drive.drive_id, start_lba+1, buffer);
    memset(&buffer, 0, sizeof(buffer));

    struct eosfs_gd *gd = (struct eosfs_gd *)&buffer;

    gd->blocks = min(part->sector_count-3, 32);
    gd->group_descriptor = true;

    ata_write_sector(drive.drive_id, start_lba+2, buffer);
}

void list_root_dir_eosfs(partitionid_t part, struct file *buff)
{
    uint16_t mbr[256];
    ata_read_sector(part.drive_id, 0, mbr);

    uint32_t buff_ind = 223 + (part.partition * 8);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&mbr[buff_ind];

    uint8_t buffer[512];
    uint32_t start = mbrpart->lba_start + 3;

    uint32_t wawawaawaw = sizeof(struct eosfs_dir_entry);
    bool end = false;
    int f = 0;
    for (;;)
    {
        // printf("%d\n", start);
        ata_read_sector(part.drive_id, start, (uint16_t *)buffer);
        for (int i = 0;i < 256 / wawawaawaw;i++)
        {
            uint16_t offset = wawawaawaw * i;
            // printf("o: %d\n", offset);
            struct eosfs_dir_entry *e = (struct eosfs_dir_entry *)&buffer[offset];
            if (e->filename[0] == '\0')
            {
                end = true;
                return;
            }
            if (i == 0 && e->content_block) // we ignore this if it's not on the first entry
            {
                break;
            }
            strcpy(buff[f].filename, e->filename);
            buff[f].isdir = e->flags & EOSFS_DIR ? true : false;
            buff[f].starting_lba = start;
            buff[f].offset = i * wawawaawaw;
            buff[f].partition = part;
            f++;
            if (e->next_entry.gid != 0)
            {
                uint32_t q = start - 1;
                uint32_t gid = 0;
                while (gid != e->next_entry.gid - 1)
                {
                    ata_read_sector(part.drive_id, start, (uint16_t *)buffer);
                    struct eosfs_gd *gd = (struct eosfs_gd *)&buffer;
                    q += gd->blocks;
                    gid++;
                }
                start = q + e->next_entry.bid;
                break;
            }
        }
        if (end)
            return;
        start++;
    }
}

bool read_file_eosfs(struct file f, void *ebuffer, uint32_t count, uint32_t skip)
{
    uint8_t buffer[512];
    ata_read_sector(f.partition.drive_id, f.starting_lba, (uint16_t *)buffer);
    struct eosfs_dir_entry *e = (struct eosfs_dir_entry *)&buffer[f.offset];

    uint32_t start = f.starting_lba;
    struct eosfs_dir_entry *temp_e = (struct eosfs_dir_entry *)&buffer;
    while (!temp_e->group_descriptor)
    {
        start--;
        ata_read_sector(f.partition.drive_id, start, (uint16_t *)buffer);
        temp_e = (struct eosfs_dir_entry *)&buffer;
    }

    uint32_t q = start;
    uint32_t gid = 0;
    while (gid != e->first_content_block.gid)
    {
        ata_read_sector(f.partition.drive_id, start, (uint16_t *)buffer);
        struct eosfs_gd *gd = (struct eosfs_gd *)&buffer;
        q += gd->blocks;
        gid++;
    }
    start = q + e->first_content_block.bid;
    ata_read_sector(f.partition.drive_id, start, (uint16_t *)buffer);

    struct eosfs_cbh *content_block = (struct eosfs_cbh *)buffer;
    if (!content_block->content_block)
        return false;

    memset(ebuffer, 0, sizeof(ebuffer));
    ebuffer = (void *)buffer[sizeof(struct eosfs_cbh)];
    
    return true;
}

bool create_file_root_eosfs(char *fname, partitionid_t part)
{
    uint16_t mbr[256];
    ata_read_sector(part.drive_id, 0, mbr);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&mbr[223 + part.partition * 8];

    uint8_t buffer[512];
    ata_read_sector(part.drive_id, mbrpart->lba_start, (uint16_t *)buffer);
}
