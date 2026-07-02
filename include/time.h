#ifndef TIME_H
#define TIME_H
#include <stdint.h>
#include <stdbool.h>
#include "ports.h"

extern uint32_t sys_timestamp;
extern uint32_t uptime;

uint8_t bcd_int(uint8_t bcd);

static inline bool leap_year(uint32_t y)
{
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

typedef struct {
    uint8_t seconds, minutes, hours, day, month;
    uint32_t year;
} time_t;

void get_rtc_time(time_t *t);

extern const int d_month[12];

uint32_t time_t_to_timestamp(time_t *t);

void program_pit(uint32_t freq);

void set_time(uint32_t timestamp);

void inc_uptime();

void timestamp_to_time_t(uint32_t timestamp, time_t *t);

#endif
