/*
 *-----------------------------------------------------------------------------
 * File		: TraceLog.h
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP TraceLog Class
 * Date		: 2003/12/17
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include	<iostream>
#include	<string>
#include	<sstream>
using namespace std;

#include	<stdio.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<time.h>
#include	<unistd.h>

#include	"ServerCommon.h"

#include	"ProcessInfo.h"
#include	"TraceMgr.h"
#include	"FileMgr.h"
using namespace std;
using namespace OpenSOAP;


#define		DELIMIT		","


/*
 * Trace Logger Classes
 */

class TraceLog {
	private:
		bool		initialized;
		string		log_filename;
		string		log_time;	// Hidden
		string		proc_name;
		int		proc_id;
		string		trace_string;	// Build this!
                //extend
                string formatedData;
	
	protected:
	
	public:
		//string		request_id;
#if 0
		string		message_id;	// From client
		string		job_id;
		string		client;
		string		action;
		string		direction;
		string		in;
		string		service;
		string		method;
#endif //if 0
		string		sv_node;
		int		status;
		int		level;
		
		string		module;		// Local stuff
		string		function;
		int		ret_code;
		int		det_code;
		string		comment;
		
		TraceLog();			// Constructors
		TraceLog(int);
		TraceLog(int, string);		
		~TraceLog();			// Destructor

		int TraceInit(int);
		
		int TraceTerm();

		int TraceUpdate();
                //extend
		int TraceUpdate(pthread_t tid); 

		int TraceWrite();
		int TraceRead();
		int DelimitAppend(string);	// Append with de-limit character
		int DelimitAppend(int);		// Overload
		
		//int SetRequestID(string);

                //extend
                int SetFormatedData(const string& data) {
                    formatedData = data;
                    return 0;
                }

#if 0
		int SetMessageID(string);
		int SetJobID(string);
		int SetClient(string);
		int SetAction(string);
		int SetDirection(string);
		int SetIn(string);
		int SetService(string);
		int SetMethod(string);
#endif //if 0
		int SetSvNode(string);
		int SetStatus(int);
		int SetLevel(int);
		int SetModule(string);
		int SetFunction(string);
		int SetRetCode(int);
		int SetDetCode(int);
		int SetComment(string);
		
};

