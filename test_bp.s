# use command `gcc -nostartfiles -no-pie test_bp.s -o test_bp`
# to generate a minimum binary executable file

.section .data
msg:
    .asciz "Hello,\n"
msg2:
    .asciz "world!\n"

.section .text
.global _start
_start:
    # Print the first message
    mov $1, %rax        # system call number (sys_write)
    mov $1, %rdi        # file descriptor (stdout)
    lea msg(%rip), %rsi # pointer to string
    mov $7, %rdx        # length of first string
    syscall

    # Print the second message
    mov $1, %rax        # system call number (sys_write)
    mov $1, %rdi        # file descriptor (stdout)
    lea msg2(%rip), %rsi # pointer to string
    mov $7, %rdx        # length of second string
    syscall

    # Exit the program
    mov $60, %rax       # system call number (sys_exit)
    xor %rdi, %rdi      # exit code 0
    syscall

