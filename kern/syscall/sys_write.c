#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <file.h>
#include <kern/limits.h>    //contains limits for strings, etc.
#include <uio.h>
#include <kern/iovec.h>

int sys_write( int fd, userptr_t buf, size_t size, int *retval ) {

    struct openfile *of;

    struct iovec iov;
    struct uio myuio;

    int ret;

    ret = findFD(fd, &of);  //now "of" contains the pointer to the openfile structure corresponding to fd
    if ( ret ) {
        return ret;
    }

    if ( of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    lock_acquire(of->lock);

    //Initialize a uio suitable for I/O from a kernel buffer.

    uio_uinit(&iov, &myuio, buf, size, of->offset, UIO_WRITE);

    //VOP_READ is used to perform the effective read from the io

    ret = VOP_WRITE(of->f_cwd, &myuio);

    if ( ret ) {
        lock_release(of->lock);
        return ret;
    }

    //Now we replace the offset with the updated one in the uio

    of->offset = myuio.uio_offset;

    lock_release(of->lock);

    //Here we can set retval to the amount read. In the uio struct there is a data that
    //take into account the residual amount of data to transfer. Thanks to it, we can obtain
    //the effective data read (and we can return it with retval).

    *retval = size - myuio.uio_resid;

    return 0;

}