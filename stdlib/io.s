.intel_syntax noprefix
.section .rodata
fmt_println:
    .string "%lld\n"
fmt_scanln:
    .string "%lld"

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
