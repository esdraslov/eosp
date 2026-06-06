; yep, a file just for this
global gdt_flush
extern stack_top
gdt_flush:
    mov eax, [esp + 4]  ; Get the pointer to gdt_ptr from the C argument
    lgdt [eax]          ; Load the GDT

    push 0x08
    push .flush
    retf  ; far return
.flush:
    mov ax, 0x10        ; 0x10 is the offset to our data segment
    mov ds, ax          ; Load all data segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;mov esp, stack_top
    ret