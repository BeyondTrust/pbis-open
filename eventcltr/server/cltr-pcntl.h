/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server (Process Utilities)
 *
 */
#ifndef __EVTPCNTL_H__
#define __EVTPCNTL_H__


DWORD
CltrGetConfigPath(
    PSTR* ppszPath
    );

DWORD
CltrGetPrefixPath(
    PSTR* ppszPath
    );

DWORD
CltrGetMaxRecords(
    DWORD* pdwMaxRecords
    );

DWORD
CltrGetMaxAge(
    DWORD* pdwMaxLogSize
    );

DWORD
CltrGetMaxLogSize(
    DWORD* pdwMaxLogSize
    );

DWORD
CltrGetRemoveEventsFlag(
    PBOOLEAN pbRemoveEvents
    );

DWORD
CltrServerMain(
    int argc,
    PSTR argv[]
    );

void
CltrServerExit(
    int retCode
    );

#endif /* __EVTPCNTL_H__ */
