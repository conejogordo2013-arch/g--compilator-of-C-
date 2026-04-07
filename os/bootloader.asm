; bootloader.asm - 512-byte BIOS boot sector (real mode)
; Basic stage-1 loader for the C! OS prototype.
; It prints a boot banner and then halts.

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, boot_msg
.print_loop:
    lodsb
    test al, al
    jz .done_print
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .print_loop

.done_print:
    mov si, halt_msg
.halt_loop_msg:
    lodsb
    test al, al
    jz .hang
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .halt_loop_msg

.hang:
    cli
    hlt
    jmp .hang

boot_msg db "C! OS: stage1 bootloader online", 13, 10, 0
halt_msg db "Kernel loading stub not wired yet.", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
