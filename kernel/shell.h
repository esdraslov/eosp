#ifndef SHELL_H
#define SHELL_H

#include "stdlib.h"
#include "ports.h"
#include "atapio.h"

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

void process_command(char *cmd)
{
    if (strcmp(cmd, "help") == 0)
    {
        printf("help - prints this text\n");
        printf("reboot - reboots the device\n");
        printf("halt - halts the system\n");
        printf("read - reads the first sector of lba 0 and prints the result\n");
    } else if (strcmp(cmd, "reboot") == 0)
    {
        reboot();
    } else if (strcmp(cmd, "halt") == 0)
    {
        haltsys();
    } else if (strcmp(cmd, "read") == 0)
    {
        uint16_t buffer[512];
        ata_read_sector(0, buffer);
        dump_sector(buffer);
        /*
        for(int i=0; i<255; i++) my_buffer[i] = 0x4141; // 'AA'
        my_buffer[255] = 0x0000; // Force a strict NULL terminator!
        dump_sector(my_buffer);
        */
    }
}
#endif
