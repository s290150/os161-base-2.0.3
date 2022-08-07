#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>

int sys_open(userptr_t filename, int flags, int mode, int* retval)
{
    /*
    * For the vfs_open(), we need to create a char* path. We can use a function called copyinstr()
    * 
    */

    char *path; //Or path[MAX] I think because I need to put it in strlen below in copyinstr()
    int ret;
    int access_mode = flags & O_ACCMODE; //see kern/fcntl.h
    struct vnode *vn;

    ret = copyinstr(filename, path, strlen(path), NULL); //The last is referred to the actual lenght of the string, if we have it. If not, we can put NULL here
    if ( ret ) {//It returns 0 on success
        return ret; //ENAMETOOLONG should be the return value
    }

    KASSERT(access_mode == O_RDONLY || access_mode == O_WRONLY || access_mode == O_RDWR); //We can add all the flags that we want

    ret = vfs_open(path, flags, mode, &vn);
    if ( ret ) {
        return ret;
    }

    





}
