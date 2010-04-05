/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog parser to help parse log file output from Microsoft Event Viewer.
 *
 */
#ifndef __EVTPARSER_H__
#define __EVTPARSER_H__

DWORD
PrintEventRecords(
    FILE* output, 
    LWCOLLECTOR_RECORD* eventRecords, 
    DWORD nRecords,
    PDWORD totalRecords
    );

DWORD
PrintEventRecordsTable(
    FILE* output, 
    LWCOLLECTOR_RECORD* eventRecords, 
    DWORD nRecords,
    PDWORD totalRecords
    );

#endif
