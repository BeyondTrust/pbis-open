/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        eventdlapi.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Eventlog API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#ifndef __EVENTDLAPI_H__
#define __EVENTDLAPI_H__

#ifdef _WIN32
#include "Stdafx.h"
#endif


typedef VOID  (*PFN_FREE_EVENT_RECORD)(
			PEVENT_LOG_RECORD pEventRecord
			);

typedef DWORD (*PFN_OPEN_EVENT_LOG) (
                        PCSTR   pszServerName,
                        PHANDLE phEventLog
                        );

typedef DWORD (*PFN_CLOSE_EVENT_LOG)(
			HANDLE hEventLog
			);

typedef DWORD (*PFN_WRITE_EVENT_LOG_BASE)(
    			HANDLE hEventLog,
    			EVENT_LOG_RECORD eventRecord
    			);

typedef struct __EVENTAPIFUNCTIONTABLE
{
	PFN_FREE_EVENT_RECORD      pfnFreeEventRecord;
	PFN_OPEN_EVENT_LOG         pfnOpenEventLog;
	PFN_CLOSE_EVENT_LOG        pfnCloseEventLog;
	PFN_WRITE_EVENT_LOG_BASE   pfnWriteEventLogBase;

} EVENTAPIFUNCTIONTABLE, *PEVENTAPIFUNCTIONTABLE;

#define EVENTAPI_FREE_EVENT_RECORD_FUNCTION     "LWIFreeEventRecord"
#define EVENTAPI_OPEN_EVENT_LOG_FUNCTION     "LWIOpenEventLog"
#define EVENTAPI_CLOSE_EVENT_LOG_FUNCTION       "LWICloseEventLog"
#define EVENTAPI_WRITE_EVENT_LOG_BASE_FUNCTION  "LWIWriteEventLogBase"


#endif /* __EVENTDLAPI_H__ */
