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
#ifndef __EVTRECORD_H__
#define __EVTRECORD_H__

typedef struct _EVENT_LOG_RECORD_W
{
    UINT64   qwEventRecordId;
    PWSTR pwszLogname;
    PWSTR  pwszEventType;
    DWORD   dwEventDateTime;
    PWSTR  pwszEventSource;
    PWSTR  pwszEventCategory;
    DWORD   dwEventSourceId;
    PWSTR  pwszUser;
    PWSTR  pwszComputer;
    PWSTR  pwszDescription;
    
    DWORD dwDataLen;
#if defined(_DCE_IDL_) || defined(__midl)
    [size_is(dwDataLen)]
#endif
    BYTE *pvData;
} EVENT_LOG_RECORD_W, *PEVENT_LOG_RECORD_W;

typedef struct _LWCOLLECTOR_RECORD
{
    UINT64 qwColRecordId;
    DWORD dwColDateTime;
    PWSTR pwszColComputer;
    PWSTR pwszColComputerAddress;
    EVENT_LOG_RECORD_W event;
} LWCOLLECTOR_RECORD, *PLWCOLLECTOR_RECORD;

#endif /* __EVTRECORD_H__ */
