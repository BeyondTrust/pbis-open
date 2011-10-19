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
 *        lwps-provider_p.h
 * 
 *  Abstract:
 *  
 *        Likewise Password Storage (LWPS)
 *  
 *        Storage Provider API (Private Header)
 *  
 *  Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWPS_PROVIDER_P_H__
#define __LWPS_PROVIDER_P_H__

#define PSTOREDB_REGISTRY_DEFAULTS \
    "Services\\lsass\\Parameters\\Providers\\ActiveDirectory\\Pstore"

#define LWPS_REG_STORAGE_TYPE     "type"
#define LWPS_REG_PROVIDER_PATH    "path"
#define LWPS_REG_PROVIDER_DEFAULT "default"

typedef struct __LWPS_STORAGE_PROVIDER
{
    LwpsPasswordStoreType storeType;
    PSTR  pszId;
    PSTR  pszLibPath;
    PVOID pLibHandle;
    PFNLWPS_SHUTDOWN_PROVIDER pFnShutdown;
    PSTR  pszName;
    PLWPS_PROVIDER_FUNC_TABLE pFnTable;
    BOOLEAN bDefault;
} LWPS_STORAGE_PROVIDER, *PLWPS_STORAGE_PROVIDER;

DWORD
LwpsOpenProvider(
    LwpsPasswordStoreType storeType,
    PLWPS_STORAGE_PROVIDER* ppProvider
    );

DWORD
LwpsFindDefaultProvider(
    PLWPS_STACK* ppStack,
    PLWPS_STORAGE_PROVIDER* ppProvider
    );

DWORD
LwpsFindSpecificProvider(
    LwpsPasswordStoreType storeType,
    PLWPS_STACK* ppStack,
    PLWPS_STORAGE_PROVIDER* ppProvider
    );

DWORD
LwpsFindAllProviders(
    PLWPS_STACK* ppStack
    );

DWORD
LwpsConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

DWORD
LwpsConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

DWORD
LwpsConfigFreeProviderInStack(
    PVOID pItem,
    PVOID pData
    );

DWORD
LwpsInitProvider(
    PCSTR pszConfigPath,
    PLWPS_STORAGE_PROVIDER pProvider
    );

VOID
LwpsFreeProvider(
    PLWPS_STORAGE_PROVIDER pProvider
    );

DWORD
LwpsWritePasswordToStore(
    PVOID pItem,
    PVOID pData
    );

DWORD
LwpsDeleteEntriesInStore(
    PVOID pItem,
    PVOID pData
    );

DWORD
LwpsDeleteHostInStore(
    PVOID pItem,
    PVOID pData
    );


#endif /* __LWPS_PROVIDER_P_H__ */

