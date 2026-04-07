.intel_syntax noprefix
.globl abi_add3
abi_add3:
    mov eax, edi
    add eax, esi
    add eax, edx
    ret
