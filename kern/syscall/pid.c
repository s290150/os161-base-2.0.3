//Here we define all the functions that operate on pid structure or process_table structure
#include <pid.h>
#include <proc.h>
#include <types.h>
#include <proc.h>
#include <kern/limits.h>
#include <err.h>
#include <synch.h>

int pid_init ( struct pid *p ) {

    //I'm a little bit not sure on the use of curproc to refer to the new process created
    //Maybe I can pass it as an argument (the pid structure of the new proc)

    pid_t newpid = 0;

    KASSERT( curproc->p_pid == NULL );

    p = kmalloc(sizeof(struct pid));

    if ( p == NULL) {
        return ENOMEM;
    }

    p->p_lock = lock_create("pid_lock");

    if ( p->p_lock == NULL ) {
        return ENOMEM;
    }

    p->parent_pid = NULL;
    p->exit_status = 0;
    p->exit = false;

    newpid = get_newpid();

    /* if ( newpid == 0 && curproc->p_processtable->n_active_processes <= __PID_MAX ) {
        newpid = curproc->p_processtable->n_active_processes + 1;
        curproc->p_processtable->n_active_processes++;
    } else {
        return ENOMEM;
    } */

    p->current_pid = newpid;

    return 0;

}

processtable * proctable_init() {

    curproc->p_processtable = kmalloc(sizeof(struct processtable)* __PROC_MAX);

    KASSERT(curproc->p_processtable != NULL);

    curproc->p_processtable->pt_lock = lock_create("pt_lock");

    KASSERT(curproc->p_processtable->pt_lock != NULL);

    for ( int pid = 0; pid < __PROC_MAX; pid++ ) {
        curproc->p_processtable->pid_ptr[pid] = NULL;
    }

}

void processtable_placeproc(struct proc *p, pid_t pid) {

    lock_acquire(curproc->p_processtable->pt_lock);

    curproc->p_processtable->pid_ptr[pid] = p;

    lock_release(curproc->p_processtable->pt_lock);

}

pid_t get_newpid () {

    pid_t newpid;
    int ret;

    lock_acquire(curproc->p_processtable->pt_lock);

    for ( pid_t i = __PID_MIN; i < __PID_MAX; i++ ) {
            if ( cuproc->p_processtable->pid_ptr[i] == NULL ) {
                pt->n_active_processes++;
                return i;
            }
    }

    lock_release(curproc->p_processtable->pt_lock);

    return ENOMEM;

}

proc proc_search_pid ( pid_t pid ) {

    return curproc->p_processtable->pid_ptr[pid];

}

void proc_wait ( struct proc *process ) {

    P(process->end_sem);

    pid_destroy(process->p_pid);

    // If I perform the proc_destroy, I destroy also all the struct allocated dinamically with it?
    // (filetable and pid table)

    proc_destroy(process);

}

void pid_destroy( struct pid *pid ) {

    curproc->p_processtable->pid_ptr[pid->current_pid] = NULL;

}

