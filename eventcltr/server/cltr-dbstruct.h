/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server structures
 *
 */
#ifndef __EVTDBSTRUCT_H__
#define __EVTDBSTRUCT_H__

typedef struct __EVENTLOG_CONTEXT
{
    sqlite3* pDbHandle;
} EVENTLOG_CONTEXT, *PEVENTLOG_CONTEXT;
    
#endif /* __EVTDBSTRUCT_H__ */
