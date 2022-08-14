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

    KASSERT(access_mode != O_RDONLY && access_mode != O_WRONLY && access_mode != O_RDWR); //KSSERT calls for kpanic, so these conditions must be not equal instead, right?

    ret = vfs_open(path, flags, mode, &vn);
    if ( ret ) {
        return ret;
    }

    /* Maybe I need to use kmalloc to allocate the memory for the openfile structure
    * but in the prof's pdf this is not indicated so I proceed as it says.
    */
    of = kmalloc(sizeof(struct openfile));
	if (of == NULL) {
		vfs_close(vn);
		return ENOMEM;
	}

    of->offset = 0; //Initialization of the offset

    ret = placeOpenFile(of, retval);
    if ( ret ) {
        vfs_close(vn);
        return ret;
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
