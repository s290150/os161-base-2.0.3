#include <syscall.h>
#include <pid.h>
#include <types.h>
#include <kern/limits.h>
#include <arch/mips/include/trapframe.h>
#include <lib.h>
#include <addrspace.h>
#include <current.h>
#include <proc.h>
#include <errno.h>
#include <file.h>
#include <thread.h>
#include <trap.h>
#include <synch.h>

int sys_waitpid( pid_t pid, userptr_t status, int option, pid_t *retval ) {

    // We need to recover the proc structure using the pid associated to the process that
    // we are waiting. So we need a processtable that take into account all the processes
    // (basically the same we did for filetable)

}