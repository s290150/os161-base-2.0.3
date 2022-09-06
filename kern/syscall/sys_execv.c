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

int
loadexec(char *progname)
{
	struct addrspace *old_as, *new_as;
    
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
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
	result = load_elf(v, &entrypoint);
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
	result = as_define_stack(new_as, &stackptr);
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
    char *path;
	int *argc;
    int result;
	size_t numBytes;
    
    result = copyinstr(progname, path, PATH_MAX, NULL);    //copy the progname from userspace
    if (result){
        return result;
    }

	result = copyin_argv(argv, argc, numBytes);

	if ( result ) {
		return result;
	}


	result = loadexec(path);

	if ( result ) {
		return result;
	}

	result = copyout_argv();

	if ( result ) {
		return result;
	}

    //copyin and copyout for managing argv

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/, argv/*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

int copyin_argv( userptr_t argv, unsigned int *argc, size_t *numBytes ) {

	int i = 0;
	int j = 0;
	char **commands;
	int *pointers;
	size_t actual;

	argc = 0;
	numBytes = 0;

	while ( *(char **)(argv+i) != NULL ) {

		commands[j] = kmalloc(100*sizeof(char));

		result = copyin(argv+i, pointers, sizeof(int)); //argv+i point to the address of the argv vector plus i (that increment of 4 every iteration) to point every time to 4 bytes forward
		
		if (result) {
			return EFAULT;
		}

		result = copyinstr((userptr_t)pointers, commands[j], 100*sizeof(char), &actual);

		if ( result ) {
			return EFAULT;
		}

		i = i+4;

		j = j+1;

		argc = argc + 1;

		int len = strlen(command[j])+1;

		numBytes = numBytes + len;

	}

	return 0;

}