### Motivation

 A debugger can start some process and debug it, or attach itself to an existing process. It can single-step through the code, set breakpoints and run to them, examine variable values and stack traces. Many debuggers have advanced features such as executing expressions and calling functions in the debbugged process's address space, and even changing the process's code on-the-fly and watching the effects.

### Linux ptrace system call

one **tracer** process observe and control the execution of another **tracee** process

Primarily used to implement break point debugging and system call tracing.

“Tracee” always mean “one thread”, never “a (possibly multi-threaded) process”

Combined with `fork` and `execve` **we create a child process and execve programs in the child process** so that the address space separate, and the parent process can manage the common resources, since after `execve` system call, the process image will be replaced, including code,data and stacks…

#### Parent and Child process Way

the ptrace system call is below

```c
long ptrace(enum __ptrace_request request, pid_t pid,
                 void *addr, void *data);
```

##### Child process Side

The first argument *request* specifies the way how debuggers work

`PTRACE_TRACEME` request,

which means that this child process asks the OS kernel to let its parent trace it. 

What’s in `man 2 ptrace` 

> Indicates that this process is to be traced by its parent. Any signal (except SIGKILL) delivered to this process will cause it to stop and its parent to be notified via wait(). **Also, all subsequent calls to exec() by this process will cause a SIGTRAP to be sent to it, giving the parent a chance to gain control before the new program begins execution**. A process probably shouldn't make this request if its parent isn't expecting to trace it. (pid, addr, and data are ignored.)

So any afterward `exec` function, as the highlighted part explains, causes the OS kernel to stop the process just before it begins executing the program in `execl` and send a signal to the parent.

##### Parent process Side

Once the child starts executing the `exec` call, it will stop and be sent the `SIGTRAP` signal. The parent here waits for this to happen with the first `wait` call. 

When the parent checks that it was because the child was stopped, **most interesting part of this process** comes…

It invokes `ptrace` with the `PTRACE_SINGLESTEP` request giving it the child process ID. What this does is tell the OS - *please restart the child process, but stop it after it executes the next instruction*. 

Again, the parent waits for the child to stop and the loop continues. The loop will terminate when the signal that came out of the `wait` call wasn't about the child stopping. Most of the time, it’s SIGINT(键盘中断) or  SIGCHLD(程序正常退出) fault

#### Attaching to a running process

A thread can be attached to the tracer and turn into the tracee.

#### How much Instructions will be running?

With so tiny program, the LOI is enormous(more than 100,000…) 

**By default**,`gcc` on Linux links programs to the C runtime libraries dynamically. 

`libc.a` will automatically be transfered to the **linker**, the dynamic library loader looks for the required shared libs

#### More Information ptrace can get

use GETREGS macro to get the tracee’s regs

and use PEEKTEXT combined with regs.eip to get the current instruction

### Signals

**如果在Shell中没有信号，background执行的程序无法被父进程所回收**

* Will become zombies when they terminate 
* Will never be reaped because shell (typically) will not terminate
* Will create a memory leak that could run the kernel out of memory

底层的硬件异常是对用户不可见的

而信号是对用户可见的

**如**SIGINT信号发送给了**前台进程组的每一个进程** 

#### Signal Source

Signal is **sent from the kernel** (sometimes at the request of another process) to a process

#### Signal Semantics

It has **two steps** Sending Signal and Receiving Signal

#### Details

`SIGINT` 提供了进程一个清理和处理的机会，而 `SIGKILL` 则是强制立即终止进程，不允许任何处理或清理。

### How Break Point Works

There are two main pillars of debugging

1. Break Point to let program stop at specfic position
2. Inspect values in the debugged process’s memory such as Watch Point does

#### Software Interrupts

To handle asynchronous events like IO and hardware timers, the Interrupt mechansim is being used.

**A Hardware Interrupt** is usually a dedicated electrical signal to which a special “**response circuitry**” is attached.

> This circuitry notices an activation of the interrupt and makes the CPU stop its current execution, save its state, and jump to a predefined address where a handler routine for the interrupt is located. When the handler finishes its work, the CPU resumes execution from where it stopped.

In contrast, Software interrupts are caused by program itself usually.

 Such "traps" allow many of **the wonders of modern OSes** (task scheduling, virtual memory, memory protection, debugging) to be implemented efficiently.

#### Int 3 in theory

 `int` is x86 jargon for "trap instruction" - a call to a **predefined interrupt handler**.

With a 8-bit operand specifying the number of the interrupt that occured, so 256 traps are **in theory supported.** 

> The INT 3 instruction generates a special one byte opcode (CC) that is intended for calling the debug exception handler. (This one byte form is valuable because it can be used to replace the first byte of any instruction with a breakpoint, including other one byte instructions, without over-writing other code).

#### Int 3 in practice

==偷梁换柱==

```shell
gcc -nostartfiles -no-pie test.s -o test # to generate a minimum binary executable
```

 Real debuggers set breakpoints on lines of code and on functions, not on some bare memory addresses

Real debuggers covers symbols and debugging information, not in this part :)

To set a breakpoint at some target address in the traced process, **the debugger does** the following:

1. Remember the data stored at the target address
2. Replace the first byte at the target address with the int 3 instruction

When OS run this process, it hit `int 3` instruction where it stops and OS sends a signal caught by its parent process which is the **debugger** 

**Next: **

1. Replace the int 3 instruction at the target address with the original instruction
2. Roll the instruction pointer of the traced process back by one. This is needed because the instruction pointer now points *after* the `int 3`, having already executed it.
3. Allow the user to interact with the process in some way, since the process is still halted at the desired target address. This is the part where your debugger lets you peek at variable values, the call stack and so on.
4. When the user wants to keep running, the debugger will take care of placing the breakpoint back (since it was removed in step 1) at the target address, unless the user asked to cancel the breakpoint.

`Int` instructions on x86 occupy two bytes - 0xcd followed by the interrupt number [6]. int 3 could've been encoded as cd 03, but there's a special single-byte instruction reserved for it - 0xcc. **Very Important Design** 

注意数据按照小端存放，存放位置是在对应指令的首部，但按照逻辑运算是低位运算

**可执行文件的默认加载地址**:

- 在许多 Linux 系统中，`0x400000` 是可执行文件（尤其是 ELF 格式的文件）的默认加载地址。这意味着，当一个 ELF 可执行文件被加载到内存中时，操作系统的加载器（如 `ld-linux.so`）通常会将它加载到从 `0x400000` 开始的虚拟地址范围。这样做是为了确保可执行文件有足够的地址空间，并简化程序的地址空间布局。

#### How long the x86-64 architecture can be? 

**x86-64 指令的最大长度是 15 字节**，这是处理器硬件限制决定的。超过 15 字节的指令无效，处理器会引发异常。

When implementing the simply debugger, I ran into the **mask bug** that is so danger!!!! 

#### Modern OS work

**x86 (32位)**: 早期的 Linux 运行在 32 位的 x86 处理器上，这是最早支持的架构之一。

**x86-64 (64位)**: 64 位版本的 x86 架构，也称为 AMD64 或 x64，是目前最广泛使用的架构，支持64位处理器，运行大多数现代的桌面和服务器操作系统

==基本工作原理还是一致的== 

### References

*Peek* and *poke* are well-known system programming [jargon](http://www.jargon.net/jargonfile/p/peek.html) for directly reading and writing memory contents.

eli.thegreenplace.net 

