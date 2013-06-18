/*
 *-----------------------------------------------------------------------------
 * File		: shmem.h
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
#include	<sys/shm.h>


int		shm_create(key_t, int, int *);			/* Create */
int		shm_exist(key_t, int, int *);			/* Exist ? */
int		shm_init(int, int);				/* Initialize */
int		shm_delete(int);				/* Delete */

int		shm_getid(key_t, int, int *);			/* Get ID */
int		shm_attach(int, char **);			/* Attach */
int		shm_detach(char *);				/* Detach */

int		shm_write(int, int, unsigned char *, int);	/* Write */
int		shm_read(int, int, unsigned char **, int);	/* Read */

int		shm_writefile(int, int, int);			/* Write to file */

int		shm_status(key_t);				/* Status ?? */



