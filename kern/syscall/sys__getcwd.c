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
#include <file.h>
#include <uio.h>

int sys__getcwd( userptr_t buf, size_t len, int* retval ) {

    struct iovec iov;
    struct uio myuio;
    int ret;

    //Initialize a uio suitable for I/O from a kernel buffer.
    uio_kinit( &iov, &myuio, buf, len, 0, UIO_READ );

    ret = vfs_getcwd( &myuio ); //It permits to get the current directory

    if ( ret ) {
        return ret;
    }

    //Here we can set retval to the amount read. In the uio struct there is a data that
    //take into account the residual amount of data to transfer. Thanks to it, we can obtain
    //the effective data read (and we can return it with retval). (The same as for the read).

    *retval = len - myuio.uio_resid;

    return 0;

}