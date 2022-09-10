//Here we define all the functions that operate on pid structure or process_table structure
#include <pid.h>
#include <proc.h>
#include <types.h>
#include <proc.h>
#include <kern/limits.h>
#include <err.h>
#include <synch.h>

int pid_init ( struct pid *p, bool is_proc ) {

    //I'm a little bit not sure on the use of curproc to refer to the new process created
    //Maybe I can pass it as an argument (the pid structure of the new proc)

    pid_t newpid = 0;

    KASSERT( p == NULL );

    p = kmalloc(sizeof(struct pid));
    if ( p == NULL) {
        return ENOMEM;
    }

    p->p_lock = lock_create("pid_lock");
    if ( p->p_lock == NULL ) {
        return ENOMEM;
    }

    if ( is_proc ) {
        p->parent_pid = 0;
        p->current_pid = 1;
    } else {
        p->parent_pid = NULL; //Inizialization of parent pid, that is associated in sys_fork()
        newpid = get_newpid();
        p->current_pid = newpid;
    }


    p->exit_status = 0;
    p->exit = false;


    return 0;

}

processtable * proctable_init() {
    struct processtable *process_table;

    process_table = kmalloc(sizeof(struct processtable));
    KASSERT(process_table != NULL);

    process_table->pt_lock = lock_create("pt_lock");
    KASSERT(process_table->pt_lock != NULL);

    for ( int pid = 0; pid < __PROC_MAX; pid++ ) {
        process_table->pid_ptr[pid] = NULL;
    }
    return process_table;
}

void processtable_placeproc(struct proc *p, pid_t pid) {

    lock_acquire(pt->pt_lock);

    pt->pid_ptr[pid] = p;

    lock_release(pt->pt_lock);

}

pid_t get_newpid () {

    pid_t newpid;
    int ret;

    lock_acquire(pt->pt_lock);

    for ( pid_t i = __PID_MIN; i < __PID_MAX; i++ ) {
            if ( pt->pid_ptr[i] == NULL ) {
                pt->n_active_processes++;
                return i;
            }
    }

    lock_release(pt->pt_lock);

    return ENOMEM;

}

proc proc_search_pid ( pid_t pid ) {

    return pt->pid_ptr[pid];

}

void proc_wait ( struct proc *process ) {

    P(process->end_sem);

    pid_destroy(process->p_pidinfo);

    // If I perform the proc_destroy, I destroy also all the struct allocated dinamically with it?
    // (filetable and pid table)

    proc_destroy(process);

}

void pid_destroy( struct pid *pid ) {

    pt->pid_ptr[pid->current_pid] = NULL;

}

