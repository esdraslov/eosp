#ifndef ATAPIO_H
#define ATAPIO_H

#include "ports.h"
#include "stdlib.h"
#define DATAPORT 0x1F0
#define SECCOUNT 0x1F2 // sector count
#define DHSELECT 0x1F6 // drive/head select
#define DVSTATUS 0x1F7 // drive status
#define LBALOW 0x1F3
#define LBAMID 0x1F4
#define LBAHIGH 0x1F5
#define ATA_ALT_STATUS 0x3F6

void ata_read_sector(uint32_t lba, uint16_t *buffer)
{
    uint16_t *dest = buffer;

    int timeout = 0;
    uint8_t status;

    while (1) { // wait till drive not busy :/
        status = inb(DVSTATUS);
        if ((status & 0x80) == 0) {
            break; // Drive is not busy anymore!
        }
        
        timeout++;
        if (timeout > 1000) {
            // If this prints "0xFF", your port number is wrong or no drive is attached!
            printf("\n[ATA Timeout 1] Status was: 0x%X\n", status);
            return; 
        }
        __asm__ volatile("pause" ::: "memory");
    }

    outb(SECCOUNT, (uint8_t)1); // say "I want to something 1 sector" to drive (why am I commeting like this)
    outb(LBALOW, (uint8_t)lba); // tell where is that sector
    outb(LBAMID, (uint8_t)(lba >> 8));
    outb(LBAHIGH, (uint8_t)(lba >> 16));
    outb(DHSELECT, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F))); // tell if it's on the master or slave drive (really bad terminology)

    outb(DVSTATUS, (uint8_t)0x20); // tell we want to read

    // for(int i = 0; i < 5; i++) { // time-wasting thingy because ATA IDE is EXTREMELY slow
    //     inb(ATA_ALT_STATUS);
    // }

    __asm__ volatile (
        "1:\n\t"
        "movw $0x1F7, %%dx\n\t"
        "inb (%%dx), %%al\n\t"
        "testb $0x08, %%al\n\t"
        "jnz 2f\n\t"            // If DRQ is set, jump forward to 2
        "pause\n\t"
        "jmp 1b\n\t"            // Otherwise, loop back to 1
        "2:\n\t"
        : 
        : 
        : "dx", "al" // Tell the compiler exactly which registers we are using!
    );

    for (int i = 0; i < 256; i++)
    {
        // now we only read 256 words (512 bytes)
        dest[i] = inw(0x1F0);
    }
}

#endif
