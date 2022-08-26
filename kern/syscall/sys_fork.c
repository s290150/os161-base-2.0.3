#include <pid.h>
#include <types.h>
#include <kern/limits.h>
#include <arch/mips/include/trapframe.h>
#include <lib.h>
#include <addrspace.h>
#include <current.h>
#include <proc.h>
#include <errno.h>

int sys_fork( struct trapframe *tf, pid_t* retval ) {

    pid_t new_pid;
    struct trapframe *tf_new;
    struct addrspace *addr_new = NULL;
    struct proc *new_proc = NULL;
    //struct thread *curt = curthread;
    //struct proc *curp = curt->t_proc;
    char pid_name[11];
    int ret;

    spinlock_acquire(curproc->p_lock);

    if ( curproc->p_processtable->n_active_processes >= __PID_MAX ) { //Too many active processes
        return ENPROC;
    }

    // Need to understande if the name can be equal for all processes that we want to create
    //Maybe we can use curproc->p_name.

    new_proc = proc_create_runprogram(pid_name);
    if ( new_proc == NULL ) {
        return ENOMEM;
    }

    new_proc->p_pid->parent_pid = curproc->p_pid->current_pid;

    //Create the new pid

    new_pid = 


}