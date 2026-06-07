#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    // We cast port to a 32-bit integer internally to force a native 32-bit register selection
    uint32_t port32 = port;
    __asm__ volatile ("outb %b0, %w1" : : "a"(val), "d"(port32));
}

static inline uint8_t inb(uint16_t port) {
    uint32_t port32 = port;
    uint8_t ret;
    __asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "d"(port32));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint32_t port32 = port;
    uint16_t ret;
    __asm__ volatile ("inw %w1, %w0" : "=a"(ret) : "d"(port32));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    // We cast port to a 32-bit integer internally to force a native 32-bit register selection
    uint32_t port32 = port;
    __asm__ volatile ("outw %w0, %w1" : : "a"(val), "d"(port32));
}

#endif
