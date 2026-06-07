; yep, a file just for this
global gdt_flush
global ring3perm
extern stack_top
gdt_flush:
    mov eax, [esp + 4]  ; Get the pointer to gdt_ptr from the C argument
    lgdt [eax]          ; Load the GDT
    ret ; return right after for debugging porpuses

    jmp far 0x10:.flush     ; 0x10 is the offset to our code segment. Far jump!
.flush:
    mov ax, 0x18        ; 0x18 is the offset to our data segment
    mov ds, ax          ; Load all data segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;mov esp, stack_top
    ret

ring3perm:
    pushfd
    pop eax
    or eax, 0x3000
    push eax
    popfd
    ret
