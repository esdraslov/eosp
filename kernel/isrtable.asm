section .text
; "globaling" remap method
global picremap
; "globaling" IRS'
global isr1 ; Keyboard IRQ
global isr0 ; Timer IRQ (very useless)

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


picremap:
    mov al, 0x11 ; IWC 1
    out 0x20, al
    out 0xA0, al

    mov al, 0x20 ; IWC 2
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    mov al, 0x4 ; IWC 3
    out 0x21, al
    mov al, 0x2
    out 0xA1, al

    mov al, 0x1 ; IWC 4
    out 0x21, al
    out 0xA1, al

    mov al, 0x0 ; Unmasking
    out 0x21, al
    out 0xA1, al
