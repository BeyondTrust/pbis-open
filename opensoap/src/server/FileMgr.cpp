/*
 *-----------------------------------------------------------------------------
 * File		: FileMgr.cpp
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP File Manager
 * Date		: 2003/12/17
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */


#include	"TraceTypes.h"
#include	"FileMgr.h"

/*
 * Default/Base log file
 */
#define		TRACELOGFILE		"/usr/local/opensoap/var/log/TraceLog"


/*
 *-----------------------------------------------------------------------------
 * These should only be called by the process initiator via shmmgr_prep(),
 * and not by any other functions. 
 * Sequence : FileLibExist() -> FileLibCreate()
 *-----------------------------------------------------------------------------
 */

/*
 * Boolean to indicate truncation on prep
 */
extern int FileLibPrep(const char *logfile, bool trunc)
{
	int		ret_value;
	
	/*
	 * Once off processing at the startup of the OpenSOAP server group
	 * of processes.
	 *  - Verify existence of log file
	 *  - Create if necessary
	 *  - Verify file can be opened for R/W
	 *  - Verify file can be locked/unlocked
	 *  - etc
	 */
	
	if (FileLibExist(logfile) == FILE_FAILURE){
		if ((ret_value = FileLibCreate(logfile)) == FILE_FAILURE){
			ret_value = FILE_FAILURE;
		}
	}
	else{
		if (trunc){
			FileLibDelete(logfile);
		}
		FileLibCreate(logfile);
		ret_value = FILE_SUCCESS;
	}
	
	return(ret_value);
}

/*
 * Boolean to indicate automatic backup on exit
 */
extern int FileLibCleanup(bool backup)
{
	if (backup){
		;
	}

	return(FILE_SUCCESS);
}

extern int FileLibCreate(const char *logfilename)
{
	int			log_fd;
	int			ret_value;
	
	if ((log_fd = open(logfilename, O_CREAT|O_RDWR, 0666)) < 0){
		switch(errno){
			default :
				break;
		}
		ret_value = FILE_FAILURE;
	}
	else{
		ret_value = FILE_SUCCESS;
		close(log_fd);
	}

	return(ret_value);
}

extern int FileLibExist(const char *logfilename)
{
	struct stat		logfile_stat;
	int			ret_value;
	
	
	if (stat(logfilename, &logfile_stat) < 0){
		switch(errno){
			default : 
				ret_value = FILE_FAILURE;
				break;
		}
	}
	else{
		ret_value = FILE_SUCCESS;
	}

	return(ret_value);
}

extern int FileLibDelete(const char *logfilename)
{
	int		ret_value;
	
	if (unlink(logfilename) < 0){
		switch(errno){
			default : 
				break;
		}
		ret_value = FILE_FAILURE;
	}
	else{
		ret_value = FILE_SUCCESS;
	}

	return(ret_value);
}

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */


extern int FileLibAppend(const char *filename, char *data, long length)
{
	int		ret_value;
	int		fd;
	
	if (FileLibExist(filename) == FILE_FAILURE){
		if ((ret_value = FileLibCreate(filename)) == FILE_FAILURE){
			return(ret_value);
		}
	}
	
	if ((ret_value = FileLibInit(filename, &fd)) == FILE_FAILURE){
		return(ret_value);
	}
	
	FileLibWriteFile(fd, data, length);
	
	FileLibTerm(fd);
	
	ret_value = FILE_SUCCESS;

	return(ret_value);
}

extern int FileLibInit(const char *filename, int *fd)
{
	int		ret_value;
	/*
	 * Open Log file for reading & writing
	 * Save file descriptor
	 */

	if ((*fd = open(filename, O_RDWR)) < 0){
		ret_value = FILE_FAILURE;
	}
	else{
		ret_value = FILE_SUCCESS;
	}

	return(ret_value);
}

extern int FileLibTerm(int fd)
{
	int		ret_value;
	/*
	 * Close the Log file
	 */

	if (close(fd) < 0){
		ret_value = FILE_FAILURE;
	}
	else{
		ret_value = FILE_SUCCESS;
	}
	return(ret_value);
}


/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

extern int FileLibWriteFile(int fd, char *data, long length)
{
	int		ret_value;
	
	FileLibLockFile(fd);
	
	lseek(fd, 0, SEEK_END);
	write(fd, (char *)data, length);

	FileLibUnlockFile(fd);
	
	return(FILE_SUCCESS);
}

extern int FileLibReadFile(char *filename, long *address, char **data, long length)
{

	return(FILE_SUCCESS);
}

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

extern int FileLibLockFile(int fd)
{
	struct flock		log_lock;
	int			ret_value;
	
	log_lock.l_type = F_WRLCK;
	log_lock.l_whence = 0;
	log_lock.l_start = 0;
	log_lock.l_len = 0;

	while ((ret_value = fcntl(fd, F_SETLK, &log_lock)) < 0){
		;
	}
	
	/*
	 * Should start a timer here to prevent indefinite file locking
	 */

	return(FILE_SUCCESS);
}

extern int FileLibUnlockFile(int fd)
{
	struct flock		log_lock;
	int			ret_value;
	
	log_lock.l_type = F_UNLCK;
	log_lock.l_whence = 0;
	log_lock.l_start = 0;
	log_lock.l_len = 0;

	while ((ret_value = fcntl(fd, F_SETLK, &log_lock)) < 0){
		;
	}

	return(FILE_SUCCESS);
}

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */







