/*
 *-----------------------------------------------------------------------------
 * File		: TraceMgr.h
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP Trace Manager
 * Date		: 2003/12/17
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>


int		TraceLibInit(key_t, char **);
int		TraceLibTerm(void);
int		WriteTraceObject(const char *, const char *);
int		ReadTraceObject(const char *, long *, char **, long);





