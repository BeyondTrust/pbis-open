/*
 *-----------------------------------------------------------------------------
 * File		: semop.cpp
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


#include	"semop.h"

#include	"TraceTypes.h"

extern int sem_create(key_t key_value, int *semph_id)
{
	int		semid;
	int		ret_value;
	
	if ((semid = semget(key_value, 1, IPC_CREAT | IPC_EXCL | 0666)) < 0){
		switch(errno){
			case EEXIST :
			case ENOMEM :
				break;
		}
		*semph_id = 0;
		ret_value = SEM_FAILURE;
	}
	else{
		*semph_id = semid;
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

extern int sem_exist(key_t key_value, int *semph_id)
{
	int		semid;
	int		ret_value;
	
	if ((semid = semget(key_value, 1, 0666)) < 0){
		switch(errno){
			case EACCES :
				ret_value = SEM_SUCCESS;
				break;
			case EEXIST :
				ret_value = SEM_SUCCESS;
				break;
			case ENOENT :
				ret_value = SEM_FAILURE;
				break;
			case EINVAL :
				ret_value = SEM_SUCCESS;
				break;
			default :
				ret_value = SEM_SUCCESS;
				break;
		}
		*semph_id = 0;
	}
	else{
		*semph_id = semid;
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

extern int sem_init(key_t semph_id)
{
	int			semid;
	int			ret_value;

	semid = semph_id;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    union semun 
#else
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short int *array;
        struct seminfo *__buf;
    } 
#endif
    arg;

	arg.val = 1;
	
	if ((ret_value = semctl(semid, 0, SETVAL, arg)) < 0){
	/* 0, (or non-negative value for return values) on success */
		switch(errno){
			case EACCES :
				break;
			case EIDRM :
				break;
			default :
				break;
		}
		ret_value = SEM_FAILURE;
	}
	else{
		ret_value = SEM_SUCCESS;
	}
	
	return(ret_value);
}

extern int sem_delete(int semph_id)
{
	int		semid;
	int		ret_value;
	
	semid = semph_id;
	
	if ((ret_value = semctl(semid, 0, IPC_RMID, 0)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SEM_FAILURE;
	}
	else{
		ret_value = SEM_SUCCESS;
	}
	
	return(ret_value);
}


extern int sem_getid(key_t key_value, int *semph_id)
{
	int		semid;
	int		ret_value;
	
	if ((semid = semget(key_value, 1, 0666)) < 0){
		switch(errno){
			case EACCES :
			case ENOENT :
				break;
			default :
				break;
		}
		*semph_id = 0;
		ret_value = SEM_FAILURE;
		fprintf(stderr, "Failed to get SEM_ID:sem_getid()\n");
	}
	else{
		*semph_id = semid;
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

extern int sem_getval(int semph_id, int *semph_val)
{
	int		semid;
	int		ret_value;
	int		sem_value;
	
	semid = semph_id;
	
	if ((sem_value = semctl(semid, 0, GETVAL)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		*semph_val = -1;	// Impossible value!
		ret_value = SEM_FAILURE;
	}
	else{
		*semph_val = sem_value;
		ret_value = SEM_SUCCESS;
	}
	
	return(ret_value);
}

extern int sem_lock(int semph_id)
{
	int		semid;
	int		ret_value;
	struct sembuf	sops[1];
	
	semid = semph_id;
	
	sops[0].sem_num = 0;
	sops[0].sem_op = SEM_LOCK;
	sops[0].sem_flg = IPC_NOWAIT;
	if ((ret_value = semop(semid, sops, 1)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SEM_FAILURE;
	}
	else{
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

extern int sem_lock_wait(int semph_id)
{
	int		semid;
	int		ret_value;
	struct sembuf	sops[1];
	
	semid = semph_id;
	
	sops[0].sem_num = 0;
	sops[0].sem_op = SEM_LOCK;
	sops[0].sem_flg = 0;
	if ((ret_value = semop(semid, sops, 1)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SEM_FAILURE;
	}
	else{
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

/*
 * Note
 * ====
 * Check that the value for "semval" is less than 1 before
 * calling this, as it will result in cascaded unlocks!
 * The value for "semval" could grow to be greater than 1,
 * which would then allow multiple locks from various processes.
 */
extern int sem_unlock(int semph_id)
{
	int		semid;
	int		ret_value;
	struct sembuf	sops[1];
	
	semid = semph_id;
	
	sops[0].sem_num = 0;
	sops[0].sem_op = SEM_UNLOCK;
	sops[0].sem_flg = 0;
	if ((ret_value = semop(semid, sops, 1)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = SEM_FAILURE;
	}
	else{
		ret_value = SEM_SUCCESS;
	}

	return(ret_value);
}

/*
 * Note:
 * -----
 *   When a semaphore is initially created with semget(), it is not lockable
 *   as it will not have its "semval" initialized (to 0). So to use it,
 *   it first has to be unlocked, or have its "semval" set to 1. Subsequent "LOCK"
 *   requests add the "LOCK" value (-1) to the this semval PROVIDED the value
 *   of semval is greater than or equal to the absolute value of "sem_op".
 *   This is the hard part of the concept of semaphores.
 *
 *   In which case the following function may very well be REDUNDANT.
 *
 *   (-PS. I don't understand what I was trying to do here! Seriously, I don't!)
 */
extern int sem_lock_ok(key_t key_value)
{
	int		semid;
	int		ret_value;
	struct sembuf	sops[1];
	
	if ((semid = semget(key_value, 1, 0666)) < 0){
		switch(errno){
			case EACCES :
				break;
			default :
				break;
		}
		ret_value = semid;
	}
	else{
		sops[0].sem_num = 0;
		sops[0].sem_op = SEM_VAL;
		sops[0].sem_flg = IPC_NOWAIT;
		
		if ((ret_value = semop(semid, sops, 1)) < 0){
			switch(errno){
				case EACCES :
					break;
				default :
					break;
			}
		}
	}

	return(ret_value);
}

