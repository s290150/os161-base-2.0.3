#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <file.h>
#include <limits.h>

int sys_close( int fd ) {

    struct openfile *of;
    int ret;

    ret = findFD(fd, of);

    if ( ret ) {
        return ret;
    }

   ret = closeOpenFile(of);

   if ( ret ) {
        return ret;
   }

    curproc->p_fileTable->op_ptr[fd] = NULL; //At the end, the structure is deleted by the array

    return 0;

}
    /*struct fileTable *ft = curthread->t_fileTable;

    if ( fd == 0 || fd > MAX_OF ) { //Control if the file descriptor fd is into a valid range
        return EBADF; //Bad file descriptor
    }

    *of = ft->array_OF[fd]; //associate the openfile structure to the one pointed by
                            //the file descriptor in the System File Table

    if ( *of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }*/

    /*
    * From this point
    */

    /*if ( of->reference_count == 1 ) { //If this is the last opening of the file, we can 
                                      //destroy the openfile structure by the System File Table
        vfs_close(of->f_cwd);
        //free also memory if we use kmalloc with kfree(file)
    } else { //if it is not, we can only decrease the reference count, controlling the PANIC
             //with KASSERT
        KASSERT(of->reference_count > 1);
        of->reference_count--;
    } */

    /*
    * To this point, we can create a function to destroy the openfile struct in system filetable
    * and dereference the eventual "virtual" memory allocated.
    */

    /*curthread->t_fileTable->array_OF[fd] = NULL; //At the end, the structure is deleted by the array

    return 0;

}*/