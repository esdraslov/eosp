#ifndef SHELL_H
#define SHELL_H

#include "stdlib.h"
#include "ports.h"
#include "atapio.h"

uint16_t buffer[512] __attribute__((aligned(16)));

void reboot(void)
{
    uint8_t temp = inb(0x64);
    while (temp & 0x01)
    {
        inb(0x60);
        temp = inb(0x64);
    }

    outb(0x64, 0xFE);

    __asm__ __volatile__("cli");
    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

// basically halt 'n' fire
void haltsys(void)
{
    printf("System halted, it's now safe to shutdown...");
    __asm__ __volatile__("cli");
    while(1)
    {
        __asm__ __volatile__("hlt");
    }
}

void dump_sector(uint16_t *buffer)
{
    for (int i = 0; i < 256; i++)
    {
        printf("%d ", buffer[i]);

        if ((i+1) % 8 == 0)
        {
            printf("\n"); // newline every 8 numbers
        }
    }
}

void process_command(char *cmdl)
{
    char argv[5][32] = {0};
    int i = 0;
    int j = 0;
    int sub = 0;
    while (cmdl[i] != '\0' && j < 5) {
        if (cmdl[i] == ' ') {
            sub = ++i;
            j++;
            continue;
        }
        argv[j][i-sub] = cmdl[i];
        i++;
    }

    char* cmd = argv[0];
    printf("%s", argv[1]);

    if (strcmp(cmd, "help") == 0)
    {
        printf("help - prints this text\n");
        printf("reboot - reboots the device\n");
        printf("halt - halts the system\n");
        printf("read <lba> - reads the first sector of lba <lba> and prints the result\n");
        printf("write <lba> <text> - writes to the first sector of lba <lba> with <text>");
    } else if (strcmp(cmd, "reboot") == 0)
    {
        reboot();
    } else if (strcmp(cmd, "halt") == 0)
    {
        haltsys();
    } else if (strcmp(cmd, "read") == 0)
    {
        ata_read_sector(atoi(argv[1]), buffer);
        dump_sector(buffer);
        /*
        for(int i=0; i<255; i++) my_buffer[i] = 0x4141; // 'AA'
        my_buffer[255] = 0x0000; // Force a strict NULL terminator!
        dump_sector(my_buffer);
        */
    } else if (strcmp(cmd, "write"))
    {
        // 1. Get your LBA target
        uint32_t lba = atoi(argv[1]);

        // 2. Create a dedicated 512-byte sector buffer initialized to 0
        uint16_t disk_buffer[256] = {0}; 

        // 3. Safely copy your text into the sector buffer as characters
        char *dest = (char *)disk_buffer;
        int i = 0;

        // Copy until the end of the text string, but stop before overflowing the 512 bytes
        while (argv[2][i] != '\0' && i < 511) {
            dest[i] = argv[2][i];
            i++;
        }
        dest[i] = '\0'; // Ensure it's null-terminated if you want to read it back as text later

        // 4. Pass the safely padded 512-byte block to the hardware
        ata_write_sector(lba, disk_buffer);
    }
}   
#endif
