#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

__attribute__((always_inline)) static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ __volatile__("inb %%dx, %%al" : "=a"(result) : "d"(port));
    return result;
}

__attribute__((always_inline)) static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("out %%al, %%dx" : : "a"(value), "d"(port));
}

__attribute__((always_inline)) static inline uint16_t inw(uint16_t port)
{
    uint16_t result;
    __asm__ __volatile__("inw %%dx, %%ax" : "=a"(result) : "d"(port));
    return result;
}

#endif
