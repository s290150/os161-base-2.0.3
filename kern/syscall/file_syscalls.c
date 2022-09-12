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

int sys_open(userptr_t filename, int flags, int mode, int* retval)
{
    char path[__PATH_MAX+1]; //the path name has a max length given in kern/limits.h
    int ret;
    int access_mode = flags & O_ACCMODE; //see kern/fcntl.h
    struct vnode *vn;
    struct openfile *of;

    ret = copyinstr(filename, path, strlen(path), NULL); //The last is referred to the actual lenght of the string, if we have it. If not, we can put NULL here
    if ( ret ) {
        return ret; //ENAMETOOLONG should be the return value from copyinstr
    }

    ret = file_open(path, flags, mode, retval);
    if ( ret ) {
        return ret;
    }

    return 0;

}

int sys_close( int fd ) {

    struct openfile *of;
    int ret;

    ret = findFD(fd, &of);
    if ( ret ) {
        return ret;
    }
    if ( of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

   ret = closeOpenFile(of);

   if ( ret ) {
        return ret;
   }

    curproc->p_fileTable->op_ptr[fd] = NULL; //At the end, the structure is deleted by the array

    return 0;

}

int sys__getcwd( userptr_t buf, size_t len, int* retval ) {

    struct iovec iov;
    struct uio myuio;
    int ret;

    //Initialize a uio suitable for I/O from a kernel buffer.
    uio_uinit( &iov, &myuio, buf, len, 0, UIO_READ );

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

int sys_chdir( userptr_t pathname ) {

    char path[__PATH_MAX+1];
    int ret;

    ret = copyinstr(pathname, path, strlen(path), NULL);
    if ( ret ) {
        return ret;
    }

    ret = vfs_chdir( path );
    if ( ret ) {
        return ret;
    }

    return 0;

}

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

int sys_lseek( int fd, off_t offset, int whence, int *retval ) {

    struct stat info;
    struct openfile *of;
    int ret;

    ret = findFD( fd, &of );
    if ( ret ) {
        return ret;
    }
    if ( of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    lock_acquire(of->lock);

    //Depending on the type of seek, we need to set the retval to change the offset

    switch( whence ) {
        case SEEK_SET: //Set the offset sent by the user
            *retval = offset;
            break;
        case SEEK_CUR: //Set the offset as the current position plus offset variable
            *retval = of->offset + offset;
            break;
        case SEEK_END: //Set the offset as the size of the file plus offset variable
            ret = VOP_STAT(of->f_cwd, &info); //In this way I populate the info structure with
                                              //Informations of this opened file.
            if ( ret ) {
                lock_release(of->lock);
                return ret;
            }

            *retval = info.st_size + offset;
            break;
        default: //whence is not valid
            lock_release(of->lock);
            return EINVAL;
    }

    //We can also put the error for a resulting negative offset (or beyond the end of a seekable device)

    of->offset = *retval;

    lock_release(of->lock);

    return 0;

}

int sys_read( int fd, userptr_t buf, size_t size, int *retval ) {

    struct openfile *of;

    struct iovec iov;
    struct uio myuio;

    int ret;

    ret = findFD(fd, &of);
    if ( ret ) {
        return ret;
    }
    if ( of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    lock_acquire(of->lock);

    //Initialize a uio suitable for I/O from a kernel buffer.

    uio_uinit(&iov, &myuio, buf, size, of->offset, UIO_READ); //Maybe here it is uio_uinit
                                                              //I don't remember why i put
                                                              //uio_kinit

    //VOP_READ is used to perform the effective read from the io

    ret = VOP_READ(of->f_cwd, &myuio);

    if ( ret ) {
        lock_release(of->lock);
        return ret;
    }

    //Now we replace the offset with the updated one in the uio

    of->offset = myuio.uio_offset;

    //Here we can set retval to the amount read. In the uio struct there is a data that
    //take into account the residual amount of data to transfer. Thanks to it, we can obtain
    //the effective data read (and we can return it with retval).

    lock_release(of->lock);

    *retval = size - myuio.uio_resid;

    return 0;

}

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