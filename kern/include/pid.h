#include <types.h>
#include <kern/limits.h>




struct pid {

    pid_t parent_pid; //parent pid
    pid_t current_pid; //pid of the actual process
    int exit_status; //process' exit status
    bool exit; //Process' exited?
    struct lock *p_lock; //Process' lock (maybe for a contemporary acces from waitpid() and the process
                         //itself that has to terminate).

};

struct processtable {
    struct proc *proc_ptr [__PROC_MAX];
    pid_t *used_pid[__PID_MAX]; //If in the i position there's a 1, the position is occupied; otherwise
                               //, if it's 0, the position is free.
    struct lock *pt_lock; //for contemporary acces for the allocation of a new process' pid
    int *n_active_processes; //variable used to know how many processes are now active (we can't use
                            //the __PID_MAX variable because maybe we reached it as pid's value of
                            //a process but we have holes in the mean).
};

pid pid_init( struct pid *p ); //Here we can also put the initialization of the queue for the pid to re-use
processtable *proctable_init();
void processtable_placeproc(struct proc *p, pid_t pid)
pid_t get_newpid();
proc proc_search_pid( pid_t pid );
void proc_wait( struct pid *p );
void pid_destroy();