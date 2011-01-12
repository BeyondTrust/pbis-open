/*
 *-----------------------------------------------------------------------------
 * File		: shmem.cpp
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP Shared Manager
 * Date		: 2003/12/02
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include	"shmem.h"

#include	"TraceTypes.h"


extern int shm_create(key_t key_value, int size, int *shmem_id)
{
	int		shmid;
	int		ret_value;
	
	if ((shmid = shmget(key_value, size, IPC_CREAT | IPC_EXCL | 0666)) < 0){
		switch(errno){
			case EACCES :
			case EEXIST :
				break;
			default :
				break;
		}
		*shmem_id = 0;
		ret_value = SHM_FAILURE;
	}
	else{
		*shmem_id = shmid;
		ret_value = SHM_SUCCESS;
	}

	return(ret_value);
}

/*
 * Remember, here we are only checking for the existence of a shared memory
 * segment, regardless of any other properties.
 * FAILURE here indicates that a shared segment is available for
 * use.
 */
extern int shm_exist(key_t key_value, int size, int *shmem_id)
{
	int		shmid;
	int		ret_value;
	
	if ((shmid = shmget(key_value, size, 0666)) < 0){
		switch(errno){
			case EACCES :
				ret_value = SHM_SUCCESS;
				break;
			case ENOENT :
				ret_value = SHM_FAILURE;
				break;
			default :
				ret_value = SHM_SUCCESS;
				break;
		}
		*shmem_id = 0;
	}
	else{
		*shmem_id = shmid;
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

/*
 * Initialize a Shared Memory Segment by its ID
 */
extern int shm_init(int shmid, int size)
{
	int		ret_value;
	char		*shm_addr;
	
	if ((shm_addr = (char *)shmat(shmid, NULL, 0)) == (char *) -1){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
//		bzero(shm_addr, size);
		memset(shm_addr, 0, size);
		shmdt(shm_addr);
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

extern int shm_delete(int shmem_id)
{
	int			shmid;
	int			ret_value;
	struct shmid_ds		shm_ds;
	
	shmid = shmem_id;
	
	if ((ret_value = shmctl(shmid, IPC_RMID, &shm_ds)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
		ret_value = SHM_SUCCESS;
	}

	return(ret_value);
}


/*
 * As far as I know, the "size" here is irrelevant.
 */
extern int shm_getid(key_t key_value, int size, int *shmem_id)
{
	int		shmid;
	int		ret_value;
	
	if ((shmid = shmget(key_value, size, 0666)) < 0){
		switch(errno){
			case EACCES :
				break;
			case ENOENT :
				break;
			default :
				break;
		}
		*shmem_id = 0;
		ret_value = SHM_FAILURE;
	}
	else{
		*shmem_id = shmid;
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

extern int shm_attach(int shmem_id, char **shm_addr)
{
	int		shmid;
	int		ret_value;
	
	shmid = shmem_id;
	
	if ((*shm_addr = (char *)shmat(shmid, NULL, 0)) == (char *) -1){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
		ret_value = SHM_SUCCESS;
	}

	return(ret_value);
}

extern int shm_detach(char *shm_addr)
{
	int		shmid;
	int		ret_value;
	
	if ((ret_value = shmdt(shm_addr)) < 0){
		ret_value = SHM_FAILURE;
	}
	else{
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

/*
 * Write 'data' of size 'length' at 'offset' from start of shared memory segment
 * Check that length added to offset does not exceed size of segment!
 */
extern int shm_write(int shmem_id, int offset, unsigned char *data, int length)
{
	int		shmid;
	int		ret_value;
	char		*shm_addr;

	if ((shm_addr = (char *)shmat(shmid, NULL, 0)) == (char *) -1){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
		memcpy((shm_addr + offset), data, length);
		shmdt(shm_addr);
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

/*
 * Read 'data' of size 'length' from 'offset' from start of shared memory segment
 * check that the length added to offset does not exceed size of segment
 */
extern int shm_read(int shmem_id, int offset, unsigned char **data, int length)
{
	int		shmid;
	int		ret_value;
	char		*shm_addr;
	
	shmid = shmem_id;
	
	if ((shm_addr = (char *)shmat(shmid, NULL, 0)) == (char *) -1){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
		/* Allocation */
		*data = (unsigned char *)malloc(length+1);
//		bzero(*data, (length + 1));
		memset(*data, 0, (length + 1));
		memcpy(*data, (shm_addr + offset), length);
		shmdt(shm_addr);
		ret_value = SHM_SUCCESS;
	}
	
	return(ret_value);
}

/*
 * Write the entire contents(up to used) of the specified shared memory segment
 * to a file and reset the contents
 */
extern int shm_writefile(int shmem_id, int log_fd, int size)
{
	int		shmid;
	int		ret_value;
	char		*shm_addr;
	
	if ((shm_addr = (char *)shmat(shmid, NULL, 0)) == (char *) -1){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SHM_FAILURE;
	}
	else{
		// LockFileWrite(log_fd, (unsigned char *)shm_addr, length);
//		bzero(shm_addr, size);
		memset(shm_addr, 0, size);
		shmdt(shm_addr);
		ret_value = SHM_SUCCESS;
	}

	return(ret_value);
}

/*
 * This is the first step in performing analysis of a
 * shared segment. Basically it entails getting the
 * shmid_ds structure for the shared segement and looking
 * at the contents.
 * Obviously it is a little bit more involved than this.
 * This must be called by each 'process/thread' to ensure that
 * the properties of the segment are properly inherited.
 * This function will also establish a "global_description" for
 * segment. If the segment has been newly created, the
 * global_description is initialized, but if the segment already
 * exists, then the global_description receives the established
 * data.
 */
extern int shm_status(key_t key_value)
{
	int		shmid;
	int		ret_value;
	
	ret_value = SHM_SUCCESS;
	
	return(ret_value);
}




