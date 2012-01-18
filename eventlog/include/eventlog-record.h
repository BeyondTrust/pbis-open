/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog definitions
 *
 */
#ifndef __EVENTLOG_RECORD_H__
#define __EVENTLOG_RECORD_H__

typedef struct _LW_EVENTLOG_RECORD
{
    UINT64 EventRecordId;
    PWSTR pLogname;
    PWSTR pEventType;
    // Seconds since 1970
    UINT64 EventDateTime;
    PWSTR pEventSource;
    PWSTR pEventCategory;
    DWORD EventSourceId;
    PWSTR pUser;
    PWSTR pComputer;
    PWSTR pDescription;
    
    DWORD DataLen;
#if defined(_DCE_IDL_) || defined(__midl)
    [size_is(DataLen)]
#endif
    BYTE *pData;
} LW_EVENTLOG_RECORD, *PLW_EVENTLOG_RECORD;

#endif /* __EVENTLOG_RECORD_H__ */
