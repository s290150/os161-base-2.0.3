//Here we define all the functions that operate on pid structure or process_table structure
#include <pid.h>
#include <proc.h>
#include <types.h>
#include <proc.h>
#include <kern/limits.h>
#include <kern/errno.h>
#include <synch.h>
#include <lib.h>

struct pid *pid_init ( bool is_proc ) {

    //I'm a little bit not sure on the use of curproc to refer to the new process created
    //Maybe I can pass it as an argument (the pid structure of the new proc)

    struct pid *p;
    pid_t newpid = 0;

    //KASSERT( p == NULL );

    p = kmalloc(sizeof(struct pid));
    KASSERT(p != NULL);

    p->p_lock = lock_create("pid_lock");
    if (p->p_lock == NULL){
        kfree(p);
    }
    KASSERT(p->p_lock != NULL);


    if ( is_proc ) {
        p->parent_pid = 0;
        p->current_pid = 1;
    } else {
        p->parent_pid = 0; //Inizialization of parent pid, that is associated in sys_fork()
        newpid = get_newpid();
        p->current_pid = newpid;
    }

    p->exit_status = 0;
    p->exit = false;

    return p;
}

struct processtable * proctable_init(void) {
    struct processtable *process_table;

    process_table = kmalloc(sizeof(struct processtable));
    KASSERT(process_table != NULL);

    spinlock_init(&process_table->pt_lock);// = lock_create("pt_lock");
    //KASSERT(&process_table->pt_lock != NULL); with spinlock how can you verify if creation is successful?

    for ( int pid = 0; pid < __PROC_MAX; pid++ ) {
        process_table->proc_ptr[pid] = NULL;
    }
    return process_table;
}

void processtable_placeproc(struct proc *p, pid_t pid) {

    spinlock_acquire(&pt->pt_lock);
    pt->proc_ptr[pid] = p;
    spinlock_release(&pt->pt_lock);

}

pid_t get_newpid (void) {
    spinlock_acquire(&pt->pt_lock);

    for ( pid_t i = __PID_MIN; i < __PID_MAX; i++ ) {
            if ( pt->proc_ptr[i] == NULL ) {
                pt->n_active_processes++;
                return i;
            }
    }

    spinlock_release(&pt->pt_lock);

    return ENOMEM;

}

struct proc * proc_search_pid ( pid_t pid ) {

    return pt->proc_ptr[pid];

}

void proc_wait ( struct proc *process ) {

    P(process->p_sem);

    pid_destroy(process->p_pidinfo);

    // If I perform the proc_destroy, I destroy also all the struct allocated dinamically with it?
    // (filetable and pid table)

    proc_destroy(process);

}

void pid_destroy( struct pid *pid ) {

    pt->proc_ptr[pid->current_pid] = NULL;

}

