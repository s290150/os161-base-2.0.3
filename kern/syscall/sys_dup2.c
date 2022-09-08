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
    struct openfile *of, *temp_of;
    int ret;

    ret = findFD( oldfd, &of );
    if ( ret ) {
        return ret;
    }

    ret = findFD( newfd, &temp_of);
    if ( ret ) {
        return ret;
    }
    
    //If the file descriptors are the same, the dup2 finishes without errors
    if ( oldfd == newfd ) {
        return 0;
    }

    if ( temp_of != NULL ) {

        ret = closeOpenFile(temp_of);
        if ( ret ) {
            return ret;
        }
        
        ft->op_ptr[fd] = NULL; //At the end, the structure is deleted by the array
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