/*
 *-----------------------------------------------------------------------------
 * File		: SharedMgr.h
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


#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>


int SharedLibPrep(const char *, const char *, key_t, key_t *);
int SharedLibCleanup(const char *, const char *, bool);
int SharedLibSaveRootKey(const char *, key_t);
int SharedLibGetRootKey(const char *, key_t *);
int SharedLibRemoveRootKey(const char *);

int SharedLibInit(key_t, char **);
int SharedLibTerm(void);

int SharedLibAppend(const char *, const char *);
int SharedLibForcedFlush(const char *);
int SharedLibFlush(const char *, int);

