/* -*- mode: c++; -*-
 *-----------------------------------------------------------------------------
 * $RCSfile: ThreadInfo.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#ifndef _ThreadInfo_H
#define _ThreadInfo_H

#include <pthread.h>
#include <sys/time.h>
#include "MsgInfo.h"

#include "ServerDef.h"

namespace OpenSOAP {

class ThreadInfo {
	private:
		pid_t		ProcessID;		//unix process id
		pthread_t	ThreadID;		//thread id
		struct	timeval	CreateTime;	//thread create time

	protected:
		ThreadInfo * ThreadDataPtr;	//Thread Data Access Pointer

	public:
		MsgInfo* msginfo;
	
	public:
		ThreadInfo();
		ThreadInfo(ThreadInfo& ti);
		virtual ~ThreadInfo();

		MsgInfo	*GetMsgInfo();
		pid_t	GetPID();
		pthread_t	GetTID();
		int		SetMsgInfo(MsgInfo * mi);
		struct timeval * GetCreateTime();
}; //class ThreadInfo

} //namespace OpenSOAP

#endif //_ThreadInfo_H

