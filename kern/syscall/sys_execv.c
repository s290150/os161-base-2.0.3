#include <types.h>
#include <kern/unistd.h>
#include <limits.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <addrspace.h>
#include <thread.h>
#include <current.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <copyinout.h>
#include <kern/fcntl.h>
#include <file.h>
#include <proc.h>

/* In our implementation, the arguments are copied in the user stack without padding (It is really needed?) */

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