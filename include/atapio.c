#include "atapio.h"

void ata_read_sector(uint8_t drive_id, uint32_t lba, uint16_t *buffer) {
    uint8_t slave_bit = drive_id;
    uint16_t base_port = 0x1F0;;

    if (drive_id >= 2 && drive_id < 4)
    {
        slave_bit -= 2;
        base_port -= CHDIF; // we now talk to channel 2
    } else if (drive_id >= 4)
    {
        return;
    }

    outb(base_port + DHSELECT, 0xE0 | (slave_bit << 4) | ((lba >> 24) & 0x0F)); // what drive to talk to (slave/master) (holy bad terminology)

    while ((inb(base_port + DVSTATUS) & 128)) {} // wait till not busy :/

    // 1. Send commands (Keep your outb statements here)
    outb(DEVCTRL, 0x2); // tell ATA TO SHUT UP
    outb(base_port + SECCOUNT, 1); // read 1 sector
    outb(base_port + LBALOW, (uint8_t)lba); // where is that sector
    outb(base_port + LBAMID, (uint8_t)(lba >> 8));
    outb(base_port + LBAHIGH, (uint8_t)(lba >> 16));
    outb(base_port + DVCOMMAND, 0x20); // Read command

    while ((inb(base_port + DVSTATUS) & 0x8) == 0) {} // wait till ready

    for (int i = 0;i < 256; i++)
    {
        buffer[i] = inw(base_port + DATAPORT); // actually read (I FORGOT THIS THE FIRST TIME :/)
    }

    inb(base_port + DATAPORT); // tell we're done
}

void ata_write_sector(uint8_t drive_id, uint32_t lba, uint16_t *buffer) {
    // pratically the same thing
    // 1. Setup registers
    uint8_t slave_bit = drive_id;
    uint16_t base_port = 0x1F0;;

    if (drive_id >= 2 && drive_id < 4)
    {
        slave_bit -= 2;
        base_port -= CHDIF; // we now talk to channel 2
    } else if (drive_id >= 4)
    {
        return;
    }

    outb(base_port + DHSELECT, 0xE0 | (slave_bit << 4) | ((lba >> 24) & 0x0F));
    outb(base_port + DEVCTRL, 0x2); // SHUT UP ATA
    outb(base_port + SECCOUNT, 1); // Write 1 sector
    outb(base_port + LBALOW, (uint8_t)lba);
    outb(base_port + LBAMID, (uint8_t)(lba >> 8));
    outb(base_port + LBAHIGH, (uint8_t)(lba >> 16));
    
    // 2. Issue the WRITE command (0x30)
    outb(base_port + DVCOMMAND, 0x30);

    // 3. Wait for the drive to be ready to receive the buffer
    while (1) {
        uint8_t status = inb(base_port + DVSTATUS);
        if (!(status & 0x80) && (status & 0x08)) {
            break; // Busy is clear, DRQ is set!
        }
    }

    // 4. Blast the text data out to the drive buffer
    for (int i = 0; i < 256; i++) {
        outw(base_port + DATAPORT, buffer[i]);
    }

    // 5. Tell the drive to commit the cache to the actual storage
    outb(base_port + DVCOMMAND, 0xE7);
    while (inb(base_port + DVSTATUS) & 0x80); // Wait for flush to finish
}
