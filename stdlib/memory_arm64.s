.text
.extern malloc
// `free` is provided directly by libc on AArch64.

.global alloc
alloc:
    b malloc
