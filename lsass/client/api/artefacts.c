/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        groups.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        NSSArtefact Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaFindNSSArtefactByKey(
    HANDLE   hLsaConnection,
    DWORD    dwInfoLevel,
    PCSTR    pszKeyName,
    PCSTR    pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID*   ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    LSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ findNssArtefactByKeyReq;
    // Do not free pResultList and pError
    PLSA_NSS_ARTEFACT_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    findNssArtefactByKeyReq.dwInfoLevel = dwInfoLevel;
    findNssArtefactByKeyReq.pszMapName = pszMapName;
    findNssArtefactByKeyReq.pszKeyName = pszKeyName;
    findNssArtefactByKeyReq.dwFlags = dwFlags;

    request.tag = LSA_Q_FIND_NSS_ARTEFACT_BY_KEY;
    request.object = &findNssArtefactByKeyReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_FIND_NSS_ARTEFACT_BY_KEY_SUCCESS:
            pResultList = (PLSA_NSS_ARTEFACT_INFO_LIST)response.object;

            if (pResultList->dwNumNssArtefacts != 1)
            {
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            switch (pResultList->dwNssArtefactInfoLevel)
            {
                case 0:
                    *ppNSSArtefactInfo = pResultList->ppNssArtefactInfoList.ppInfoList0[0];
                    pResultList->ppNssArtefactInfoList.ppInfoList0[0] = NULL;
                    pResultList->dwNumNssArtefacts = 0;
                    break;

                default:
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_FIND_NSS_ARTEFACT_BY_KEY_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *ppNSSArtefactInfo = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaBeginEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwMaxNumNSSArtefacts,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    LSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ beginNssArtefactEnumReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    beginNssArtefactEnumReq.dwInfoLevel = dwInfoLevel;
    beginNssArtefactEnumReq.dwMaxNumNSSArtefacts = dwMaxNumNSSArtefacts;
    beginNssArtefactEnumReq.pszMapName = pszMapName;
    beginNssArtefactEnumReq.dwFlags = dwFlags;

    request.tag = LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS;
    request.object = &beginNssArtefactEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_SUCCESS:
            *phResume = response.object;
            break;
        case LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    *phResume = (HANDLE)NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    // Do not free pResultList and pError
    PLSA_NSS_ARTEFACT_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    request.tag = LSA_Q_ENUM_NSS_ARTEFACTS;
    request.object = hResume;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_ENUM_NSS_ARTEFACTS_SUCCESS:
            pResultList = (PLSA_NSS_ARTEFACT_INFO_LIST)response.object;
            *pdwNumNSSArtefactsFound = pResultList->dwNumNssArtefacts;
            switch (pResultList->dwNssArtefactInfoLevel)
            {
                case 0:
                    *pppNSSArtefactInfoList = (PVOID*)pResultList->ppNssArtefactInfoList.ppInfoList0;
                    pResultList->ppNssArtefactInfoList.ppInfoList0 = NULL;
                    pResultList->dwNumNssArtefacts = 0;
                    break;

                default:
                   dwError = LW_ERROR_INVALID_PARAMETER;
                   BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_ENUM_NSS_ARTEFACTS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    *pdwNumNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumNSSArtefacts(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    request.tag = LSA_Q_END_ENUM_NSS_ARTEFACTS;
    request.object = (LWMsgHandle*) hResume;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_END_ENUM_NSS_ARTEFACTS_SUCCESS:
            break;
        case LSA_R_END_ENUM_NSS_ARTEFACTS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    lwmsg_assoc_destroy_message(pContext->pAssoc, &response);
    lwmsg_session_release_handle(pContext->pSession, (LWMsgHandle*) hResume);

    return dwError;

error:

    if (response.object)
    {

    }

    goto cleanup;
}


DWORD
LsaValidateNSSArtefactInfoLevel(
    DWORD dwNSSArtefactInfoLevel
    )
{
    return ((dwNSSArtefactInfoLevel != 0) ? LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL : 0);
}
