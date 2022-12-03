#include <pid.h>
#include <types.h>
#include <kern/limits.h>
#include <kern/syscall.h>
#include <syscall.h>
#include <lib.h>
#include <addrspace.h>
#include <current.h>
#include <proc.h>
#include <kern/errno.h>
#include <file.h>
#include <thread.h>
#include <../arch/mips/include/trapframe.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <copyinout.h>
#include <synch.h>
#include <vnode.h>

void
enter_forked_process(void *data1, unsigned long data2){
	/* struct trapframe mytf;
	struct trapframe *ntf = tf; */

	(void)data2;
	
	void *tf = (void *) curthread->t_stack + 16;

	memcpy(tf, (const void *) data1, sizeof(struct trapframe));
	kfree((struct trapframe *) data1);
	
	as_activate();
	mips_usermode((struct trapframe *)tf);
}

int sys_fork( struct trapframe *tf, pid_t* retval ) {

    struct trapframe *tf_new;
    //struct addrspace *addr_new = NULL;
    struct proc *new_proc = NULL;
    int ret;

    //spinlock_acquire(&curproc->p_lock);	//already done in proc_create_runprogram

    /* From here */

    if ( pt->n_active_processes >= __PID_MAX ) { //Too many active processes
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

    /* Here there is a control related to the creation of the new pid
    // and new associations. */

	spinlock_acquire(&curproc->p_lock);
    new_proc->p_pidinfo->parent_pid = curproc->p_pidinfo->current_pid;
	spinlock_release(&curproc->p_lock);

	// Create a new address space and copy the old one (parent process)

    ret = as_copy(curproc->p_addrspace, &new_proc->p_addrspace);
    if ( ret ) {
        proc_destroy(new_proc);
        return ret;
    }

	spinlock_acquire(&curproc->p_lock);
	if (curproc->p_cwd != NULL) {
		VOP_INCREF(curproc->p_cwd);
		new_proc->p_cwd = curproc->p_cwd;
	}
	spinlock_release(&curproc->p_lock);

    // We allocate a new trapframe and copy it into the new one

    tf_new = kmalloc(sizeof(struct trapframe));
    if ( tf_new == NULL ) {
		as_destroy(new_proc->p_addrspace);
        proc_destroy(new_proc);
        return ENOMEM;
    }

    memcpy(tf_new, tf, sizeof(struct trapframe));
	tf_new->tf_v0 = 0;	//retval?
	tf_new->tf_v1 = 0;	//retval1?		   
	tf_new->tf_a3 = 0;	//signal no error
	tf_new->tf_epc += 4;	//advances the program counter to avoid restarting the syscall over and over again.

    

    //memcpy(new_proc->p_addrspace, addr_new, sizeof(struct addrspace));	//as_copy at row 87 does the same job as this line maybe

    // Here we copy filetable and update the process table//

    filetable_copy(new_proc->p_filetable);
    processtable_placeproc(new_proc, new_proc->p_pidinfo->current_pid);

    //This is the return for the parent

    *retval = new_proc->p_pidinfo->current_pid;
	
	// Create a new thread and attach it to the new proc (copying the trapframe
    // of the old process to it).

    ret = thread_fork( curthread->t_name, new_proc, enter_forked_process, (void *)tf_new, (unsigned long) 1);
    if ( ret ) {
		as_destroy(new_proc->p_addrspace);
        kfree(tf_new);
        proc_destroy(new_proc);
        return ret;
    }
	
	 

    // This is the return for the child
    return 0;

}

int
loadexec(char *progname, vaddr_t *entrypoint, vaddr_t *stackptr)
{
	struct addrspace *old_as, *new_as;
    
	struct vnode *v;
	int result;
    char *newname;

	

    /* new name for thread */   
    newname = kstrdup(progname);    //strcpy with a kmalloc to have the same dimension of the argument
	if (newname == NULL) {
		return ENOMEM;
	}

    /* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
        kfree(newname);
		return result;
	}

	/* Create a new address space. */
	new_as = as_create();
	if (new_as == NULL) {
        kfree(newname);
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	old_as = proc_setas(new_as);    //sets the new as and returns the old one in case of errors
	as_activate();

	/* Load the executable. */
	result = load_elf(v, entrypoint);
	if (result) {
        vfs_close(v);
        proc_setas(old_as); 
		as_activate();
		as_destroy(new_as);
		kfree(newname);
        return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(new_as, stackptr);
	if (result) {
        proc_setas(old_as); 
		as_activate();
		as_destroy(new_as);
        kfree(newname);
		return result;
	}

    if (old_as) {
		as_destroy(old_as);
	}

    kfree(curthread->t_name);   //is it needed?
	curthread->t_name = newname;

	return 0;
}

int sys_execv(char * progname, char ** argv){

	vaddr_t entrypoint, stackptr;
    char path[__PATH_MAX];
	int argc;
    int result;
	char *commands[10];
	//int *pointer;
	size_t actual;
	char **uargv;
	unsigned int curaddr;
	int length;
    argc = 0;
	int i = 0;
	int j = 0;

	length = 100*sizeof(char);

    result = copyinstr((const_userptr_t)progname, path, __PATH_MAX, NULL);    //copy the progname from userspace
	//copyinstr copies only for the length of progname, __PATH_MAX is used only inside it to verify that the actual length doesn't exceed this value.
    if (result){
        return result;
    }

	//while ( *(char **)(argv+i) != NULL ) {
	
	do {

		//a control for i to not exceed __ARG_MAX is needed here

		commands[i] = (char *)kmalloc(length);	//pointer to a heap space for a single command of 100 chars
		if (commands[i] == NULL){
        	return ENOMEM;
    	}
		//pointer = kmalloc(sizeof(int));

		/* result = copyin((userptr_t)(argv+i), pointer, sizeof(int)); //argv+i point to the address of the argv vector plus i (that increment of 4 every iteration) to point every time to 4 bytes forward
		if (result) {
			return EFAULT;
		}

		result = copyinstr((userptr_t)pointer, commands[j], , &actual);
		if ( result ) {
			return EFAULT;
		} */
		// the commented code seems optimized by the compiler, since there is a jump from row 215 to 232. Looking at the code, it seems that copyinstr can be done directly, without the pointer variable.
		
		result = copyinstr((userptr_t)argv[i], commands[i], (100*sizeof(char)), &actual);	//previously length was 100*sizeof(char), that didn't work correctly because there were no brackets!
		if ( result ) {
			return EFAULT;
		}

		i++;
		//j = j+1;
		argc = argc + 1;
	} while(argv[i] != NULL);	//a do while is needed because userspace variables can't be written or tested directly, it seems.

	commands[j+1] = NULL;


	result = loadexec(path, &entrypoint, &stackptr);

	if ( result ) {
		return result;
	}

	uargv = (char **) kmalloc(sizeof(char *) * (argc+1));

	if ( uargv == NULL )
	{
		kfree(commands);
		return ENOMEM;
	}

	curaddr = stackptr;

	for (j=0; j<i; j++)
	{
		length = strlen(commands[j])+1;
		curaddr -= length;
		result = copyout(commands[j], (userptr_t) curaddr, length);
		if ( result ) {
			return result;
		}
		kfree(commands[j]);
		memcpy(uargv[j], &curaddr, sizeof(unsigned int));
		//uargv[j] = curaddr; //This line gave me an error, so I tought to use memcpy to copy it
							  //But the second argument (curaddr) needs to be a pointer so I
							  //Wrote it with the &. I don't know exactly if it is the correct
							  //reference.
	}

	uargv[j] = NULL;
	curaddr -= sizeof(char *) * (j+1);

	result = copyout(uargv, (userptr_t) curaddr, sizeof(char *) * (j+1));
	if ( result ) {
		return result;
	}

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/, (userptr_t)stackptr/*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

void sys__exit( int status ) {
    struct proc *p = curproc;
    curproc->p_pidinfo->exit_status = status;
    curproc->p_pidinfo->exit = true;
	
	
	//kprintf("\nchild %d\n", (int)pt->proc_ptr[p->p_pidinfo->current_pid]);
    // All the files opened by this process need to be closed
    // But the ones shared with other processes not. What does it means exactly?
	
	//kprintf("exit pid %d\n", curproc->p_pidinfo->current_pid);
	proc_remthread(curthread);
    
    V(p->p_sem); // This semaphore is put high to be used by the waitpid() system call
	//kprintf("sem value: %d\n", (unsigned int)curproc->p_sem->sem_count);
	
	thread_exit();
}

int sys_getpid( pid_t *retval )
{

    *retval = curproc->p_pidinfo->current_pid;

    return 0;

}

int sys_waitpid( pid_t pid, userptr_t status, int option, pid_t *retval ) {

    // We need to recover the proc structure using the pid associated to the process that
    // we are waiting. So we need a processtable that take into account all the processes
    // (basically the same we did for filetable)

    struct proc *p = proc_search_pid(pid);
    int p_status;
	int result;

	

    if ( p == NULL ) {
        return ENOMEM;
    }

	if ( option != 0 ) {
		return EINVAL;
	}
	

	//kprintf("\nparent %d\n", (int)pt->proc_ptr[pid]);
    p_status = proc_wait(p);
	
    result = copyout((const void *) &p_status, status, sizeof(int));
    if (result) {
        return result;
    }

	
	//here something breaks the filetable (discovered from miniforktest)
	
    *retval = pid;
    return 0;
}
