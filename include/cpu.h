// cpu.h or isr.h
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically
} registers_t;

typedef struct {
    uint32_t flags;

    uint32_t mem_low;
    uint32_t mem_high;

    uint32_t boot_dev;
    uint32_t cmdline;
    
    uint32_t mods_count;
    uint32_t mods_addr;

    uint32_t sysms;
    
    uint32_t mmap_len;
    uint32_t mmap_addr;

    uint32_t config_table;
    uint32_t bootloader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

#endif