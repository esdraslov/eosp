#include "time.h"

uint32_t sys_timestamp;
uint32_t uptime;

uint8_t bcd_int(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}

void get_rtc_time(time_t *t)
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
    uint8_t y = bcd_int(inb(0x71));
    outb(0x70, 0x32);
    uint8_t c = bcd_int(inb(0x71));

    t->year = c * 100 + y;
}

const int d_month[12] = { 31, 28, 31, 30, 
    31, 30, 31, 31, 30, 31, 30, 31 };

uint32_t time_t_to_timestamp(time_t *t)
{
    uint32_t days = 0;

    for (uint32_t i = 1970;i < t->year;i++)
    {
        days += leap_year(i) ? 366 : 365;
    }

    for (int i = 0; i < t->month -1; i++)
    {
        days += d_month[i];
    }
    if (t->month >= 2 && leap_year(t->year))
        days++;

    days += t->day -1;

    return (days * 86400) +
        (t->hours * 3600) +
        (t->minutes * 60) +
        t->seconds;
}

void program_pit(uint32_t freq)
{
    uint32_t d = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(d & 0xFF));
    outb(0x40, (uint8_t)((d >> 8) & 0xFF));
}

void set_time(uint32_t timestamp)
{
    sys_timestamp = timestamp;
}

void inc_uptime()
{
    uptime++;
}

void timestamp_to_time_t(uint32_t timestamp, time_t *t) {
    // 1. Extract the time within the current day
    uint32_t seconds_in_day = timestamp % 86400;
    uint32_t total_days     = timestamp / 86400;

    t->seconds = seconds_in_day % 60;
    t->minutes = (seconds_in_day / 60) % 60;
    t->hours   = seconds_in_day / 3600;

    // 2. Determine the Year
    t->year = 1970;
    while (1) {
        uint32_t days_in_this_year = leap_year(t->year) ? 366 : 365;
        if (total_days < days_in_this_year) {
            break; // We found the correct year!
        }
        total_days -= days_in_this_year;
        t->year++;
    }

    // 3. Determine the Month
    t->month = 1; // Start at January (1-indexed)
    for (int m = 0; m < 12; m++) {
        uint32_t days_in_this_month = d_month[m];
        
        // Adjust for February if it's a leap year
        if (m == 1 && leap_year(t->year)) {
            days_in_this_month = 29;
        }

        if (total_days < days_in_this_month) {
            break; // We found the correct month!
        }
        total_days -= days_in_this_month;
        t->month++;
    }

    // 4. Determine the Day
    // (Add 1 because total_days left over is 0-indexed, but calendar days start at 1)
    t->day = total_days + 1;
}
