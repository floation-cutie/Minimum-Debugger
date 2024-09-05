.section .data
msg:
    .ascii "Hello, world!\n"

.section .text
.global _start
_start:
    mov $1, %rax        # system call number (sys_write)
    mov $1, %rdi        # file descriptor (stdout)
    lea msg(%rip), %rsi # pointer to string
    mov $14, %rdx       # length of string
    syscall

    mov $60, %rax       # system call number (sys_exit)
    xor %rdi, %rdi      # exit code 0
    syscall

