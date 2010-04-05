/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        provider-main.h
 *
 * Abstract:
 *
 *        Likewise Group Policy
 *
 *        LSASS Authentication Service Interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __AUTHSVC_H__
#define __AUTHSVC_H__

BOOLEAN
GPAKrb5TicketHasExpired(
    VOID
    );

DWORD
GPAGetKrb5CachePath(
    PSTR* ppszKrb5CachePath
    );

DWORD
GPAGetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  ppszCachePath,
    PDWORD pdwGoodUntilTime
    );

DWORD
GPADestroyKrb5Cache(
    PCSTR pszCachePath
    );

#endif

