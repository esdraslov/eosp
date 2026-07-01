#include "fat16.h"
#include "vfs.h"

uint32_t get_next_cluster_fat(uint16_t current_cluster, uint32_t fat_slba, partitionid_t part)
{
    uint32_t fat_offset = current_cluster * 2;
    uint32_t fat_lba = fat_slba + (fat_offset / 512);
    uint32_t entry_index = (fat_offset % 512) / 2;
    // printf("DEBUG get_next: cluster=%d, offset=%d, fat_lba=%d, entry_index=%d\n", 
    //        current_cluster, fat_offset, fat_lba, entry_index);
    uint16_t buffer[256];
    ata_read_sector(part.drive_id, fat_lba, buffer);
    return buffer[entry_index];
}

void format_partition_fat16(partitionid_t drive, uint32_t start_lba, struct MBRPartition *part)
{
    struct fat16_bpb bpb;
    bpb.oem_name[0] = 'M';
    bpb.oem_name[1] = 'S';
    bpb.oem_name[2] = 'W';
    bpb.oem_name[3] = 'I';
    bpb.oem_name[4] = 'N';
    bpb.oem_name[5] = '4';
    bpb.oem_name[6] = '.';
    bpb.oem_name[7] = '1';

    bpb.bytes_per_sector = 512; // hardcoded
    bpb.sectors_per_cluster = 4; // hardcoded too because I want conviniency
    bpb.reserved_sectors = 1; // on FAT16, it's always one
    bpb.num_fats = 2; // redudancy
    bpb.root_dir_entries = 512; // compatibility, it's 512
    
    uint32_t net_sectors = part->sector_count - 1 - 32;
    uint32_t numerator = 2 * (net_sectors / 4);
    uint32_t denominator = (512 / 2) + (2 / 4);
    bpb.sectors_per_fat = (numerator + denominator - 1) / denominator;

    bpb.total_sectors_short = part->sector_count;

    bpb.media_type = 0xF8; // because it's a hard drive
                
    bpb.jmp[0] = 0xEB; // jmp 0x3C
    bpb.jmp[1] = 0x3C;
    bpb.jmp[2] = 0x90;

    uint16_t sector_buffer[256] = {0}; // 512 bytes total, all zeroed out

    // Copy your stack-allocated struct into the very front of the buffer
    memcpy(sector_buffer, &bpb, sizeof(struct fat16_bpb));

    // Assign the final magic boot signature (0xAA55) to the last 2 bytes
    sector_buffer[255] = 0xAA55;

    // Commit it to disk!
    ata_write_sector(drive.drive_id, start_lba, sector_buffer);
}

void list_dir_fat16(partitionid_t part, struct file *buff)
{
    uint16_t pbuffer[256];
    ata_read_sector(part.drive_id, 0, pbuffer);

    uint32_t buffer_index = 223 + (part.partition * 8);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&pbuffer[buffer_index];

    uint16_t buffer[256];
    ata_read_sector(part.drive_id, mbrpart->lba_start, buffer);

    struct fat16_bpb *bpb = (struct fat16_bpb *)buffer;
    uint32_t root_dir_lba = mbrpart->lba_start + bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat);
    
    // We calculate how many total sectors the root directory takes up
    // (Usually 512 entries * 32 bytes per entry = 16384 bytes -> 32 sectors)
    uint32_t root_dir_sectors = (bpb->root_dir_entries * 32) / 512;

    uint8_t dir_buffer[512];
    int end_of_directory = 0;
    // printf("DEBUG: Part Start: %d\n", mbrpart->lba_start);
    // printf("DEBUG: Reserved Sec: %d\n", bpb->reserved_sectors);
    // printf("DEBUG: Num FATs: %d\n", bpb->num_fats);
    // printf("DEBUG: Sec Per FAT: %d\n", bpb->sectors_per_fat);
    // printf("DEBUG: Target Root LBA: %d\n", root_dir_lba);
    // printf("sectors per cluster %d\n", bpb->sectors_per_cluster);

    int l = 0;
    // OUTER LOOP: Step through each sector of the root directory region
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        
        // Read the current root directory sector
        ata_read_sector(part.drive_id, root_dir_lba + i, (uint16_t *)dir_buffer);
        
        for (int j = 0;j < 16;j++)
        {
            //printf("entry %d sector %d\n", j, i);
            struct FAT16DirEntry *entry = (struct FAT16DirEntry *)&dir_buffer[j*32];

            // printf("filename: %s", entry->filename);

            if (entry->filename[0]==0)
            {
                end_of_directory = 1;
                break;
            }

            if (entry->filename[0] == 0xE5)
                continue;

            if (entry->attributes == LFN)
                continue; // ignore LFN records

            //ummm now just print right
            int k = 0;
            while (k < 12)
            {
                if (entry->filename[k] == ' ')
                {
                    k++;
                    continue;
                }
                if (k >= 8)
                {
                    buff[l].filename[k+1] = (char)entry->filename[k];
                } else {
                    buff[l].filename[k] = (char)entry->filename[k];
                }
                k++;
            }
            buff[l].filename[k] = '\0';
            buff[l].filename[8] = '.';

            buff[l].isdir = entry->attributes & ATTR_DIRECTORY;
            buff[l].starting_lba = root_dir_lba + i;
            buff[l].offset = j*32; // i hope evrything is correct?
            buff[l].partition = part;
            l++;
        }
        
        // Break out of the outer sector loop if the inner loop hit 0x00
        if (end_of_directory) {
            break;
        }
    }
}

bool read_file_fat16(struct file f, void *ebuffer, uint32_t count, uint32_t skip)
{ // THIS WORKED FIRST TRY (single-cluster read)
    uint16_t buffer[256];
    ata_read_sector(f.partition.drive_id, f.starting_lba, buffer);

    struct FAT16DirEntry *entry = (struct FAT16DirEntry *)&buffer[f.offset];

    uint16_t fcluster = entry->starting_cluster;
    uint32_t fsize = entry->file_size;

    // get the BPB (I never cache this, but I should)
    uint16_t pbuffer[256];
    ata_read_sector(f.partition.drive_id, 0, pbuffer);

    uint32_t buffer_index = 223 + (f.partition.partition * 8);
    struct MBRPartition *mbrpart = (struct MBRPartition *)&pbuffer[buffer_index];

    // printf("%d\n", buffer_index);

    uint16_t mbrbuffer[256];
    ata_read_sector(f.partition.drive_id, mbrpart->lba_start, mbrbuffer);

    struct fat16_bpb *bpb = (struct fat16_bpb *)mbrbuffer;
    // get the DRS
    uint32_t fat_lba = mbrpart->lba_start + bpb->reserved_sectors;

    uint32_t root_dir_lba = fat_lba + (bpb->num_fats * bpb->sectors_per_fat);

    uint32_t root_dir_sectors = (bpb->root_dir_entries * 32) / bpb->bytes_per_sector;

    uint32_t drs_lba = root_dir_lba + root_dir_sectors;

    uint32_t cskip = (skip / 256) / bpb->sectors_per_cluster;
    uint32_t askip = (skip / 256) - (cskip * bpb->sectors_per_cluster);
    uint32_t rskip = skip - (askip * 256) - ((cskip * bpb->sectors_per_cluster)*256);
    for (uint32_t i = 0; i < cskip; i++)
    {
        fcluster = get_next_cluster_fat(fcluster, fat_lba, f.partition);
        if (fcluster >= 0xFFF8)
            return true;
    }

    // uint16_t test_cluster = fcluster;
    // printf("Cluster Chain: %d", test_cluster, fat_lba);
    // for (int c = 0; c < 4; c++) {
    //     test_cluster = get_next_cluster_fat(test_cluster, fat_lba, f.partition);
    //     printf(" -> %d", test_cluster);
    // }
    // printf("\n");


    // printf("cskip: %d\naskip: %d\nrskip: %d\nlba: %d\n", cskip, askip, rskip, fat_lba);

    uint32_t tlba = drs_lba + ((fcluster - 2) * bpb->sectors_per_cluster);

    uint32_t actual_tlba = tlba + askip;

    uint32_t counted = 0;
    bool end_read = false;
    uint16_t *output = (uint16_t *)ebuffer;
    uint32_t ncluster = fcluster;
    uint32_t toread = askip;
    uint32_t idxa = 0;
    for (int i = 0; i < (count / 256); i++)
    {
        actual_tlba = drs_lba +((ncluster - 2) * bpb->sectors_per_cluster);
        uint16_t tmpbuffer[256];
        ata_read_sector(f.partition.drive_id, actual_tlba + toread, tmpbuffer);
        toread++;
        // printf("%d %d\n", actual_tlba, actual_tlba + toread);

        for (int j = 0; j < 256; j++)
        {
            // if (rskip-- > 0)
            //     continue;
            if (counted++ >= count)
            {
                end_read = true;
                break;
            }
            output[idxa] = tmpbuffer[j];
            idxa++;
        }
        if (end_read)
            break;

        output += 256;
        if (toread >= bpb->sectors_per_cluster)
        {
            ncluster = get_next_cluster_fat(ncluster , fat_lba, f.partition);
            toread = 0;
            printf("cluster %d\n", ncluster);

            if (ncluster >= 0xFFF8)
                return true;
        }
    }
    return false;
}
