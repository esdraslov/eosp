#ifndef ATAPIO_H
#define ATAPIO_H

#include "ports.h"
#include "stdlib.h"
#define DATAPORT 0x0
#define SECCOUNT 0x2 // sector count
#define DHSELECT 0x6 // drive/head select
#define DVSTATUS 0x7 // drive status
#define DVCOMMAND 0x7 // drive command YEA IT'S JUST DVSTATUS
#define LBALOW 0x3
#define LBAMID 0x4
#define LBAHIGH 0x5
#define ALT_STATUS 0x3F6 // alternative status thing
#define DEVCTRL 0x3F6 // device control
#define CHDIF 0x80 // when change for control drive 3 and 4

void ata_read_sector(uint8_t drive_id, uint32_t lba, uint16_t *buffer);

void ata_write_sector(uint8_t drive_id, uint32_t lba, uint16_t *buffer);

void ata_read_sectors(uint8_t drive_id, uint32_t lba, uint8_t sectors_count, uint16_t *buffer);

void ata_write_sectors(uint8_t drive_id, uint32_t lba,  uint8_t sectors_count, uint16_t *buffer);

// uint32_t ata_get_total_sectors();

#endif
