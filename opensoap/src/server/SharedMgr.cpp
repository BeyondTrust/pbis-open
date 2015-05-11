/*
 *-----------------------------------------------------------------------------
 * File		: SharedMgr.cpp
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
#include	"shmem.h"

#include	"TraceTypes.h"
#include	"SharedMgr.h"
#include	"FileMgr.h"

#include	"ServerCommon.h"
#include	"SrvConfAttrHandler.h"
#include	"SrvConf.h"

using namespace std;
using namespace OpenSOAP;

int get_sem_key(key_t *);
int get_shm_key(key_t *);

/*
 * Library Wide Declarations
 */

/*
 * Level 1
 */
SHM_RSRC		shm_rsrc;	/* Resource Control */
KEYPAIR_ATTRS		mgt_keypair;	/* Data */

/*
 * Level 2
 */
SHM_RSRC		mgt_rsrc;	/* Resource Control */
MGT_HEAD		mgt_head;	/* Data */

KEYPAIR_ATTRS		*data_keypair;	/* Don't know how many yet */

/*
 * Level 3
 */
SHM_RSRC		*data_rsrc;	/* Don't know how many yet */

int			data_segments;	/* Don't know how many yet */


/*
 *-----------------------------------------------------------------------------
 * External functions
 *-----------------------------------------------------------------------------
 */


/*
 * Preparation point for the Shared Resource System
 * This is called only once per system, at the system
 * initialization time. Process/programs that are part
 * of the system MUST not access this function. Also, this
 * function passes back the lead key for other processes.
 */
extern int SharedLibPrep(const char *key_file, const char *log_file, key_t key_value, key_t *out_key)
{
	int		ret_value;
	int		offset;
	int		i;
	key_t		root_key;

/*
 * NOTE 1
 * ======
 * Is there a valid/existing key available ?
 */
	SharedLibGetRootKey(key_file, &root_key);

	if (root_key == 0){
		if (get_shm_key(&root_key) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
	}
	*out_key = root_key;

	if (root_key <= 0){
		return(GEN_FAILURE);
	}

	shm_rsrc.shm.key.value = root_key;
	
	fprintf(stderr, "SharedMgr::SharedLibPrep: Root Key = %d\n", shm_rsrc.shm.key.value);
	
	/*
	 * -+- Level 1 -+-
	 */
	
/*
 * NOTE 2.1
 * ========
 * Verify that the key_value is non-zero. If zero, will have
 * to go about getting a proper key!!
 */

	shm_rsrc.shm.size = sizeof(KEYPAIR_ATTRS);
	
	/*
	 * Does this exist ?
	 *  Yes : Do nothing
	 *   No : Create
	 */
	if ((ret_value = shm_exist(shm_rsrc.shm.key.value, shm_rsrc.shm.size,
			&(shm_rsrc.shm.key.id))) == SHM_FAILURE){
		if ((ret_value = shm_create(shm_rsrc.shm.key.value, shm_rsrc.shm.size,
				&(shm_rsrc.shm.key.id))) == SHM_FAILURE){
			fprintf(stderr, "SharedMgr::SharedLibPrep: Failed to create shared memory segment for key(%d)\n",
				shm_rsrc.shm.key.value);
			return(SHM_FAILURE);
		}
		else{
			fprintf(stderr, "SharedMgr::SharedLibPrep: HEAD SHM Key/Id pair = %d, %d\n",
				shm_rsrc.shm.key.value, shm_rsrc.shm.key.id);
			shm_attach(shm_rsrc.shm.key.id, &shm_rsrc.shm.addr);
			memset(shm_rsrc.shm.addr, 0, shm_rsrc.shm.size);
		}
	}
	else{
		shm_attach(shm_rsrc.shm.key.id, &shm_rsrc.shm.addr);
	}

	fprintf(stderr, "SharedMgr::SharedLibPrep: Key/Id : %d, %d\n", shm_rsrc.shm.key.value, shm_rsrc.shm.key.id);
	
	/*
	 * Get whatever is available
	 */
	memcpy(&mgt_keypair, shm_rsrc.shm.addr, sizeof(KEYPAIR_ATTRS));

/*
 * NOTE 2.2
 * ========
 * Must return the contents of mgt_keypair to the shm_rsrc shared memory
 * segment when Level 2 & Level 3 complete.
 */
	
	/*
	 * -+- Level 2 -+-
	 */


	data_segments = DATASEGMENTS;


/*
 * NOTE 3.1
 * ========
 * Can't get away from it anymore. Will have to ensure that
 * valid keys are available at this point.
 */
	if (mgt_keypair.sem_key == 0){	/* Initialize */
		if (get_sem_key(&(mgt_keypair.sem_key)) != SEM_SUCCESS){
			return(GEN_FAILURE);
		}
		mgt_rsrc.sem.key.value = mgt_keypair.sem_key;
		if (sem_create(mgt_rsrc.sem.key.value, &(mgt_rsrc.sem.key.id)) != SEM_SUCCESS){
			return(GEN_FAILURE);
		}
		fprintf(stderr, "SharedMgr::SharedLibPrep: MGT SEM (New) Key/Id pair = %d, %d\n",
			mgt_rsrc.sem.key.value, mgt_rsrc.sem.key.id);
	}
	else{
		mgt_rsrc.sem.key.value = mgt_keypair.sem_key;
		if (sem_getid(mgt_keypair.sem_key, &(mgt_rsrc.sem.key.id)) != SEM_SUCCESS){
			return(GEN_FAILURE);
		}
		fprintf(stderr, "SharedMgr::SharedLibPrep: MGT SEM (Prev) Key/Id pair = %d, %d\n",
			mgt_rsrc.sem.key.value, mgt_rsrc.sem.key.id);
	}

	if (sem_init((key_t)mgt_rsrc.sem.key.id) == SEM_FAILURE){
		fprintf(stderr, "SharedMgr::SharedLibPrep: MGT SEM init failure\n");
	}
	
	if (mgt_keypair.shm_key == 0){	/* Initialize */
		if (get_shm_key(&(mgt_keypair.shm_key)) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
		mgt_rsrc.shm.key.value = mgt_keypair.shm_key;
		mgt_rsrc.shm.size = sizeof(MGT_HEAD) + data_segments * sizeof(KEYPAIR_ATTRS);
		if (shm_create(mgt_rsrc.shm.key.value, mgt_rsrc.shm.size, &(mgt_rsrc.shm.key.id)) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
		fprintf(stderr, "SharedMgr::SharedLibPrep: MGT SHM (New) Key/Id pair = %d, %d\n",
			mgt_rsrc.shm.key.value, mgt_rsrc.shm.key.id);
		if (shm_attach(mgt_rsrc.shm.key.id, &mgt_rsrc.shm.addr) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
		memset(mgt_rsrc.shm.addr, 0, mgt_rsrc.shm.size);
		sprintf(mgt_head.signature, "Level 2");
		mgt_head.n_seg = data_segments;
		memcpy(mgt_rsrc.shm.addr, &mgt_head, sizeof(MGT_HEAD));
	}
	else{
		mgt_rsrc.shm.key.value = mgt_keypair.shm_key;
		if (shm_getid(mgt_rsrc.shm.key.value, 0, &(mgt_rsrc.shm.key.id)) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
		fprintf(stderr, "PREP: MGT SHM (Prev) Key/Id pair = %d, %d\n",
			mgt_rsrc.shm.key.value, mgt_rsrc.shm.key.id);
		if (shm_attach(mgt_rsrc.shm.key.id, &mgt_rsrc.shm.addr) != SHM_SUCCESS){
			return(GEN_FAILURE);
		}
		memcpy(&mgt_head, mgt_rsrc.shm.addr, sizeof(MGT_HEAD));
		mgt_rsrc.shm.size = sizeof(MGT_HEAD) + mgt_head.n_seg * sizeof(KEYPAIR_ATTRS);
	}
	/*
	 * In case the file location has changed
	 */
	sprintf(mgt_head.filename, "%s", log_file);
	
	/*
	 * Get whatever is available
	 */
	
	if ((data_keypair = (KEYPAIR_ATTRS *)malloc(sizeof(KEYPAIR_ATTRS) * mgt_head.n_seg)) == NULL){
		return(GEN_FAILURE);
	}
	offset = sizeof(MGT_HEAD);
	for (i = 0; i < mgt_head.n_seg; i++){
		memcpy(&(data_keypair[i]), mgt_rsrc.shm.addr + offset, sizeof(KEYPAIR_ATTRS));
		offset += sizeof(KEYPAIR_ATTRS);
	}


//	fprintf(stderr, "SharedMgr::SharedLibPrep: Data segments : %d\n", mgt_head.n_seg);


/*
 * NOTE 3.2
 * ========
 * Must return the contents of mgt_head & data_keypair[]'s to mgt_rsrc shared
 * memory segment when Level 3 complete.
 */
 
	/*
	 * -+- Level 3 -+-
	 */


	if ((data_rsrc = (SHM_RSRC *)malloc(sizeof(SHM_RSRC) * mgt_head.n_seg)) == NULL){
		free(data_keypair);
		return(GEN_FAILURE);
	}

/*
 * NOTE 4.1
 * ========
 * As before, will have to determine that there are valid keys
 * available for further processing. Should be a simpler exercise
 * than that for Level 2.
 */
	for (i = 0; i < mgt_head.n_seg; i++){
	
	/*
	 * Seamphores
	 */
		if (data_keypair[i].sem_key == 0){
			if (get_sem_key(&(data_keypair[i].sem_key)) != SEM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			data_rsrc[i].sem.key.value = data_keypair[i].sem_key;
			if (sem_create(data_rsrc[i].sem.key.value, &(data_rsrc[i].sem.key.id)) != SEM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SEM (New) Key/Id pair(%d) = %d, %d\n",
				i, data_rsrc[i].sem.key.value, data_rsrc[i].sem.key.id);
		}
		else{
			data_rsrc[i].sem.key.value = data_keypair[i].sem_key;
			if (sem_getid(data_rsrc[i].sem.key.value, &(data_rsrc[i].sem.key.id)) != SEM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SEM (Prev) Key/Id pair(%d) = %d, %d\n",
				i, data_rsrc[i].sem.key.value, data_rsrc[i].sem.key.id);
		}

		if (sem_init((key_t)data_rsrc[i].sem.key.id) == SEM_FAILURE){
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SEM(%d) init failure\n", i);
		}
	/*
	 * Shared Memory Segmants
	 */
		if (data_keypair[i].shm_key == 0){
			if (get_shm_key(&(data_keypair[i].shm_key)) != SHM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			data_rsrc[i].shm.key.value = data_keypair[i].shm_key;
			data_rsrc[i].shm.size = SHMEMSIZE;
			if (shm_create(data_rsrc[i].shm.key.value, data_rsrc[i].shm.size, &(data_rsrc[i].shm.key.id)) != SHM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SHM (New) Key/Id pair(%d) = %d, %d\n",
				i, data_rsrc[i].shm.key.value, data_rsrc[i].shm.key.id);
			if (shm_attach(data_rsrc[i].shm.key.id, &(data_rsrc[i].shm.addr)) != SHM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			memset(data_rsrc[i].shm.addr, 0, data_rsrc[i].shm.size);
			data_rsrc[i].shm.used = 0;
			data_keypair[i].used = 0;
		}
		else{
			data_rsrc[i].shm.key.value = data_keypair[i].shm_key;
			if (shm_getid(data_rsrc[i].shm.key.value, 0, &(data_rsrc[i].shm.key.id)) != SHM_SUCCESS){
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SHM (Prev) Key/Id pair(%d) = %d, %d\n", i,
				data_rsrc[i].shm.key.value, data_rsrc[i].shm.key.id);
			if (shm_attach(data_rsrc[i].shm.key.id, &(data_rsrc[i].shm.addr)) != SHM_SUCCESS){
				fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SHM(%d) attach failure\n", i);
				free(data_keypair);
				free(data_rsrc);
				return(GEN_FAILURE);
			}
			
			data_rsrc[i].shm.size = SHMEMSIZE;
			data_rsrc[i].shm.used = data_keypair[i].used;
			fprintf(stderr, "SharedMgr::SharedLibPrep: DATA SHM Used(%d) = %d %d\n", i,
				data_rsrc[i].shm.used, data_keypair[i].used);
		}
	}

	
/*
 * NOTE 5
 * ======
 * Must now write back any information that has changed regarding the shared resources.
 * In particular, the mgt_keypair of Level 1, the mgt_head & data_keypairs of Level 2
 */

	/*
	 * -+- Level 2 Data -+-
	 */
	memcpy(mgt_rsrc.shm.addr, &mgt_head, sizeof(MGT_HEAD));
	offset = sizeof(MGT_HEAD);
	for (i = 0; i < mgt_head.n_seg; i++){
		memcpy(mgt_rsrc.shm.addr + offset, &data_keypair[i], sizeof(KEYPAIR_ATTRS));
		offset += sizeof(KEYPAIR_ATTRS);
	}
//	shm_detach(mgt_rsrc.shm.addr);

	/*
	 * -+- Level 1 Data -+-
	 */
	memcpy(shm_rsrc.shm.addr, &mgt_keypair, sizeof(KEYPAIR_ATTRS));
//	shm_detach(shm_rsrc.shm.addr);
	
/*
 * NOTE 6
 * ======
 * If we get this far, then its safe to say that all keys have been defined. Save
 * the root key for possible later use.
 */

	SharedLibSaveRootKey(key_file, root_key);

	return(SHM_SUCCESS);
}

/*
 * Cleanup at the end of something disastrous.
 * This is only called by the process leader when
 * shutting down the system for some reason and any
 * data that is in the shared memory area MUST be flushed
 * to the disk.
 */
extern int SharedLibCleanup(const char *filename, const char *key_file, bool backup)
{
	int		i;
	int		offset;
	
	/*
	 * Flush any shared memory segments, & Remove all traces of presence
	 */
	
	
	/*
	 * Flush them thar memory segments
	 * 1.Do a FileLibInit() - already done
	 * 2.Loop through available segments & flush any contents to the
	 *   Log file
	 * 3.Do a FileLibTerm()
	 */
	
	offset = sizeof(MGT_HEAD);
	for (i = 0; i < mgt_head.n_seg; i++){
		memcpy(&(data_keypair[i]), mgt_rsrc.shm.addr + offset, sizeof(KEYPAIR_ATTRS));
		data_rsrc[i].shm.used = data_keypair[i].used;
		offset += sizeof(KEYPAIR_ATTRS);
	}

	for (i = 0; i < mgt_head.n_seg; i++){
		SharedLibFlush(filename, i);
	}
	
	
	/*
	 * -+- Level 3 Removal -+-
	 */

	for (i = 0; i < mgt_head.n_seg; i++){
		if (data_rsrc[i].shm.key.value > 0){
			shm_delete(data_rsrc[i].shm.key.id);
		}
		if (data_rsrc[i].sem.key.value > 0){
			sem_delete(data_rsrc[i].sem.key.id);
		}
	}
	
	
	/*
	 * -+- Level 2 Removal -+-
	 */
	
	if (mgt_rsrc.shm.key.value > 0){
		shm_delete(mgt_rsrc.shm.key.id);
	}
	if (mgt_rsrc.sem.key.value > 0){
		sem_delete(mgt_rsrc.sem.key.id);
	}

	
	/*
	 * -+- Level 1 Removal -+-
	 */
	
	if (shm_rsrc.shm.key.value > 0){
		shm_delete(shm_rsrc.shm.key.id);
	}
	
	FileLibCleanup(backup);	// Backup y/n ?

	SharedLibRemoveRootKey(key_file);	// Maybe bring this to start of function ?

	return(SHM_SUCCESS);
}

/*
 * Save the root key *after* a successful establishment
 * of the shared memory resource
 */
extern int SharedLibSaveRootKey(const char *key_file, key_t key_value)
{
	int				fd;
	int				ret_val;
	int				length;
	key_t				save_key;

	if (FileLibExist(key_file) != FILE_SUCCESS){
		FileLibCreate(key_file);
	}

	save_key = key_value;
	if ((fd = open(key_file, O_RDWR)) > 0){
		lseek(fd, 0, SEEK_SET);
		write(fd, (key_t *)&save_key, sizeof(key_t));
		close(fd);
	}

	return(SHM_SUCCESS);
}

/*
 * Recover the previously used root key for the shared memory
 * resource if it exists
 */
extern int SharedLibGetRootKey(const char *key_file, key_t *key_value)
{
	int				fd;
	int				ret_val;
	int				length;
	key_t				save_key;

	*key_value = 0;
	if (FileLibExist(key_file) == FILE_SUCCESS){
		if ((fd = open(key_file, O_RDONLY)) > 0){
			lseek(fd, 0, SEEK_SET);
			length = read(fd, (key_t *)&save_key, sizeof(key_t));
			close(fd);
			if (length == sizeof(key_t)){
				*key_value = save_key;
			}
		}
	}

	return(SHM_SUCCESS);
}

extern int SharedLibRemoveRootKey(const char *key_file)
{

//	fprintf(stderr, "Root Key repository : %sSharedKey\n", attrs[0].c_str());

	FileLibDelete(key_file);

	return(SHM_SUCCESS);
}



/*
 * Entry point for all processes into the shared resource
 * system. Passed here is the Master Key, and from it all
 * other resources can/will be extrapolated. Here, it is
 * assumed that all keys, shared segments, etc have been
 * correctly initialized by the lead process.
 */
extern int SharedLibInit(key_t key_value, char **filename)
{
	int		offset;
	int		i;
	

	/*
	 * -+- Level 0 -+-
	 */
	if (key_value <= 0){
		fprintf(stderr, "SharedMgr::SharedLibInit: Invalid Shared Resource Key(%d). -> Trace Logging Disabled\n", key_value);
		return(GEN_FAILURE);
	}
	
	/*
	 * -+- Level 1 -+-
	 */
	shm_rsrc.shm.key.value = key_value;
	if (shm_getid(shm_rsrc.shm.key.value, 0, &(shm_rsrc.shm.key.id)) != SHM_SUCCESS){
		fprintf(stderr, "SharedMgr::SharedLibInit: Unable to get Shared Resource shm.key.id. -> Trace Logging Disabled\n");
		return(GEN_FAILURE);
	}

//	fprintf(stderr, "SharedMgr::SharedLibInit: HEAD SHM Key/Id pair = %d, %d\n",
//		shm_rsrc.shm.key.value, shm_rsrc.shm.key.id);

	
	if (shm_attach(shm_rsrc.shm.key.id, &shm_rsrc.shm.addr) != SHM_SUCCESS){
		return(GEN_FAILURE);
	}
	memcpy(&mgt_keypair, shm_rsrc.shm.addr, sizeof(KEYPAIR_ATTRS));
	
	/*
	 * -+- Level 2 -+-
	 */
	mgt_rsrc.sem.key.value = mgt_keypair.sem_key;
	mgt_rsrc.shm.key.value = mgt_keypair.shm_key;
	if (sem_getid(mgt_rsrc.sem.key.value, &(mgt_rsrc.sem.key.id)) != SEM_SUCCESS){
		return(GEN_FAILURE);
	}
	if (shm_getid(mgt_rsrc.shm.key.value, 0, &(mgt_rsrc.shm.key.id)) != SHM_SUCCESS){
		return(GEN_FAILURE);
	}
	
//	fprintf(stderr, "INIT: MGT SEM Key/Id pair = %d, %d\n", mgt_rsrc.sem.key.value, mgt_rsrc.sem.key.id);
//	fprintf(stderr, "INIT: MGT SHM Key/Id pair = %d, %d\n", mgt_rsrc.shm.key.value, mgt_rsrc.shm.key.id);
	
	if (shm_attach(mgt_rsrc.shm.key.id, &mgt_rsrc.shm.addr) != SHM_SUCCESS){
		return(GEN_FAILURE);
	}
	memcpy(&mgt_head, mgt_rsrc.shm.addr, sizeof(MGT_HEAD));

/*
 * Return filename
 */	
	if ((*filename = (char *)malloc(strlen(mgt_head.filename) + 1)) == NULL){
		return(GEN_FAILURE);
	}
	sprintf(*filename, "%s", mgt_head.filename);
	
//	fprintf(stderr, "INIT: Data segments : %d\n", mgt_head.n_seg);

	if ((data_keypair = (KEYPAIR_ATTRS *)malloc(sizeof(KEYPAIR_ATTRS) * mgt_head.n_seg)) == NULL){
		return(GEN_FAILURE);
	}
	
	offset = sizeof(MGT_HEAD);
	for (i = 0; i < mgt_head.n_seg; i++){
		memcpy(&data_keypair[i], mgt_rsrc.shm.addr + offset, sizeof(KEYPAIR_ATTRS));
		offset += sizeof(KEYPAIR_ATTRS);
	}
	
	/*
	 * -+- Level 3 -+-
	 */
	if ((data_rsrc = (SHM_RSRC *)malloc(sizeof(SHM_RSRC) * mgt_head.n_seg)) == NULL){
		free(data_keypair);
		return(GEN_FAILURE);
	}
	
	for (i = 0; i < mgt_head.n_seg; i++){
		data_rsrc[i].sem.key.value = data_keypair[i].sem_key;
		data_rsrc[i].shm.key.value = data_keypair[i].shm_key;
		
		if (sem_getid(data_rsrc[i].sem.key.value, &(data_rsrc[i].sem.key.id)) != SEM_SUCCESS){
			free(data_keypair);
			free(data_rsrc);
			return(GEN_FAILURE);
		}
		if (shm_getid(data_rsrc[i].shm.key.value, 0, &(data_rsrc[i].shm.key.id)) != SHM_SUCCESS){
			free(data_keypair);
			free(data_rsrc);
			return(GEN_FAILURE);
		}
		
//		fprintf(stderr, "INIT: DATA SEM Key/Id pair(%d) = %d, %d\n", i, data_rsrc[i].sem.key.value, data_rsrc[i].sem.key.id);
//		fprintf(stderr, "INIT: DATA SHM Key/Id pair(%d) = %d, %d\n", i, data_rsrc[i].shm.key.value, data_rsrc[i].shm.key.id);
		
		data_rsrc[i].shm.size = SHMEMSIZE;
		data_rsrc[i].shm.used = data_keypair[i].used;
//		fprintf(stderr, "INIT: DATA SHM Used (%d) = %d %d\n", i, data_keypair[i].used, data_rsrc[i].shm.used);
		if (shm_attach(data_rsrc[i].shm.key.id, &(data_rsrc[i].shm.addr)) == SHM_FAILURE){
			fprintf(stderr, "INIT: DATA SHM(%d) attach failure\n", i);
			free(data_keypair);
			free(data_rsrc);
			return(GEN_FAILURE);
		}
	}
	
	return(SHM_SUCCESS);
}

/*
 * Exit point for all processes from shared resource system.
 * No parameters required.
 */
extern int SharedLibTerm()
{
	int		i;
	/*
	 * -+- Level 3 -+-
	 */
	for (i = 0; i < mgt_head.n_seg; i++){
		shm_detach(data_rsrc[i].shm.addr);
	}

	/*
	 * -+- Level 2 -+-
	 */
	shm_detach(mgt_rsrc.shm.addr);

	/*
	 * -+- Level 1 -+-
	 */
	shm_detach(shm_rsrc.shm.addr);

	return(SHM_SUCCESS);
}

extern int SharedLibAppend(const char *filename, const char *trace_data)
{
	
	/*
	 * 1. Achieve a lock on a Data Segment(Level 3)
	 * 2. Read corresponding data_keypair[].used value
	 * 3. If (enough_space)
	 *      update data_segment
	 *    else
	 *      flush data_segment
	 *      reset data_segment
	 *      update data_segment
	 * 4. Achieve lock on Mgt Segment
	 * 5. Update data_keypair[].used
	 * 6. Release Mgt Segment
	 * 7. Release Data Segment
	 */

	int			offset;
	int			i, j;
	int			lock_value;
	int			lock_semid;
	int			trace_data_length;
	int			available_space;
	int			ret_value;
	
	trace_data_length = strlen(trace_data);

//	fprintf(stderr, "SharedLibAppend(%s)(%d)\n", trace_data, trace_data_length);
	

/*
 * Add a third segment that utilizes waiting so that all hope is not lost.
 * Include an extra parameter indicating a WAIT/NOWAIT locking style.
 *
 * Off course all lock information should be periodically reviewed so
 * that locks created by dead processes can be cleared.
 */

	for (j = 0; j < 20; j++){
		/* Loop until a lockable SEMAPHORE is found */
		lock_semid = -1;
		for (i = 0; i < mgt_head.n_seg; i++){
			if ((lock_value = sem_lock(data_rsrc[i].sem.key.id)) == SEM_FAILURE){
				;
			}
			else{	// Got a lock
				lock_semid = i;		// Actually the index
//				fprintf(stderr, "APPEND: Got lock\n");
				break;
			}
		}
		if (lock_semid < 0){
			usleep(10000);
		}
		else{
			break;
		}
	}
	
	if (lock_semid < 0){
//		fprintf(stderr, "APPEND: Unable to secure semaphore lock!!\n");
		return(SHM_FAILURE);
	}
	
	
//	fprintf(stderr, "APPEND: Lock is %d\n", lock_semid);

	offset = sizeof(MGT_HEAD);
	if (sem_lock(mgt_rsrc.sem.key.id) != SEM_FAILURE){
		for (i = 0; i < mgt_head.n_seg; i++){
			memcpy(&(data_keypair[i]), mgt_rsrc.shm.addr + offset, sizeof(KEYPAIR_ATTRS));
			offset += sizeof(KEYPAIR_ATTRS);
		}
		sem_unlock(mgt_rsrc.sem.key.id);
	}
	else{
//		fprintf(stderr, "Error locking MGT SEM for renewal!!\n");
		;
	}
 
	if (lock_semid >= 0){
		data_rsrc[lock_semid].shm.used = data_keypair[lock_semid].used;
		available_space = data_rsrc[lock_semid].shm.size - data_rsrc[lock_semid].shm.used;
		if (available_space <= trace_data_length){
			SharedLibFlush(filename, lock_semid);
//			bzero(data_rsrc[lock_semid].shm.addr, data_rsrc[lock_semid].shm.size);
			memset(data_rsrc[lock_semid].shm.addr, 0, data_rsrc[lock_semid].shm.size);
			data_rsrc[lock_semid].shm.used = 0;
			data_keypair[lock_semid].used = 0;
		}
		
		memcpy(data_rsrc[lock_semid].shm.addr + data_rsrc[lock_semid].shm.used,
			trace_data, trace_data_length);
		data_rsrc[lock_semid].shm.used += trace_data_length;
		data_keypair[lock_semid].used = data_rsrc[lock_semid].shm.used;
		
		
		offset = sizeof(MGT_HEAD) + sizeof(KEYPAIR_ATTRS) * lock_semid;
		if (sem_lock(mgt_rsrc.sem.key.id) != SEM_FAILURE){
			memcpy(mgt_rsrc.shm.addr + offset, &(data_keypair[lock_semid]), sizeof(KEYPAIR_ATTRS));
			sem_unlock(mgt_rsrc.sem.key.id);
		}
		else{
//			fprintf(stderr, "Error locking MGT SEM for update!!\n");
			;
		}

		/* Release the lock */
//		sleep(2);
		sem_unlock(data_rsrc[lock_semid].sem.key.id);
			
		ret_value = SHM_SUCCESS;
	}
	else{
		/* Failed to get a lock */
		ret_value = SHM_FAILURE;
	}
	
	return(ret_value);
}

extern int SharedLibForcedFlush(const char *filename)
{
	int			offset;
	int			i, j;
	int			lock_value;
	int			lock_semid;
	int			available_space;
	int			ret_value;

//	fprintf(stderr, "Forced flushing\n");

	lock_semid = -1;
	for (i = 0; i < mgt_head.n_seg; i++){
		if ((lock_value = sem_lock(data_rsrc[i].sem.key.id)) == SEM_FAILURE){
			;
		}
		else{
			lock_semid = i;

			offset = sizeof(MGT_HEAD);
			if (sem_lock(mgt_rsrc.sem.key.id) != SEM_FAILURE){
				for (j = 0; j < mgt_head.n_seg; j++){
					memcpy(&(data_keypair[j]), mgt_rsrc.shm.addr + offset, sizeof(KEYPAIR_ATTRS));
					offset += sizeof(KEYPAIR_ATTRS);
				}
				sem_unlock(mgt_rsrc.sem.key.id);
			}
			else{
				;
			}
 
			data_rsrc[lock_semid].shm.used = data_keypair[lock_semid].used;
			if (data_rsrc[lock_semid].shm.used > 0){
				SharedLibFlush(filename, lock_semid);
//				bzero(data_rsrc[lock_semid].shm.addr, data_rsrc[lock_semid].shm.size);
				memset(data_rsrc[lock_semid].shm.addr, 0, data_rsrc[lock_semid].shm.size);
				data_rsrc[lock_semid].shm.used = 0;
				data_keypair[lock_semid].used = 0;
			}

			offset = sizeof(MGT_HEAD) + sizeof(KEYPAIR_ATTRS) * lock_semid;
			if (sem_lock(mgt_rsrc.sem.key.id) != SEM_FAILURE){
				memcpy(mgt_rsrc.shm.addr + offset, &(data_keypair[lock_semid]), sizeof(KEYPAIR_ATTRS));
				sem_unlock(mgt_rsrc.sem.key.id);
			}
			else{
				;
			}
			/* Release the lock */
			sem_unlock(data_rsrc[lock_semid].sem.key.id);
		}
	}

	return(SHM_SUCCESS);
}

extern int SharedLibFlush(const char *filename, int segment)
{
	int		i;
	/*
	 * 1. Lock Trace File
	 * 2. Go to end of file
	 * 3. Write contents of data_rsrc[segment].addr
	 * 4. Release Trace File
	 */

	
	fprintf(stderr, "SharedMgr:: SharedLibFlush() : Usage[%d] : %d\n", segment, data_keypair[segment].used);

	if (data_keypair[segment].used > 0){
		FileLibAppend(filename, data_rsrc[segment].shm.addr, data_keypair[segment].used);
	}

	return(SHM_SUCCESS);
}



/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

/*
 *-----------------------------------------------------------------------------
 * Local functions, used for generating keys for
 * 1.Semaphores
 * 2.Shared Memory Segments
 *-----------------------------------------------------------------------------
 */

/*
 * Next available Semaphore Key
 */
int get_sem_key(key_t *key_value)
{
	static key_t	new_key = 0;
	int		ret_value;
	int		sem_id;
	int		key_search_count;
	
	if (new_key == 0){
		new_key = SEM_KEY_START;
	}
	
	key_search_count = 0;

	while(1){
		if ((ret_value = sem_exist(new_key, &sem_id)) != SEM_SUCCESS){
			*key_value = new_key;
			break;
		}
		else{
			new_key++;
		}
		key_search_count++;
		if (key_search_count >= KEY_SEARCH_LIMIT){
			return(SEM_FAILURE);
		}
	}

	return(SEM_SUCCESS);
}

/*
 * Next available Shared Memory Segment Key
 */
int get_shm_key(key_t *key_value)
{
	static key_t	new_key = 0;
	int		ret_value;
	int		shm_id;
	int		key_search_count;
	
	if (new_key == 0){
		new_key = SHM_KEY_START;
	}

	key_search_count = 0;
	
	while(1){
		if ((ret_value = shm_exist(new_key, 0, &shm_id)) != SHM_SUCCESS){
			*key_value = new_key;
			break;
		}
		else{
			new_key++;
		}
		key_search_count++;
		if (key_search_count >= KEY_SEARCH_LIMIT){
			return(SHM_FAILURE);
		}
	}

	return(SHM_SUCCESS);
}





