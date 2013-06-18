/*
 *-----------------------------------------------------------------------------
 * File		: FIleMgr.h
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

#include	<errno.h>
#include	<fcntl.h>
#include	<sys/file.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<unistd.h>



int		FileLibPrep(const char *, bool);
int		FileLibCleanup(bool);
int		FileLibCreate(const char *);
int		FileLibExist(const char *);
int		FileLibDelete(const char *);

int		FileLibAppend(const char *, char *, long);
int		FileLibInit(const char *, int *);
int		FileLibTerm(int);

int		FileLibWriteFile(int, char *, long);
int		FileLibReadFile(char *, long *, char **, long);

int		FileLibLockFile(int);
int		FileLibUnlockFile(int);




