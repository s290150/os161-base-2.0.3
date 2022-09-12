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

int sys_fork( struct trapframe *tf, pid_t* retval ) {

    pid_t new_pid;
    struct trapframe *tf_new;
    struct addrspace *addr_new = NULL;
    struct proc *new_proc = NULL;
    int ret;

    spinlock_acquire(&curproc->p_lock);

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

    /* From here */


    new_proc->p_pidinfo->parent_pid = curproc->p_pidinfo->current_pid;

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

    ret = as_copy(curproc->p_addrspace, &addr_new);		//as_copy requires a double pointer for addr_new, it seems.

    if ( ret ) {
        kfree(tf_new);
        proc_destroy(new_proc);
        return ret;
    }

    memcpy(new_proc->p_addrspace, addr_new, sizeof(struct addrspace));

    // I copy the filetable//
    filetable_copy(new_proc->p_filetable);

    // Create a new thread and attach it to the new proc (copying the trapframe
    // of the old process to it).

    ret = thread_fork( curthread->t_name, new_proc, enter_forked_process, tf_new, NULL); //function without & ?
    if ( ret ) {
        as_destroy(addr_new);
        kfree(tf_new);
        proc_destroy(new_proc);
        return ret;
    } 

    processtable_placeproc(new_proc, new_proc->p_pidinfo->current_pid);

    //This is the return for the parent

    *retval = new_proc->p_pidinfo->current_pid;

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

int sys_execv(userptr_t progname, userptr_t argv){

	vaddr_t entrypoint, stackptr;
    char *path;
	int argc;
    int result;
	size_t numBytes;
	int i = 0;
	int j = 0;
	char **commands;
	int *pointer;
	size_t actual;
	char **uargv;
	unsigned int curaddr;
    
    result = copyinstr(progname, path, PATH_MAX, NULL);    //copy the progname from userspace
    if (result){
        return result;
    }

	argc = 0;

	while ( *(char **)(argv+i) != NULL ) {

		commands[j] = kmalloc(100*sizeof(char));

		result = copyin(argv+i, pointer, sizeof(int)); //argv+i point to the address of the argv vector plus i (that increment of 4 every iteration) to point every time to 4 bytes forward
		
		if (result) {
			return EFAULT;
		}

		result = copyinstr((userptr_t)pointer, commands[j], 100*sizeof(char), &actual);

		if ( result ) {
			return EFAULT;
		}

		i = i+4;

		j = j+1;

		argc = argc + 1;

	}

	command[j+1] = NULL;


	result = loadexec(path, &entrypoint, &stackptr);

	if ( result ) {
		return result;
	}

	uargv = (char *) kmalloc(sizeof(char *) * (argc+1));

	if ( uargv == NULL )
	{
		as_destroy(curthread->t_vmspace);
		kfree_all(commands);
		kfree(commands);
		return ENOMEM;
	}

	curaddr = stackptr;

	for (j=0; j<i; j++)
	{
		length = strlen(commands[j])+1;
		curaddr -= length;
		result = copyout(commands[j], curaddr, length);
		if ( result ) {
			return result;
		}
		uargv[j] = curaddr;
	}

	uargv[j] = NULL;
	curaddr -= sizeof(char *) * (j+1);

	result = copyout(uargv, curaddr, sizeof(char *) * (j+1));
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

    proc_remthread(curthread); //Detach of the current thread from the process that calls the exit

    // All the files opened by this process need to be closed
    // But the ones shared with other processes not. What does it means exactly?

    V(p->p_sem); // This semaphore is put high to be used by the waitpid() system call

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

    if ( p == NULL ) {
        return ENOMEM;
    }

    p_status = p->p_pidinfo->exit_status;

    proc_wait(p);

    result = copyout((const void *) &p_status, status, sizeof(int));
    if (result) {
        return result;
    }

    *retval = pid;

    return 0;

}
