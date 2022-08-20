#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <fileTable.h>
#include <kern/limits.h>    //contains limits for strings, etc.
#include <uio.h>
#include <kern/iovec.h>
#include <file.h>

int findFD ( int fd, struct openfile** of ) {

    struct fileTable *ft = curthread->t_fileTable;

    if ( fd == 0 || fd > __OPEN_MAX ) { //Control if the file descriptor fd is into a valid range
        return EBADF; //Bad file descriptor
    }

    *of = ft->array_OF[fd]; //associate the openfile structure to the one pointed by
                            //the file descriptor in the System File Table

    if ( *of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    return 0;
}

int closeOpenFile ( struct openfile *of ) {

    if ( of->reference_count == 1 ) { //If this is the last opening of the file, we can 
                                      //destroy the openfile structure by the System File Table
        vfs_close(of->f_cwd);
        //free also memory if we use kmalloc with kfree(file)
    } else { //if it is not, we can only decrease the reference count, controlling the PANIC
             //with KASSERT
        KASSERT(of->reference_count > 1);
        of->reference_count--;
    }

    return 0;

}

int placeOpenFile(struct openfile *of, int *fd) {

    struct fileTable *ft = curthread->t_fileTable; //I think that, in this way, I can refere to
    //a System fileTable that is common to all the processes that are currently present on the
    //disk and that contains the openfile structure of all the files opened

    for( int i = 0; i < __OPEN_MAX; i++ ) {
        if ( ft->array_OF[i] == NULL ) {
            ft->array_OF[i] = of;
            *fd = i;
            return 0;
        }
    }
    return EMFILE; //Too many open files

}