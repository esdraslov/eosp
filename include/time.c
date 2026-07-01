#include "time.h"

uint8_t bcd_int(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}

void get_rtc_time(struct rtc_time *t)
{
    outb(0x70, 0x0A);
    while (inb(0x71) & 0x80) {};

    outb(0x70, 0x00);
    t->seconds = bcd_int(inb(0x71));
    outb(0x70, 0x02);
    t->minutes = bcd_int(inb(0x71));
    outb(0x70, 0x04);
    t->hours = bcd_int(inb(0x71));
    outb(0x70, 0x07);
    t->day = bcd_int(inb(0x71));
    outb(0x70, 0x08);
    t->month = bcd_int(inb(0x71));
    outb(0x70, 0x09);
    t->year = bcd_int(inb(0x71));
}
