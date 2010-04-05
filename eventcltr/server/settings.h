/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Collector server settings interface
 *
 */

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

DWORD
CltrGetRecordsPerPeriod(
    PDWORD pdwRecords
    );

DWORD
CltrGetRecordsPerBatch(
    PDWORD pdwRecords
    );

DWORD
CltrGetPeriodSeconds(
    PDWORD pdwSecs
    );

DWORD
CltrGetDatabasePath(
    OUT PWSTR* ppwszPath
    );

DWORD
CltrGetLogPath(
   OUT PWSTR* ppwszPath
   );

DWORD
CltrGetLogLevel(
    CltrLogLevel* pdwSecs
    );

DWORD
CltrGetRemoteSecurityDescriptor(
    PSECURITY_DESCRIPTOR* ppDescriptor
    );

#endif /* __SETTINGS_H__ */
