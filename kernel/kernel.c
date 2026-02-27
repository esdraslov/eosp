#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ports.h"

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#define KEYBOARD_DATA 0x60

// IRS methods
extern void isr0();
extern void isr1();

// IDT
struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t flags;
	uint16_t offset_high;
} __attribute__((packed));

struct idt_entry idt[256];

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct idt_ptr idtp;

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

// Helper methods
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void idt_set_gate(int n, uint32_t handler)
{
	idt[n].offset_low = handler & 0xFFFF;
	idt[n].selector = 0x08;
	idt[n].zero = 0;
	idt[n].flags = 0x8E;
	idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void io_wait()
{
	outb(0x80, 0);
}

void u32_str(uint32_t value, char *buffer)
{
	char temp[11];
	int i = 0;

	if (value == 0)
	{
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	while (value > 0)
	{
		temp[i++] = '0' + (value % 10);
		value /= 100;
	}

	int j = 0;
	while (i > 0)
	{
		buffer[j++] = temp[--i];
	}

	buffer[j] = '\0';
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
	if (c == '\n') {
		terminal_row++;
		terminal_column = 0;

		if (terminal_row == VGA_HEIGHT)
		{
			terminal_row = 0;
		}
		return;
	}
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}

extern void picremap();

void kernel_main(void) 
{
	terminal_initialize();
	// Set IDT
	__asm__ __volatile__("cli"); // Disable interruptions (maybe important?)
	outb(0x20, 0xFF);
	idtp.limit = sizeof(idt) - 1;
	idtp.base = (uint32_t)&idt;
	idt_set_gate(33, (uint32_t)isr1);
	idt_set_gate(32, (uint32_t)isr0);
	__asm__ __volatile__("lidt %0" : : "m"(idtp));
	picremap(); // this also remaps the PIC
	__asm__ __volatile__("sti"); // Enable interruptions (VERY IMPORTANT)

	// Post-boot
	char buffer;
	terminal_writestring("EOSP booted successfully\n");
	u32_str((uint32_t)inb(0x21), &buffer);
	terminal_writestring(&buffer);
}

void isr1_handler()
{
	//uint8_t scancode = inb(KEYBOARD_DATA);

	terminal_writestring("IRQ 1 actioned");

	/*
	if (scancode & 0x80)
	{}
	else
	{}*/
}
