CC = clang
CFLAGS = -m32 -ffreestanding -fno-stack-protector -O0 -Wall -Wextra -target i686-elf -fno-pie -no-pie
LDFLAGS = -m elf_i386

ASM = nasm

OBJS = boot.o kernel.o isrtable.o

all: os.iso

boot.o:
	$(ASM) -f elf32 kernel/boot.asm -o boot.o

isrtable.o:
	$(ASM) -f elf32 kernel/isrtable.asm -o isrtable.o

kernel.o:
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -T linker.ld -o kernel.bin $(OBJS)

os.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp boot/grub.cfg iso/boot/grub/
	grub-mkrescue -o os.iso iso

clean:
	rm -rf *.o *.bin iso os.iso
