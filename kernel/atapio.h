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

void ata_read_sector(uint32_t lba, uint16_t *buffer) {
    // 1. Send commands (Keep your outb statements here)
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20); // Read command

    // 2. The Atomic Assembly Block
    // This handles polling AND reading without letting GCC generate stack offsets!
    __asm__ volatile (
        "cld\n\t"                     // 1. Force Direction Flag to 0 (Crucial!)
        "movw $0x1F7, %%dx\n\t"       // Target status port
        
        ".poll_loop:\n\t"
        "inb (%%dx), %%al\n\t"        // Read status
        "testb $0x08, %%al\n\t"       // Check DRQ bit
        "jnz .read_data\n\t"          // If set, exit loop to read
        "pause\n\t"
        "jmp .poll_loop\n\t"          // Retry
        
        ".read_data:\n\t"
        "movw $0x1F0, %%dx\n\t"       // Target data port
        "movl $256, %%ecx\n\t"        // Read 256 words (512 bytes)
        "rep insw\n\t"                // Stream hardware data straight into %es:%edi!
        : "=D"(buffer)                // Output/Destination register forced to EDI
        : "D"(buffer)                 // Input buffer pointer mapped to EDI
        : "eax", "ecx", "edx", "memory" // Clobber list telling GCC exactly what we took
    );
}

#endif
