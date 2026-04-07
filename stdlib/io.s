.intel_syntax noprefix
.section .rodata
fmt_println:
    .string "%lld\n"
fmt_scanln:
    .string "%lld"
fmt_bool_true:
    .string "true\n"
fmt_bool_false:
    .string "false\n"
fmt_hex:
    .string "0x%llx\n"

.text
.extern printf
.extern scanf
.globl println
println:
    push rbp
    mov rbp, rsp
    mov rsi, rdi
    lea rdi, [rip + fmt_println]
    xor eax, eax
    call printf
    pop rbp
    ret

.globl scanln
scanln:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    lea rdi, [rip + fmt_scanln]
    lea rsi, [rbp - 8]
    xor eax, eax
    call scanf
    mov rax, QWORD PTR [rbp - 8]
    leave
    ret

.globl print_bool
print_bool:
    push rbp
    mov rbp, rsp
    cmp rdi, 0
    jne .Lpb_true
    lea rdi, [rip + fmt_bool_false]
    xor eax, eax
    call printf
    pop rbp
    ret
.Lpb_true:
    lea rdi, [rip + fmt_bool_true]
    xor eax, eax
    call printf
    pop rbp
    ret

.globl print_hex
print_hex:
    push rbp
    mov rbp, rsp
    mov rsi, rdi
    lea rdi, [rip + fmt_hex]
    xor eax, eax
    call printf
    pop rbp
    ret
