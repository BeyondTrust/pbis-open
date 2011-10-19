/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        provider-main_p.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        File Storage Provider
 * 
 *        Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PROVIDER_MAIN_H__
#define __PROVIDER_MAIN_H__

typedef struct __FILEDB_PROVIDER_CONTEXT
{
    HANDLE hRWLock;
} FILEDB_PROVIDER_CONTEXT, *PFILEDB_PROVIDER_CONTEXT;

DWORD
LwpsInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszName,
    PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
    );

DWORD
FileDB_OpenProvider(
    PHANDLE phProvider
    );

DWORD
FileDB_ReadPasswordByHostName(
    HANDLE hProvider,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
FileDB_ReadPasswordByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
FileDB_ReadHostListByDomainName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PSTR **ppszHostNames,
    DWORD *pdwNumEntries
    );

DWORD
FileDB_WritePassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
FileDB_DeleteAllEntries(
    HANDLE hProvider
    );

DWORD
FileDB_DeleteHostEntry(
    HANDLE hProvider,
    PCSTR  pszHostname
    );

VOID
FileDB_FreePassword(
   PLWPS_PASSWORD_INFO pInfo
   );

DWORD
FileDB_CloseProvider(
    HANDLE hProvider
    );

VOID
FileDB_FreePasswordInfo(
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
LwpsShutdownProvider(
    PSTR pszName,
    PLWPS_PROVIDER_FUNC_TABLE pFnTable
    );

VOID
LwpsFreeProviderContext(
    PFILEDB_PROVIDER_CONTEXT pContext
    );

#endif /* __PROVIDER_MAIN_H__ */
