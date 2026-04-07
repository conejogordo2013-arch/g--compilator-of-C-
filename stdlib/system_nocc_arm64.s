.text
.global system_exit
system_exit:
    // Linux AArch64: exit(status)
    mov x8, #93
    svc #0
    ret

