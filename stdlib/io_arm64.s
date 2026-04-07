.section .rodata
fmt_println:
    .asciz "%ld\n"
fmt_scanln:
    .asciz "%ld"
fmt_bool_true:
    .asciz "true\n"
fmt_bool_false:
    .asciz "false\n"
fmt_hex:
    .asciz "0x%lx\n"

.text
.extern printf
.extern scanf
.global println
println:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov x1, x0
    adrp x0, fmt_println
    add x0, x0, :lo12:fmt_println
    bl printf
    ldp x29, x30, [sp], #16
    ret

.global scanln
scanln:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    sub sp, sp, #16
    adrp x0, fmt_scanln
    add x0, x0, :lo12:fmt_scanln
    mov x1, sp
    bl scanf
    ldr x0, [sp]
    add sp, sp, #16
    ldp x29, x30, [sp], #16
    ret

.global print_bool
print_bool:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    cmp x0, #0
    bne 1f
    adrp x0, fmt_bool_false
    add x0, x0, :lo12:fmt_bool_false
    b 2f
1:
    adrp x0, fmt_bool_true
    add x0, x0, :lo12:fmt_bool_true
2:
    bl printf
    ldp x29, x30, [sp], #16
    ret

.global print_hex
print_hex:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov x1, x0
    adrp x0, fmt_hex
    add x0, x0, :lo12:fmt_hex
    bl printf
    ldp x29, x30, [sp], #16
    ret
