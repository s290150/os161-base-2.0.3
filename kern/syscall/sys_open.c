#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>

int sys_open(userptr_t filename, int flags, int mode, int*retval)
{
    //vfs_open();
}
