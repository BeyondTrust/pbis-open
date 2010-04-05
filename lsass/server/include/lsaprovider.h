/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSAPROVIDER_H__
#define __LSAPROVIDER_H__

#include "lsautils.h"

typedef DWORD (*PFNSHUTDOWNPROVIDER)(
    VOID
    );

typedef VOID  (*PFNCLOSEHANDLE)(HANDLE hProvider);

typedef BOOLEAN (*PFNSERVICESDOMAIN)(PCSTR pszDomain);

typedef DWORD (*PFNAUTHENTICATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNAUTHENTICATEUSEREX)(
                        HANDLE hProvider,
                        PLSA_AUTH_USER_PARAMS pUserParams,
                        PLSA_AUTH_USER_INFO *ppUserInfo
                        );

typedef DWORD (*PFNVALIDATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNCHECKUSERINLIST)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszListName);

typedef DWORD (*PFNCHANGEPASSWORD) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword,
                        PCSTR  pszOldPassword
                        );

typedef DWORD (*PFNSETPASSWORD) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNOPENSESSION) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId
                        );

typedef DWORD (*PFNCLOSESESSION) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId
                        );

typedef DWORD (*PFNLOOKUP_NSS_ARTEFACT_BY_KEY)(
                        HANDLE hProvider,
                        PCSTR  pszKeyName,
                        PCSTR  pszMapName,
                        DWORD  dwInfoLevel,
                        LSA_NIS_MAP_QUERY_FLAGS dwFlags,
                        PVOID* ppNSSArtefactInfo
                        );

typedef DWORD (*PFNBEGIN_ENUM_NSS_ARTEFACTS)(
                        HANDLE  hProvider,
                        DWORD   dwInfoLevel,
                        PCSTR   pszMapName,
                        LSA_NIS_MAP_QUERY_FLAGS dwFlags,
                        PHANDLE phResume
                        );

typedef DWORD (*PFNENUMNSS_ARTEFACTS) (
                        HANDLE  hProvider,
                        HANDLE  hResume,
                        DWORD   dwMaxNumGroups,
                        PDWORD  pdwGroupsFound,
                        PVOID** pppGroupInfoList
                        );

typedef VOID (*PFNEND_ENUM_NSS_ARTEFACTS)(
                        HANDLE hProvider,
                        HANDLE hResume
                        );

typedef DWORD (*PFNGET_STATUS)(
                HANDLE hProvider,
                PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus
                );

typedef VOID (*PFNFREE_STATUS)(
                PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus
                );

typedef DWORD (*PFNREFRESH_CONFIGURATION)();

typedef DWORD (*PFNPROVIDER_IO_CONTROL) (
                HANDLE hProvider,
                uid_t  peerUid,
                gid_t  peerGID,
                DWORD  dwIoControlCode,
                DWORD  dwInputBufferSize,
                PVOID  pInputBuffer,
                DWORD* pdwOutputBufferSize,
                PVOID* ppOutputBuffer
                );

#define LSA_SYMBOL_NAME_INITIALIZE_PROVIDER "LsaInitializeProvider2"

#endif /* __LSAPROVIDER_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
