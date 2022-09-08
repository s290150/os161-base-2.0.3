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