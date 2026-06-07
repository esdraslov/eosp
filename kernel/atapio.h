#ifndef ATAPIO_H
#define ATAPIO_H

#include "ports.h"
#define DATAPORT 0x1F0
#define SECCOUNT 0x1F2 // sector count
#define DHSELECT 0x1F6 // drive/head select
#define DVSTATUS 0x1F7 // drive status
#define LBALOW 0x1F3
#define LBAMID 0x1F4
#define LBAHIGH 0x1F5

void ata_read_sector(uint32_t lba, uint16_t *buffer)
{
    while (inb(DVSTATUS) & 0x80); // wait until drive is not busy :/

    outb(SECCOUNT, 1); // say "I want to something 1 sector" to drive (why am I commeting like this)
    outb(LBALOW, (uint8_t)lba); // tell where is that sector
    outb(LBAMID, (uint8_t)(lba >> 8));
    outb(LBAHIGH, (uint8_t)(lba >> 16));
    outb(DHSELECT, 0xE0 | ((lba >> 24) & 0x0F)); // tell if it's on the master or slave drive (really bad terminology)

    outb(DVSTATUS, 0x20); // tell we want to read

    while ((inb(DVSTATUS) & 0x08) == 0) {} // and wait :/

    for (int i = 0; i < 256; i++)
    {
        // now we only read 256 words (512 bytes)
        buffer[i] = inw(0x1F0);
    }
}

#endif
