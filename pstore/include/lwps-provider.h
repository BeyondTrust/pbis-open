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
 *  Copyright (C) Likewise Software. All rights reserved.
 *
 *  Module Name:
 * 
 *        lwps-provider.h
 * 
 *  Abstract:
 *  
 *        Likewise Password Storage (LWPS)
 *  
 *        Storage Provider API
 *  
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWPS_PROVIDER_H__
#define __LWPS_PROVIDER_H__

#define LWPS_CFG_PROVIDER_TAG    "password storage:"

typedef DWORD (*PFNLWPS_STORE_OPEN_PROVIDER)(
                 PHANDLE phProvider
                 );

typedef DWORD (*PFNLWPS_STORE_READ_PASSWORD_BY_HOST_NAME)(
                 HANDLE hProvider,
                 PCSTR  pszHostname,
                 PLWPS_PASSWORD_INFO* ppInfo
                 );

typedef DWORD (*PFNLWPS_STORE_READ_PASSWORD_BY_DOMAIN_NAME)(
                 HANDLE hProvider,
                 PCSTR  pszDomainName,
                 PLWPS_PASSWORD_INFO* ppInfo
                 );

typedef DWORD (*PFNLWPS_STORE_READ_HOST_LIST_BY_DOMAIN_NAME)(
                 HANDLE hProvider,
                 PCSTR  pszDomainName,
                 PSTR **pppszHostnames,
                 DWORD *pdwHostname
                 );

typedef DWORD (*PFNLWPS_STORE_WRITE_PASSWORD)(
                 HANDLE hProvider,
                 PLWPS_PASSWORD_INFO pInfo
                 );

typedef DWORD (*PFNLWPS_STORE_DELETE_ALL_ENTRIES)(
                 HANDLE hProvider
                 );

typedef DWORD (*PFNLWPS_STORE_DELETE_HOST_ENTRY)(
                 HANDLE hProvider,
                 PCSTR pszHostname
                 );

typedef VOID (*PFNLWPS_STORE_FREE_PASSWORD)(
                 PLWPS_PASSWORD_INFO pInfo
                 );

typedef DWORD (*PFNLWPS_STORE_CLOSE_PROVIDER)(
                 HANDLE hProvider
                 ); 

typedef struct __LWPS_PROVIDER_FUNC_TABLE
{
    PFNLWPS_STORE_OPEN_PROVIDER  pFnOpenProvider;
    PFNLWPS_STORE_READ_PASSWORD_BY_HOST_NAME pFnReadPasswordByHostName;
    PFNLWPS_STORE_READ_PASSWORD_BY_DOMAIN_NAME pFnReadPasswordByDomainName;
    PFNLWPS_STORE_READ_HOST_LIST_BY_DOMAIN_NAME pFnReadHostListByDomainName;
    PFNLWPS_STORE_WRITE_PASSWORD pFnWritePassword;
    PFNLWPS_STORE_DELETE_ALL_ENTRIES pFnDeleteAllEntries;
    PFNLWPS_STORE_DELETE_HOST_ENTRY pFnDeleteHostEntry;
    PFNLWPS_STORE_FREE_PASSWORD  pfnFreePassword;
    PFNLWPS_STORE_CLOSE_PROVIDER pFnCloseProvider;
} LWPS_PROVIDER_FUNC_TABLE, *PLWPS_PROVIDER_FUNC_TABLE;

#define LWPS_SYMBOL_STORAGE_PROVIDER_INITIALIZE "LwpsInitializeProvider"
#define LWPS_SYMBOL_STORAGE_PROVIDER_SHUTDOWN   "LwpsShutdownProvider"

typedef DWORD (*PFNLWPS_INITIALIZE_PROVIDER)(
                  PCSTR pszConfigFilePath,
                  PSTR* ppszName,
                  PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
                  );

typedef DWORD (*PFNLWPS_SHUTDOWN_PROVIDER)(
                  PSTR pszName,
                  PLWPS_PROVIDER_FUNC_TABLE pFnTable
                  );

typedef struct _LWPS_STATIC_PROVIDER
{
    PCSTR pszId;
    PFNLWPS_INITIALIZE_PROVIDER pInitialize;
    PFNLWPS_SHUTDOWN_PROVIDER pShutdown;
} LWPS_STATIC_PROVIDER, *PLWPS_STATIC_PROVIDER;

#ifdef ENABLE_STATIC_PROVIDERS
#define LWPS_INITIALIZE_PROVIDER(name) LwpsInitializeProvider_##name
#define LWPS_SHUTDOWN_PROVIDER(name) LwpsShutdownProvider_##name
#else
#define LWPS_INITIALIZE_PROVIDER(name) LwpsInitializeProvider
#define LWPS_SHUTDOWN_PROVIDER(name) LwpsShutdownProvider
#endif

#define LWPS_STATIC_PROVIDER_ENTRY(name, id) \
    { #id, LWPS_INITIALIZE_PROVIDER(name), LWPS_SHUTDOWN_PROVIDER(name) }

#define LWPS_STATIC_PROVIDER_END \
    { NULL, NULL, NULL }

#endif /* __LWPS_PROVIDER_H__ */

