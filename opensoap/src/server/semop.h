/*
 *-----------------------------------------------------------------------------
 * File		: semop.h
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

#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

int		sem_create(key_t, int *);	/* Create a semaphore */
int		sem_exist(key_t, int *);	/* Does semaphore exist ? */
int		sem_init(key_t);		/* Initialize semaphore value */
int		sem_delete(int);		/* Delete a semaphore */

int		sem_getid(key_t, int *);	/* Get sempahore ID */
int		sem_getval(int, int *);		/* Get semaphore semval value */
int		sem_lock(int);			/* Lock a semaphore */
int		sem_lock_wait(int);		/* Wait to lock a semaphore */
int		sem_unlock(int);		/* Unlock a semaphore */

int		sem_lock_ok(key_t);		/* Don't know what this is */

