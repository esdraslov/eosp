#include "ext2.h"

void format_partition_ext2(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part)
{
    time_t t;
    get_rtc_time(&t);
    uint32_t stamp = time_t_to_timestamp(&t);

    struct ext_sb block;
    memset(&block, 0, sizeof(struct ext_sb));

    block.creator_os = os_linux;
    block.magic = 0xEF53;
    block.inodes_count = (part->sector_count * 512) / 8192;
    block.inodes_count = (block.inodes_count + 7) & ~7;
    block.blocks_count = part->sector_count / 2;
    block.blocks_per_group = 8192;
    block.frags_per_group = 8192;
    block.inodes_per_group = min(8192, block.inodes_count);
    block.free_blocks = block.blocks_count; // not sure how many metadata blocks yet
    block.free_inodes = block.inodes_count - 1;
    block.wtime = stamp;
    block.first_data_block = 1;

    uint16_t fhalf[256];
    memcpy(&fhalf, &block, 512);
    // ata_write_sector(drive.drive_id, start_lba+2, fhalf); // write later

    struct ext2_bgd bgd; // TODO: Make this initialize every single block group
    memset(&bgd, 0, sizeof(struct ext2_bgd));

    bgd.block_bitmap = 3;
    bgd.inode_bitmap = 4;
    bgd.inode_table = 5;

    uint32_t total_inodes_b = block.inodes_per_group * 128;
    uint32_t block_size = 1024 << block.log_block_size;
    uint32_t inode_table_size = (total_inodes_b + (block_size - 1)) / block_size;
    uint32_t metadata = 5 + inode_table_size;

    bgd.free_inodes_count = block.inodes_per_group - 10;
    bgd.free_blocks_count = block.blocks_per_group - metadata;
    bgd.used_dirs_count = 1;
    block.free_blocks -= metadata;
    block.feature_compat |= COMPAT_LAZY_BG;
    // block.feature_ro_compat |= 0x1;

    ata_write_sectors(drive.drive_id, start_lba+4, 2, (uint16_t *)&bgd);

    uint8_t block_bitmap[1024] = {0};
    for (int i = 0;i < metadata;i++)
    {
        uint32_t byte = i / 8;
        uint8_t bit = i % 8;
        block_bitmap[byte] |= (1 << bit);
    }

    ata_write_sectors(drive.drive_id, start_lba+6, 2, (uint16_t *)&block_bitmap);

    uint8_t inode_bitmap[1024] = {0};
    for (int i = 0;i < 10;i++)
    {
        uint32_t byte = i / 8;
        uint8_t bit = i % 8;
        inode_bitmap[byte] |= (1 << bit);
    }

    ata_write_sectors(drive.drive_id, start_lba+8, 2, (uint16_t *)&inode_bitmap);


    uint32_t offset = 10;

    struct ext_inode inode_table[8];
    memset(&inode_table, 0, sizeof(inode_table));
    inode_table[1].mode = 0x41ED;
    inode_table[1].size = 1024;
    inode_table[1].links_count = 2;
    inode_table[1].blocks = 2;
    inode_table[1].block[0] = metadata;

    inode_table[1].atime = stamp;
    inode_table[1].ctime = stamp;
    inode_table[1].mtime = stamp;

    ata_write_sectors(drive.drive_id, start_lba+offset, 2, (uint16_t *)inode_table);

    memset(&inode_table, 0, sizeof(inode_table));

    for (int i = 1; i < inode_table_size; i++)
    {
        offset += 2;
        ata_write_sectors(drive.drive_id, start_lba+offset, 2, (uint16_t *)inode_table);
    }

    uint8_t dir_b[1024] = {0};
    uint32_t o = 0;

    struct ext2_dir_entry *f = (struct ext2_dir_entry *)&dir_b[o];
    f->inode = 2;
    f->name_len = 1;
    f->file_type = 2;
    strcpy(f->name, ".");
    f->rec_len = 12;
    o += 12;

    struct ext2_dir_entry *ff = (struct ext2_dir_entry *)&dir_b[o];
    ff->inode = 2;
    ff->name_len = 2;
    ff->file_type = 2;
    strcpy(ff->name, "..");
    ff->rec_len = 12;
    o += 12;
}
