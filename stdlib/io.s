.intel_syntax noprefix
.text
.globl println
println:
    # Placeholder stdlib IO ABI bridge.
    ret

.globl scanln
scanln:
    mov rax, 0
    ret
