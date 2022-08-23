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
#include <file.h>

int sys_lseek( int fd, off_t offset, int whence, int *retval ) {

    struct stat info;
    struct openfile *of;
    int ret;

    ret = findFD( fd, &of );

    if ( ret ) {
        return ret;
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