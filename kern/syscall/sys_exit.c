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

void sys__exit( int status ) {

    struct proc *p = curproc;

    curproc->p_pid->exit_status = status;
    curproc->p_pid->exit = true;

    proc_remthread(curthread); //Detach of the current thread from the process that calls the exit

    // All the files opened by this process need to be closed
    // But the ones shared with other processes not. What does it means exactly?

    V(p->p_sem); // This semaphore is put high to be used by the waitpid() system call

    thread_exit()

}