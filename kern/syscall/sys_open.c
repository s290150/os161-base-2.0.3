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
    /*The copyinstr copies a NULL-terminated string, at most len bytes long, from USER SPACE ADDRESS to the KERNEL SPACE ADDRESS. The number of bytes actually copied, including the terminating
    NULL, is returned in *done(if done is NON-NULL). The copyinstr function return ENAMETOOLONG if the string is longer than len bytes.*/

    /*KASSERT(access_mode == O_RDONLY && access_mode == O_WRONLY && access_mode == O_RDWR && access_mode == O_APPEND ); //KSSERT calls for kpanic, so these conditions must be not equal instead, right?
                                                                                            //KASSERT is a macro which tests for conditions which should never occurr in a correct implementation
    ret = vfs_open(path, flags, mode, &vn);
    if ( ret ) {
        return ret;
    }*/

    /* Maybe I need to use kmalloc to allocate the memory for the openfile structure
    * but in the prof's pdf this is not indicated so I proceed as it says.
    */

    /*of = kmalloc(sizeof(struct openfile));
	if (of == NULL) {
		vfs_close(vn);
		return ENOMEM;
	}

    of->offset = 0; //Initialization of the offset

    ret = filetable_placefile(of, retval);
    if ( ret ) {
        vfs_close(vn);
        return ret;
    }*/

    ret = file_open(path, flags, mode, retval);

    if ( ret ) {
        return ret;
    }

    return 0;

}
