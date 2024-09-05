#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

void run_target(const char *programname) {
  printf("target started. will run '%s'\n", programname);

  /* Allow tracing of this process */
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    perror("ptrace");
    return;
  }

  /* Replace this process's image with the given program */
  execl(programname, programname, NULL);
}

void run_debugger(pid_t child_pid) {
  int wait_status;
  printf("debugger started\n");

  /* Wait for child to stop on its first instruction */
  wait(&wait_status);
  struct user_regs_struct regs;

  unsigned addr = 0x40101e;
  /* You can't use unsigned descriptor to store the data */
  long data = ptrace(PTRACE_PEEKTEXT, child_pid, addr, 0);
  printf("Original data at addr 0x%08x: 0x%lx\n", addr, data);
  /* Make the child execute another instruction */

  // cause this long mask so danger :)
  unsigned data_with_trap = (data & 0xffffffffffffff00) | 0xcc;
  ptrace(PTRACE_POKETEXT, child_pid, addr, data_with_trap);

  unsigned data_after = ptrace(PTRACE_PEEKTEXT, child_pid, addr, 0);
  printf("After trap, data at addr 0x%08x: 0x%x\n", addr, data_after);

  ptrace(PTRACE_CONT, child_pid, 0, 0);
  wait(&wait_status);
  if (WIFSTOPPED(wait_status)) {
    printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
  } else {
    printf("Unexpected status: %d\n", wait_status);
  }

  printf("Now starting debugging...\n");
  if (WSTOPSIG(wait_status) == SIGTRAP) {
    // turn to original instruction
    ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
    printf("Hit breakpoint at address 0x%08llx\n", regs.rip - 1);
    ptrace(PTRACE_POKETEXT, child_pid, addr, data);
    regs.rip -= 1;
    printf("Current RAX: 0x%llx\n", regs.rax);
    ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

    unsigned data_restore = ptrace(PTRACE_PEEKTEXT, child_pid, addr, 0);
    printf("After restore, data at addr 0x%08x: 0x%x\n", addr, data_restore);
  } else {
    printf("Child got a signal: %d\n", WSTOPSIG(wait_status));
  }

  ptrace(PTRACE_CONT, child_pid, 0, 0);
  wait(&wait_status);
  if (WIFEXITED(wait_status)) {
    printf("Child exited\n");
  } else {
    printf("Unexpected status: %s\n", strsignal(WSTOPSIG(wait_status)));
  }
}

int main(int argc, char **argv) {
  pid_t child_pid;

  if (argc < 2) {
    fprintf(stderr, "Expected a program name as argument\n");
    return -1;
  }

  child_pid = fork();
  if (child_pid == 0)
    run_target(argv[1]);
  else if (child_pid > 0)
    run_debugger(child_pid);
  else {
    perror("fork");
    return -1;
  }

  return 0;
}
