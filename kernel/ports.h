#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ __volatile__("in %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("out %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint8_t result;
    __asm__ __volatile__("inw %%dx, %%ax" : "=a"(result) : "d"(port));
    return result;
}

#endif
