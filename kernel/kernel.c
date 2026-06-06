#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ports.h"
#include "stdlib.h"
#include "shell.h"

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#define KEYBOARD_DATA 0x60

// IRS methods
extern void isr0();
extern void isr1();

// GDT methods
extern void gdt_flush(uint32_t ptr);

// GDT
struct gdt_entry {
    uint16_t limit_low;     // Lower 16 bits of the limit
    uint16_t base_low;      // Lower 16 bits of the base
    uint8_t  base_middle;   // Next 8 bits of the base
    uint8_t  access;        // Access flags (Ring level, executable, etc.)
    uint8_t  granularity;   // Limit high nibble and flags (Size, Granularity)
    uint8_t  base_high;     // Last 8 bits of the base
} __attribute__((packed));
struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct gdt_ptr gdtp;
struct gdt_entry gdt[3];

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

void idt_set_gate(int n, uint32_t handler)
{
	idt[n].offset_low = handler & 0xFFFF;
	idt[n].selector = 0x10;
	idt[n].zero = 0;
	idt[n].flags = 0x8E;
	idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].limit_low    = (limit & 0xFFFF);
    gdt[num].base_low     = (base & 0xFFFF);
    gdt[num].base_middle  = (base >> 16) & 0xFF;
    gdt[num].access       = access;
    gdt[num].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].base_high    = (base >> 24) & 0xFF;
}

// who knows if I'll need this later
void init_gdt() {
	//while(1) {};
    gdtp.limit = (sizeof(struct gdt_entry) * 3) - 1;
	// gdtp.limit = 23;
    gdtp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment

	terminal_writestring("GDT entries:\n");
	for (int i = 0; i < 3; i++)
	{
		char buffer[16];
		itoa(i, buffer);
		terminal_writestring("gdt entry ");
		terminal_writestring(buffer);// not sure if concatinating works :/
		terminal_writestring(": ");

		uint8_t *entry = (uint8_t*)(gdtp.base + i * 8);
		for (int j = 0; j < 8;j++)
		{
			char buf[16]; // I still dont know why I always put 16 when other sizes probably works
			itoa(entry[j], buf);
			terminal_writestring(buf);
			terminal_writestring(" ");
		}
		terminal_writestring("\n");
	}
	//while(1) {}

    gdt_flush((uint32_t)&gdtp);
}

// not really useful anymore
// void io_wait()
// {
// 	outb(0x80, 0);
// }

extern void picremap();

char cmd[256];
int pos = 0;

void kernel_main(void) 
{
	terminal_initialize();
	// Set IDT and GDT
	__asm__ __volatile__("cli"); // Disable interruptions (maybe important?)
	//init_gdt(); // initialize the GDT no rmore
	outb(0x20, 0xFF);
	idtp.limit = sizeof(idt) - 1;
	idtp.base = (uint32_t)&idt;
	idt_set_gate(33, (uint32_t)isr1);
	idt_set_gate(32, (uint32_t)isr0);
	__asm__ __volatile__("lidt %0" : : "m"(idtp));
	picremap(); // this remaps the PIC
	__asm__ __volatile__("sti"); // Enable interruptions (VERY IMPORTANT)

	// Post-boot
	printf("EOSP booted successfully\n");
	printf("> ");

	while(1)
	{}

	// I was going to fire and halt here, but I remembered that I don't need to cuz 
	// boot.asm does that at the end of the kernel
	// while(1) {}
}

void isr1_handler()
{
	uint8_t scancode = inb(KEYBOARD_DATA);

	// terminal_writestring("IRQ 1 actioned");

	if (scancode & 0x80)
	{
		return;
	}

	static const char scancode_to_ascii[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j',
        'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };

	if (scancode < sizeof(scancode_to_ascii))
	{
		char c = scancode_to_ascii[scancode];

		if (c == '\n')
		{
			cmd[pos] = '\0';
			printf("\n");
			process_command(cmd);
			pos = 0;
			printf("> ");
		} else if (c == '\b')
		{
			// still thinking on how to implement
		} else if (c != 0)
		{
			cmd[pos++] = c;
			printf("%c", c);
		}
	}
}
