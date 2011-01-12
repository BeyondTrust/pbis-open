/*
 *-----------------------------------------------------------------------------
 * File		: TraceLog.cpp
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP TraceLog Class Methods
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
#include	"TraceLog.h"


TraceLog::TraceLog()
{

	return;
}

TraceLog::TraceLog(int key_value)
{

	TraceInit(key_value);

	proc_name = "Unknown process";
	
/*
 * Get PID & set proc_id
 */
	proc_id = getpid();

	return;
}

TraceLog::TraceLog(int key_value, string pname)
{

	TraceInit(key_value);

	proc_name = pname;

	proc_id = getpid();
	
	return;
}

TraceLog::~TraceLog()
{

	return;
}

int TraceLog::TraceInit(int key_value)
{
	char		*filename = NULL;
	
	initialized = false;
	
	trace_string="";
	
	log_filename="";

#if 0	
	request_id.clear();
	message_id.clear();
	job_id.clear();
	client.clear();
	action.clear();
	direction.clear();
	in.clear();
	service.clear();
	method.clear();
#endif //if 0
	sv_node="";
	status = 0;
	level = 0;
	log_time="";
	proc_name="";
	proc_id = 0;
	module="";
	function="";
	ret_code = 0;
	det_code = 0;
	comment="";

	if (TraceLibInit(key_value, &filename) == GEN_SUCCESS){
		initialized = true;
	}

	if (filename != NULL){
		log_filename = filename;
		free(filename);
	}
	
	return(0);
}

int TraceLog::TraceTerm()
{

	TraceLibTerm();
	
	return(0);
}

int TraceLog::TraceUpdate()
{

//	fprintf(stderr, "TraceLog::TraceUpdate()\n");

	MsgInfo * mgif = ProcessInfo::GetThreadInfo()->GetMsgInfo();

        SetFormatedData(mgif->toString());
#if 0
	SetRequestID(mgif->GetRequestID());
	SetMessageID(mgif->GetMessageID());
	SetJobID(mgif->GetJobID());
	SetClient(mgif->GetClient());
	SetAction(mgif->GetAction());
	SetDirection(mgif->GetDirection());
	SetIn(mgif->GetIn());
	SetService(mgif->GetService());
	SetMethod(mgif->GetServiceMethod());
#endif

	TraceWrite();

	return(0);
}

//extend
int TraceLog::TraceUpdate(pthread_t tid)
{
	
	MsgInfo * mgif = ProcessInfo::GetThreadInfo(tid)->GetMsgInfo();

        SetFormatedData(mgif->toString());

	TraceWrite();

	return(0);
    
}


int TraceLog::TraceWrite()
{
	struct tm	logtime;
	struct timeval	now_time;
	char		time_buf[32];
	char		ms_str[8];

	trace_string="";

/*
 * Get current date & time & set log_time
 */
	gettimeofday(&now_time, NULL);
	localtime_r((time_t*)&now_time.tv_sec, &logtime);
	strftime(time_buf, 31, "%F %T.", &logtime);
	sprintf(ms_str, "%0.3f", ((float)now_time.tv_usec/(float)1000000.0));
	strcat(time_buf, &ms_str[2]);
	
	log_time = time_buf;

/*
 * Correlate the data, and invoke the Trace Library
 */

//extend
/* SOAP-Message Data Area - string */
        DelimitAppend(formatedData);

#if 0
/* Request ID - string */
	DelimitAppend(request_id);
/* Message ID - string */
	DelimitAppend(message_id);

/* Job ID - string */
	DelimitAppend(job_id);

/* Client - string */
	DelimitAppend(client);

/* Action - string */
	DelimitAppend(action);

/* Direction - string */
	DelimitAppend(direction);

/* In - bool */
	DelimitAppend(in);

/* Service - string */
	DelimitAppend(service);

/* Method - string */
	DelimitAppend(method);
#endif //if 0

/* SV_Node - string */
	DelimitAppend(sv_node);

/* Status - int */
	DelimitAppend(status);

/* Level - int */
	DelimitAppend(level);

/* Log Time - string */
	DelimitAppend(log_time);

/* Proc Name - string */
	DelimitAppend(proc_name);

/* Proc ID - int */
//	DelimitAppend(proc_id);
	DelimitAppend(getpid());

/* Module - string */
	DelimitAppend(module);

/* Function - string */
	DelimitAppend(function);

/* Ret Code - int */
	DelimitAppend(ret_code);

/* Det Code - int */
	DelimitAppend(det_code);

/* Comment - string */
	trace_string.append(comment);
	
	trace_string.append("\n");

	if (initialized){
		if (WriteTraceObject(log_filename.c_str(), trace_string.c_str()) != GEN_SUCCESS){
			fprintf(stderr, "TraceLog::TraceWrite() Error writing shared memory [%s]\n",
				trace_string.c_str());
		}
	}

	return(0);
}

int TraceLog::TraceRead()
{

	return(0);
}

int TraceLog::DelimitAppend(string append_string)
{

	trace_string.append(append_string);
	trace_string.append(DELIMIT);

	return(0);
}

int TraceLog::DelimitAppend(int append_int)
{
	ostringstream		oss;
	
	oss << append_int;
	trace_string.append(oss.str());
	trace_string.append(DELIMIT);

	return(0);
}

//----------------------------------------------
#if 0
int TraceLog::SetRequestID(string req_id)
{

	request_id = req_id;

	return(0);
}

int TraceLog::SetMessageID(string msg_id)
{

	message_id = msg_id;

	return(0);
}

int TraceLog::SetJobID(string job)
{

	job_id = job;

	return(0);
}

int TraceLog::SetClient(string client_key)
{

	client = client_key;

	return(0);
}

int TraceLog::SetAction(string act)
{

	action = act;

	return(0);
}

int TraceLog::SetDirection(string dir)
{

	direction = dir;

	return(0);
}

int TraceLog::SetIn(string inflag)
{

	in = inflag;

	return(0);
}

int TraceLog::SetService(string serv_name)
{

	service = serv_name;
	
	return(0);
}

int TraceLog::SetMethod(string meth_name)
{

	method = meth_name;

	return(0);
}
#endif //if 0
//----------------------------------------------

int TraceLog::SetSvNode(string serv_node)
{

	sv_node = serv_node;
	
	return(0);
}

int TraceLog::SetStatus(int status_code)
{

	status = status_code;
	
	return(0);
}

int TraceLog::SetLevel(int trace_level)
{

	level = trace_level;

	return(0);
}

/*
 * The following change as we move around the program
 */

int TraceLog::SetModule(string mod_name)
{

	module = mod_name;
	
	return(0);
}

int TraceLog::SetFunction(string func_name)
{

	function = func_name;
	
	return(0);
}

int TraceLog::SetRetCode(int return_code)
{

	ret_code = return_code;

	return(0);
}

int TraceLog::SetDetCode(int detail_code)
{

	det_code = detail_code;

	return(0);
}

int TraceLog::SetComment(string extra_comment)
{

	comment = extra_comment;

	return(0);
}






