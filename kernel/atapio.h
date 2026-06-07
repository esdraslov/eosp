#ifndef ATAPIO_H
#define ATAPIO_H

#include "ports.h"
#include "stdlib.h"
#define DATAPORT 0x1F0
#define SECCOUNT 0x1F2 // sector count
#define DHSELECT 0x1F6 // drive/head select
#define DVSTATUS 0x1F7 // drive status
#define DVCOMMAND 0x1F7 // drive command YEA IT'S JUST DVSTATUS
#define LBALOW 0x1F3
#define LBAMID 0x1F4
#define LBAHIGH 0x1F5
#define ALT_STATUS 0x3F6 // alternative status thing
#define DEVCTRL 0x3F6 // device control

void ata_read_sector(uint32_t lba, uint16_t *buffer) {
    while ((inb(DVSTATUS) & 128)) {} // wait till not busy :/

    // 1. Send commands (Keep your outb statements here)
    outb(DHSELECT, 0xE0 | ((lba >> 24) & 0x0F)); // what drive to talk to (slave/master) (holy bad terminology)
    outb(DEVCTRL, 0x2); // tell ATA TO SHUT UP
    outb(SECCOUNT, 1); // read 1 sector
    outb(LBALOW, (uint8_t)lba); // where is that sector
    outb(LBAMID, (uint8_t)(lba >> 8));
    outb(LBAHIGH, (uint8_t)(lba >> 16));
    outb(DVCOMMAND, 0x20); // Read command

    while ((inb(DVSTATUS) & 0x8) == 0) {} // wait till ready

    for (int i = 0;i < 256; i++)
    {
        buffer[i] = inw(DATAPORT); // actually read (I FORGOT THIS THE FIRST TIME :/)
    }

    inb(DATAPORT); // tell we're done
}

void ata_write_sector(uint32_t lba, uint16_t *buffer) {
    // pratically the same thing
    // 1. Setup registers
    outb(DHSELECT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(DEVCTRL, 0x2); // SHUT UP ATA
    outb(SECCOUNT, 1); // Write 1 sector
    outb(LBALOW, (uint8_t)lba);
    outb(LBAMID, (uint8_t)(lba >> 8));
    outb(LBAHIGH, (uint8_t)(lba >> 16));
    
    // 2. Issue the WRITE command (0x30)
    outb(DVCOMMAND, 0x30);

    // 3. Wait for the drive to be ready to receive the buffer
    while (1) {
        uint8_t status = inb(DVSTATUS);
        if (!(status & 0x80) && (status & 0x08)) {
            break; // Busy is clear, DRQ is set!
        }
    }

    // 4. Blast the text data out to the drive buffer
    for (int i = 0; i < 256; i++) {
        outw(DATAPORT, buffer[i]);
    }

    // 5. Tell the drive to commit the cache to the actual storage
    outb(0x1F7, 0xE7);
    while (inb(0x1F7) & 0x80); // Wait for flush to finish
}

#endif
