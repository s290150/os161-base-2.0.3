#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>    //contains limits for strings, etc.
#include <file.h>

int sys_dup2( int oldfd, int newfd ) {

    struct filetable *ft = curproc->p_filetable;
    struct openfile *of;
    int ret;

    ret = findFD( oldfd, of );

    if ( ret ) {
        return ret;
    }

    ret = findFD( newfd, of );

    if ( ret ) {
        return ret;
    }

    //If the file descriptors are the same, the dup2 finish without errors
    if ( oldfd == newfd ) {
        return 0;
    }

    if ( ft->op_ptr[newfd] != NULL ) {

        ret = closeOpenFile(of);

        if ( ret ) {
            return ret;
        }

        curthread->t_fileTable->array_OF[fd] = NULL; //At the end, the structure is deleted by the array

    }

    //Increase the refcount
    lock_acquire(of->lock);
    of->reference_count++;
    lock_release(of->lock);

    ft->op_ptr[newfd] = of; //In this way I am putting in the address pointed by the new fd the address
                            //of the structure pointed by the old fd. So, the refcount need to be incremented
                            //because we have two file descriptors that are referring to the same file.

    return 0;

}