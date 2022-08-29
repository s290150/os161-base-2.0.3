//Here we define all the functions that operate on pid structure or process_table structure
#include <pid.h>
#include <proc.h>
#include <types.h>
#include <proc.h>
#include <limits.h>
#include <err.h>

pid *pid_init ( struct pid *p ) {

    //I'm a little bit not sure on the use of curproc to refer to the new process created
    //Maybe I can pass it as an argument (the pid structure of the new proc)

    //KASSERT( curproc->p_pid == NULL );
    pid_t newpid = 0;

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

    for ( int i = __PID_MIN; i < curproc->p_processtable->n_active_processes; i++ ) {
        if ( curproc->p_processtable->used_pid[i] == 0 ) {
            newpid = i;
            curproc->p_processtable->used_pid[i] = 1;
            break;
        }
    }

    if ( newpid == 0 && curproc->p_processtable->n_active_processes <= __PID_MAX ) {
        newpid = curproc->p_processtable->n_active_processes + 1;
        curproc->p_processtable->n_active_processes++;
    } else {
        return ENOMEM;
    }

    p->current_pid = newpid;

    return 0;

}

int get_newpid ( struct processtable *pt ) {

    pid_t newpid;
    int ret;

    lock_acquire(pt->pt_lock);

    

}