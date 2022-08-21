#include <syscall.h>
#include <types.h>
#include <vnode.h>
#include <vfs.h>
#include <proc.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>    //contains limits for strings, etc.
#include <uio.h>
#include <kern/iovec.h>
#include <file.h>
#include <current.h>

filetable * filetable_init(){
    /*   */
    KASSERT(curproc->p_filetable == NULL);

    curproc->p_filetable = kmalloc(sizeof(struct filetable)*__OPEN_MAX);
	if (curproc->p_filetable == NULL) {
		return ENOMEM;
	}
    for (fd = 0; fd < __OPEN_MAX; fd++) {
		curproc->p_filetable->ft_entry[fd] = NULL;
	}

}

int file_open(char *filename, int flags, int mode, int *retfd){
    struct vnode *vn;
	struct openfile *file;
	int result;
	
	result = vfs_open(filename, flags, mode, &vn);
	if (result) {
		return result;
	}

	file = kmalloc(sizeof(struct openfile));
	if (file == NULL) {
		vfs_close(vn);
		return ENOMEM;
	}

	/* initialize the file struct */
	file->lock = lock_create("file lock");
	if (file->lock == NULL) {
		vfs_close(vn);
		kfree(file);
		return ENOMEM;
	}
	file->f_cwd = vn;
	file->offset = 0;
	file->mode = flags & O_ACCMODE;
	file->reference_count = 1;

    /* checks for invalid access modes */
	KASSERT(file->of_accmode==O_RDONLY || file->of_accmode==O_WRONLY || file->of_accmode==O_RDWR);

    /* place the file in the filetable, getting the file descriptor */
	result = filetable_placefile(file, retfd);
	if (result) {
		lock_destroy(file->of_lock);
		kfree(file);
		vfs_close(vn);
		return result;
	}

	return 0;
}

int placeOpenFile(struct openfile *of, int *fd) {

    for( int i = 0; i < __OPEN_MAX; i++ ) {
        if ( curproc->p_filetable[i]->of_ptr == NULL ) {
            curproc->p_filetable[i]->of_ptr = of;
            *fd = i;
            return 0;
        }
    }
    return EMFILE; //Too many open files

}



int findFD ( int fd, struct openfile** of ) {

    struct fileTable *ft = curthread->t_fileTable;

    if ( fd == 0 || fd > __OPEN_MAX ) { //Control if the file descriptor fd is into a valid range
        return EBADF; //Bad file descriptor
    }

    *of = ft->array_OF[fd]; //associate the openfile structure to the one pointed by
                            //the file descriptor in the System File Table

    if ( *of == NULL ) { //If it is NULL, obviously we are not pointing any existent structure
        return EBADF;
    }

    return 0;
}

int closeOpenFile ( struct openfile *of ) {

    if ( of->reference_count == 1 ) { //If this is the last opening of the file, we can 
                                      //destroy the openfile structure by the System File Table
        vfs_close(of->f_cwd);
        //free also memory if we use kmalloc with kfree(file)
    } else { //if it is not, we can only decrease the reference count, controlling the PANIC
             //with KASSERT
        KASSERT(of->reference_count > 1);
        of->reference_count--;
    }

    return 0;

}