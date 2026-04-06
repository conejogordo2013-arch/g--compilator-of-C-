.intel_syntax noprefix
.text
.globl system_exit
system_exit:
    mov eax, 60
    mov edi, edi
    syscall
    ret
