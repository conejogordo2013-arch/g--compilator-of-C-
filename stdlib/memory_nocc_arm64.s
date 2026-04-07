.bss
.align 4
heap_area:
    .skip 1048576
heap_end:
heap_ptr:
    .quad 0

.text
.global alloc
alloc:
    // x0 = size
    adrp x9, heap_ptr
    add  x9, x9, :lo12:heap_ptr
    ldr  x10, [x9]
    cbnz x10, 1f
    adrp x10, heap_area
    add  x10, x10, :lo12:heap_area
1:
    add  x11, x10, x0
    adrp x12, heap_end
    add  x12, x12, :lo12:heap_end
    cmp  x11, x12
    bhi  2f
    str  x11, [x9]
    mov  x0, x10
    ret
2:
    mov  x0, xzr
    ret

.global free
free:
    ret
