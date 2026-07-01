#include "ext2.h"

void format_partition_ext2(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part)
{
    struct ext2_sb block;
    memset(&block, 0, sizeof(struct ext2_sb));

    block.creator_os = os_linux;
    block.magic = 0xEF53;
    block.inodes_count = (part->sector_count * 512) / 8192;
    block.inodes_count = (block.inodes_count + 7) & ~7;
    block.blocks_count = part->sector_count / 2;
    block.blocks_per_group = 8192;
    block.frags_per_group = 8192;
    block.inodes_per_group = min(8192, block.inodes_count);
    // block.free_blocks = block.blocks_count; // not sure how many metadata blocks yet
    block.free_inodes = block.inodes_count - 1;
    // struct rtc_time t;
    // get_rtc_time(&t);
    block.wtime = 1772391600;
    block.first_data_block = 1;

    uint16_t fhalf[256];
    memcpy(&fhalf, &block, 512);
    ata_write_sector(drive.drive_id, start_lba+2, fhalf);

    struct ext2_bgd bgd;
    memset(&bgd, 0, sizeof(struct ext2_bgd));
}
