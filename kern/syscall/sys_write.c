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

int sys_write( int fd, userptr_t buf, size_t size, int *retval ) {

    struct openfile *of;
    struct fileTable *ft = curthread->t_fileTable;

    struct iovec iov;
    struct uio myuio;

    int ret;

    if ( fd == 0 || fd > MAX_OF ) { //Control if the file descriptor fd is into a valid range
        return EBADF; //Bad file descriptor
    }

    *of = ft->array_OF[fd]; //associate the openfile structure to the one pointed by
                            //the file descriptor in the System File Table

    if ( *of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    //Initialize a uio suitable for I/O from a kernel buffer.

    uio_kinit(&iov, &myuio, buf, size, of->offset, UIO_WRITE);

    //VOP_READ is used to perform the effective read from the io

    ret = VOP_WRITE(of->f_cwd, &myuio);

    if ( ret ) {
        return ret;
    }

    //Now we replace the offset with the updated one in the uio

    of->offset = myuio.uio_offset;

    //Here we can set retval to the amount read. In the uio struct there is a data that
    //take into account the residual amount of data to transfer. Thanks to it, we can obtain
    //the effective data read (and we can return it with retval).

    *retval = size - myuio.uio_resid;

    return 0;

}