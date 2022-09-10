//focus on pid ALLOCATION and RECLAMATION
//implement all the task associated with pid maintenance and only then implement getpid().
//getpid return the process ID (PID) of the calling process. No errors returned.
#include <types.h>
#include <thread.h>
#include <curthread.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <syscall.h>

int sys_getpid( pid_t *retval )
{

    *retval = curproc->p_pidinfo->current_pid;

    return 0;

}