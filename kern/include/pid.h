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
};

pid * pid_init();