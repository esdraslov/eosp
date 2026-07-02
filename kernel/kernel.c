#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ports.h"
#include "stdlib.h"
#include "shell.h"
#include "cpu.h"
#include "atapio.h"
#include "time.h"

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#define KEYBOARD_DATA 0x60

// IRS methods
extern void isr0();
extern void isr1();
extern void gpisr();

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
// struct gdt_ptr {
// 	uint16_t limit;
// 	uint32_t base;
// } __attribute__((packed));

struct gdt_ptr gdtp __attribute__((aligned(16)));
struct gdt_entry gdt[4] __attribute__((aligned(16)));

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
// void init_gdt() {
// 	//while(1) {};
//     gdtp.limit = (sizeof(struct gdt_entry) * 4) - 1;
// 	// gdtp.limit = 23;
//     gdtp.base  = (uint32_t)&gdt;

//     gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
// 	gdt_set_gate(1, 0, 0, 0, 0);                // unused
//     gdt_set_gate(2, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
//     gdt_set_gate(3, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment

//     gdt_flush((uint32_t)&gdtp);
// }

// not really useful anymore
// void io_wait()
// {
// 	outb(0x80, 0);
// }

// We use a raw uint64_t array to bypass struct alignment entirely!
uint64_t raw_gdt[4] __attribute__((aligned(16)));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtp __attribute__((aligned(16)));

void init_gdt() {
    // Hardcoded raw x86 descriptors for a flat 4GB model
    raw_gdt[0] = 0x0000000000000000ULL; // Null Descriptor
    raw_gdt[1] = 0x0000000000000000ULL; // Unused / Dummy Slot
    raw_gdt[2] = 0x00CF9A000000FFFFULL; // Code Segment (Base 0, Limit 4GB, Selector 0x10)
    raw_gdt[3] = 0x00CF92000000FFFFULL; // Data Segment (Base 0, Limit 4GB, Selector 0x18)

    gdtp.limit = (sizeof(uint64_t) * 4) - 1; // Exactly 31 bytes
    gdtp.base  = (uint32_t)&raw_gdt;

    gdt_flush((uint32_t)&gdtp);
}


extern void picremap();
extern void ring3perm();

volatile char cmd[256];
volatile int pos = 0;
volatile bool pending = false;

volatile bool shift_state = false;
volatile bool caps_state = false;

void kernel_main(void) 
{
	terminal_initialize();
	// Set IDT and GDT
	__asm__ __volatile__("cli"); // Disable interruptions (maybe important?)
	init_gdt(); // initialize the GDT
	//ring3perm(); // patching
	outb(0x20, 0xFF);
	idtp.limit = sizeof(idt) - 1;
	idtp.base = (uint32_t)&idt;
	idt_set_gate(33, (uint32_t)isr1);
	idt_set_gate(32, (uint32_t)isr0);
	idt_set_gate(13, (uint32_t)gpisr); // General Protection Fault thingy
	__asm__ __volatile__("lidt %0" : : "m"(idtp));
	picremap(); // this remaps the PIC
	__asm__ __volatile__("sti"); // Enable interruptions (VERY IMPORTANT)
	program_pit(1000); // reprogram the PIT to fire an interrupt every 1000 Hz

	time_t t; // set up the clock
	get_rtc_time(&t);
	uint32_t s = time_t_to_timestamp(&t);
	set_time(s);

	// Post-boot
	printf("EOSP booted successfully\n");
	printf("> ");

	while(1)
	{
		if (pending)
		{
			process_command(cmd);
			pending = false;
			printf("> ");
		}
	}

	// I was going to halt and fire here, but I remembered that I don't need to cuz 
	// boot.asm does that at the end of the kernel
	// while(1) {}
}

// #GP or general protection fault
void fault_handler(registers_t regs)
{
	//terminal_initialize(); // reinitialize the terminal (aka clean it)
	terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
	printf("---- #GP ----\n");
	printf("something bad happened...\n\ndump:\n");
	printf("ERR_CODE: %d\n", regs.err_code);
	printf("INT_NUM: %d EIP: %d", regs.int_no, regs.eip);
	while(1) {__asm__ __volatile__("hlt");} // HALT THE SYSTEM
}

void isr1_handler()
{
	uint8_t scancode = inb(KEYBOARD_DATA);

	// terminal_writestring("IRQ 1 actioned");

	switch(scancode)
	{
		case 0x2A:
		case 0x36:
			shift_state = true;
			break;

		case 0xAA:
		case 0xB6:
			shift_state = false;
			break;

		case 0x3A:
			caps_state = !caps_state;
			break;
	}

	if (scancode & 0x80)
	{
		return;
	}

	static const char sta[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j',
        'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };
	static const char staS[] = {
		0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}', '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J',
        'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', '<', '>', '?', 0, '*', 0, ' '
	};
	static const char staC[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
        'O', 'P', '[', ']', '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J',
        'K', 'L', ';', '\'', '`', 0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', ',', '.', '/', 0, '*', 0, ' '
    };
	static const char staSC[] = {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        '_', '+', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '{', '}', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j',
        'k', 'l', ':', '"', '~', 0, '|', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };

	if (scancode < sizeof(sta))
	{
		char c = shift_state ? caps_state ? staSC[scancode] : staS[scancode] : caps_state ? staC[scancode] : sta[scancode];

		if (c == '\n')
		{
			cmd[pos] = '\0';
			printf("\n");
			// process_command(cmd);
			pending = true;
			pos = 0;
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

void timer_handler()
{
	inc_uptime();
	if (uptime % 1000 == 0)
	{
		set_time(sys_timestamp+1);
	}
}
