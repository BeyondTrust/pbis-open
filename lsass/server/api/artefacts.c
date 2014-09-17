/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NSSArtefact Lookup and Management (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvFindNSSArtefactByKey(
    HANDLE hServer,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwMapInfoLevel,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (LW_IS_NULL_OR_EMPTY_STR(pszKeyName))
    {
        dwError = LW_ERROR_INVALID_NSS_KEY_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszMapName))
    {
        dwError = LW_ERROR_INVALID_NSS_MAP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwFlags)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnLookupNSSArtefactByKey(
                                            hProvider,
                                            pszKeyName,
                                            pszMapName,
                                            dwMapInfoLevel,
                                            dwFlags,
                                            ppNSSArtefactInfo);
        if (!dwError) {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_NSS_KEY) ||
                   (dwError == LW_ERROR_NO_SUCH_NSS_MAP)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    LSA_LOG_ERROR_API_FAILED(
            hServer,
            dwError,
            "find NIS Artefact by key (map = '%s', key = '%s')",
            LSA_SAFE_LOG_STRING(pszMapName),
            LSA_SAFE_LOG_STRING(pszKeyName));

    *ppNSSArtefactInfo = NULL;

    goto cleanup;
}

DWORD
LsaSrvBeginEnumNSSArtefacts(
    HANDLE hServer,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwNSSArtefactInfoLevel,
    DWORD  dwMaxNumNSSArtefacts,
    PHANDLE phState
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    dwError = LsaSrvCreateNSSArtefactEnumState(
                    hServer,
                    pszMapName,
                    dwFlags,
                    dwNSSArtefactInfoLevel,
                    dwMaxNumNSSArtefacts,
                    &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phState = pEnumState;

cleanup:

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(
            hServer,
            dwError,
            "start enumerating NIS Artefacts (map = '%s')",
            LSA_SAFE_LOG_STRING(pszMapName));

    goto cleanup;
}

DWORD
LsaSrvEnumNSSArtefacts(
    HANDLE  hServer,
    HANDLE  hState,
    PDWORD  pdwNSSArtefactInfoLevel,
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefactsFound
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = hState;
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsRemaining = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;
    dwNumNSSArtefactsRemaining = pEnumState->dwNumMaxRecords;

    while (dwNumNSSArtefactsRemaining &&
           pEnumState->pCurProviderState)
    {
        PLSA_SRV_PROVIDER_STATE pProviderState = pEnumState->pCurProviderState;
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        HANDLE hProvider = pProviderState->hProvider;
        HANDLE hResume = pProviderState->hResume;

        dwNumNSSArtefactsFound = 0;


        dwError = pProvider->pFnTable->pfnEnumNSSArtefacts(
                        hProvider,
                        hResume,
                        dwNumNSSArtefactsRemaining,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList);


        if (dwError) {
           if (dwError != LW_ERROR_NO_MORE_NSS_ARTEFACTS) {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }

        dwNumNSSArtefactsRemaining -= dwNumNSSArtefactsFound;

        if (dwNumNSSArtefactsRemaining) {
           pEnumState->pCurProviderState = pEnumState->pCurProviderState->pNext;
           if (dwError == LW_ERROR_NO_MORE_NSS_ARTEFACTS){
             dwError = 0;
           }
        }

        dwError = LsaAppendAndFreePtrs(
                        &dwTotalNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList);
        BAIL_ON_LSA_ERROR(dwError);

    }

    *pdwNSSArtefactInfoLevel = dwNSSArtefactInfoLevel;
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;

cleanup:

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:
    LSA_LOG_ERROR_API_FAILED(
            hServer,
            dwError,
            "continue enumerating NIS Artefacts");

    *pdwNSSArtefactInfoLevel = 0;
    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;


    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    if (ppNSSArtefactInfoList_accumulate) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList_accumulate, dwTotalNumNSSArtefactsFound);
    }

    goto cleanup;
}

DWORD
LsaSrvEndEnumNSSArtefacts(
    HANDLE hServer,
    HANDLE hState
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = hState;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    for (pProviderState = pEnumState->pProviderStateList;
         pProviderState;
         pProviderState = pProviderState->pNext)
    {
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        if (pProvider) {
           HANDLE hProvider = pProviderState->hProvider;
           pProvider->pFnTable->pfnEndEnumNSSArtefacts(
                                       hProvider,
                                       pProviderState->hResume);
        }
    }

    LsaSrvFreeEnumState(pEnumState);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;
}

