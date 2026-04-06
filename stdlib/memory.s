.intel_syntax noprefix
.bss
.align 16
heap_area:
    .skip 1048576
heap_ptr:
    .quad 0

.text
.globl alloc
alloc:
    push rbp
    mov rbp, rsp
    mov rax, QWORD PTR heap_ptr[rip]
    test rax, rax
    jne .have_ptr
    lea rax, [rip + heap_area]
.have_ptr:
    mov rcx, rax
    add rcx, rdi
    lea rdx, [rip + heap_area + 1048576]
    cmp rcx, rdx
    ja .oom
    mov QWORD PTR heap_ptr[rip], rcx
    leave
    ret
.oom:
    xor rax, rax
    leave
    ret

.globl free
free:
    ret
