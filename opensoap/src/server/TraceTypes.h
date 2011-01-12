/*
 *-----------------------------------------------------------------------------
 * File		: TraceTypes.h
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

#include        <sys/types.h>
#include	<ctype.h>
#include	<dirent.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<time.h>


/*
 * Defaults...(Good enough for now)
 * Configuration/command-line parameter changeable(?)
 */
#define		DATASEGMENTS		2
//#define	SHMEMSIZE		2048*1024
#define		SHMEMSIZE		512*1024

#define		GEN_SUCCESS		0
#define		GEN_FAILURE		0x80000000

#define		SEM_SUCCESS		0
#define		SEM_FAILURE		0x80000000

#define		SHM_SUCCESS		0
#define		SHM_FAILURE		0x80000000

#define		FILE_SUCCESS		0
#define		FILE_FAILURE		0x80000000

#define		SEM_KEY_START		10000
#define		SHM_KEY_START		10000

#define		KEY_SEARCH_LIMIT	10000


/*
 * Semaphore Operations
 */
#define		SEM_LOCK		-1
#define		SEM_UNLOCK		1
#define		SEM_VAL			0


typedef		int			SHARED_BOOL;


/*
 * Key range for sourcing keys
 */
typedef struct {
		key_t		lower;
		key_t		upper;
	} KEY_RANGE;


/*
 * Key Resource Structure
 * Depending on the super-structure this is used in, the 'id'
 * can be obtained by the appropriate ***_getid(key) function.
 */
typedef struct {
		key_t		value;
		int		id;
	} KEY_ATTRS;

/*
 * Semaphore Resource Structure
 * Need anything more ?
 */
typedef struct {
		KEY_ATTRS	key;
	} SEM_ATTRS;

/*
 * Shared Memory Resource Structure
 * Extra for a memory resource
 */
typedef struct {
		KEY_ATTRS	key;
		char		*addr;
		int		size;
		int		used;
	} SHM_ATTRS;

/*
 * Key Pair Structure
 * Ready to locate some resources.
 */
typedef struct {
		key_t		sem_key;
		key_t		shm_key;
		int		used;		/* Need this for Level 3 */
	} KEYPAIR_ATTRS;

/*
 * Shared Resource Structure
 */
typedef struct {
		SEM_ATTRS	sem;		/* Fill this... */
		SHM_ATTRS	shm;		/* ...and this */
	} SHM_RSRC;
/*
 * In KEYPAIR_ATTRS, SHM_RSRC above, MUST do
 *   *.sem_key -> *.sem.key.value, &
 *   *.shm_key -> *.shm.key.value.
 *
 */

/*
 * The contents of a MGT shared memory area consist of
 *   1. 1 x MGT_HEAD
 *   2. n x KEYPAIR_ATTRS
 * where n is defined inside the MGT_HEAD data area
 */
typedef struct {
		char		signature[256];
		char		filename[256];
		int		n_seg;
	} MGT_HEAD;



typedef struct {
		char		prog_name[256];	// Program name
		char		prog_opt[256];	// Program Option
		int		prog_pid;
		bool		running;	// false: Not running
						// true: Running	
	} OSPROC_STRUCT;


