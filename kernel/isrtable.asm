section .text
; "globaling" remap method
global picremap
; "globaling" IRS'
global isr1 ; Keyboard IRQ
global isr0 ; Timer IRQ (very useless)
global gpisr ; #GP IRQ

; does nothing rn
isr0:
    pushad
    mov al, 20h
    out 20h, al
    popad
    iret

isr1:
    pushad
    extern isr1_handler
    call isr1_handler
    mov al, 20h
    out 20h, al
    popad
    iret

gpisr:
    pushad

    mov ax, ds ; load the data segment
    push ax ; dump it
    ;repeat with other registers

    mov ax, 0x10 ; before that, load all data segment registers because apparently yes
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp ; final step FOR NOW
    extern fault_handler
    call fault_handler

    ; now do nothing because the handler will halt and fire

picremap:
    mov al, 0x11 ; IWC 1
    out 0x20, al
    mov al, 0
    out 0x80, al
    mov al, 0x11
    out 0xA0, al

    mov al, 0
    out 0x80, al

    mov al, 0x20 ; IWC 2
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 0x28
    out 0xA1, al

    mov al, 0
    out 0x80, al

    mov al, 0x4 ; IWC 3
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 0x2
    out 0xA1, al

    mov al, 0
    out 0x80, al

    mov al, 0x1 ; IWC 4
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 1
    out 0xA1, al

    mov al, 0
    out 0x80, al

    mov al, 0x0 ; Unmasking
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 0
    out 0xA1, al
    ret
