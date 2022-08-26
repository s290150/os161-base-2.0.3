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
    pid_t used_pid[__PID_MAX];
    struct lock *pt_lock; //for contemporary acces for the allocation of a new process' pid
    int n_active_processes; //variable used to know how many processes are now active (we can't use
                            //the __PID_MAX variable because maybe we reached it as pid's value of
                            //a process but we have holes in the mean).
};

pid * pid_init();