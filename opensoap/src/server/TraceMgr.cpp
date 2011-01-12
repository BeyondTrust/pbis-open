/*
 *-----------------------------------------------------------------------------
 * File		: TraceMgr.cpp
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


#include	"TraceTypes.h"

#include	"TraceMgr.h"
#include	"SharedMgr.h"

extern int TraceLibInit(key_t key_value, char **filename)
{
	int		ret_value;
	
	ret_value = SharedLibInit(key_value, filename);

	return(ret_value);
}


extern int TraceLibTerm()
{
	int		ret_value;
	
	ret_value = SharedLibTerm();
	
	return(ret_value);
}


extern int WriteTraceObject(const char *filename, const char *trace)
{
	int		ret_value;
	
	ret_value = SharedLibAppend(filename, trace);

	return(ret_value);
}

extern int ReadTraceObject(const char *filename, long *address, char **trace, long length)
{
	int		ret_value;
	
	ret_value = 0;
//	ret_value = FileLibReadFile(filename, address, trace, length);

	return(ret_value);
}





