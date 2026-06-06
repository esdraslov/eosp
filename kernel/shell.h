#ifndef SHELL_H
#define SHELL_H

#include "stdlib.h"
#include "ports.h"

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

void haltsys(void)
{
    printf("System halted, it's now safe to shutdown...");
    __asm__ __volatile__("cli");
    while(1)
    {
        __asm__ __volatile("hlt");
    }
}

void process_command(char *cmd)
{
    if (strcmp(cmd, "help") == 0)
    {
        printf("help - prints this text\n");
        printf("reboot - reboots the device");
        printf("halt - halts the system");
    } else if (strcmp(cmd, "reboot") == 0)
    {
        reboot();
    } else if (strcmp(cmd, "halt") == 0)
    {
        haltsys();
    }
}
#endif
