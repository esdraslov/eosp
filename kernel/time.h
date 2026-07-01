#ifndef TIME_H
#define TIME_H
#include <stdint.h>
#include "ports.h"

uint8_t bcd_int(uint8_t bcd);

struct rtc_time {
    uint8_t seconds, minutes, hours, day, month;
    uint32_t year;
};

void get_rtc_time(struct rtc_time *t);

#endif
