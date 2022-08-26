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
    /*
    * For the vfs_open(), we need to create a char* path. We can use a function called copyinstr()
    * 
    */

    char path[__PATH_MAX+1]; //the path name has a max length given in kern/limits.h
    int ret;
    int access_mode = flags & O_ACCMODE; //see kern/fcntl.h
    struct vnode *vn;
    struct openfile *of;

    ret = copyinstr(filename, path, strlen(path), NULL); //The last is referred to the actual lenght of the string, if we have it. If not, we can put NULL here
    if ( ret ) {//It returns 0 on success
        return ret; //ENAMETOOLONG should be the return value
    }

    ret = file_open(path, flags, mode, retval);

    if ( ret ) {
        return ret;
    }

    return 0;

}
