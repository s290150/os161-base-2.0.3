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

    /* From here */

    if ( curproc->p_processtable->n_active_processes >= __PID_MAX ) { //Too many active processes
        return ENPROC;
    }

    // To here, there's a search for an error but it need to be re evaluated when we find a final
    // place for the processtable.

    // Need to understande if the name can be equal for all processes that we want to create
    //Maybe we can use curproc->p_name.

    new_proc = proc_create_runprogram(curproc->p_name);
    if ( new_proc == NULL ) {
        return ENOMEM;
    }

    /* From here */

    ret = pid_init(new_proc);

    if ( ret ) {
        return ret;
    }

    new_proc->p_pid->parent_pid = curproc->p_pid->current_pid;

    // To here, there is a control related to the creation of the new pid
    // and new associations.

    // We allocate a new trapframe and copy it into the new one

    tf_new = kmalloc(sizeof(struct trapframe));

    if ( tf_new == NULL ) {
        proc_destroy(new_proc);
        return ENOMEM;
    }

    memcpy(tf_new, tf, sizeof(*tf));

    // Create a new address space and copy the old one (parent process)
    // We didn't modify as_copy for VM because it's not our task

    ret = as_copy(curproc->p_addrspace, addr_new);

    if ( ret ) {
        kfree(tf_new);
        proc_destroy(new_proc);
        return ret;
    }

    memcpy(new_proc->p_addrspace, addr_new, sizeof(struct addrspace));

    // I copy the filetable

    memcpy(new_proc->p_filetable, curproc->p_filetable, sizeof(struct filetable)); //

    // Create a new thread and attach it to the new proc (copying the trapframe
    // of the old process to it).

    ret = thread_fork( curthread->t_name, new_proc, &enter_forked_process, tf_new, NULL);

    if ( ret ) {
        as_destroy(addr_new);
        kfree(tf_new);
        proc_destroy(new_proc);
        return ret;
    }

    //This is the return for the parent

    *retval = new_proc->p_pid->current_pid;

    // This is the return for the child

    return 0;

}