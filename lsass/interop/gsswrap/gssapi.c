/*
 * Copyright Likewise Software
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
 *        gssutil.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        GSSAPI Wrappers
 *
 *        Utilities
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
void
LsaGssDisplayStatus(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    );

static
void
LsaGssDisplayStatus_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    );

DWORD
LsaGssCreateContext(
    PLSA_GSS_CONTEXT* ppContext
    )
{
    DWORD dwError = 0;
    PLSA_GSS_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), (PVOID*)&pContext);
    BAIL_ON_LSA_GSS_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    pContext->gssCtx = GSS_C_NO_CONTEXT;
    pContext->status = LSA_GSS_CONTEXT_STATUS_INIT;

    *ppContext = pContext;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaGssAuthenticate(
   PLSA_GSS_CONTEXT pContext,
   PBYTE            pInBytes,
   DWORD            dwInLength,
   PBYTE*           ppOutBytes,
   PDWORD           pdwOutBytes,
   PBOOLEAN         pbContinueNeeded
   )
{
    DWORD dwError = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    BOOLEAN bInLock = FALSE;
    gss_buffer_desc input_desc = {0};
    gss_buffer_desc output_desc = {0};
    PBYTE pOutBytes = NULL;
    DWORD dwOutBytes = 0;
    DWORD ret_flags = 0;
    static gss_OID_desc gss_spnego_mech_oid_desc =
                              {6, (void *)"\x2b\x06\x01\x05\x05\x02"};
    static gss_OID gss_spnego_mech_oid = &gss_spnego_mech_oid_desc;
    BOOLEAN bContinueNeeded = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);
    BAIL_ON_INVALID_POINTER(pInBytes);
    BAIL_ON_INVALID_POINTER(ppOutBytes);
    BAIL_ON_INVALID_POINTER(pdwOutBytes);
    BAIL_ON_INVALID_POINTER(pbContinueNeeded);

    LSA_GSS_LOCK_MUTEX(bInLock, &pContext->mutex);

    input_desc.length = dwInLength;
    input_desc.value = pInBytes;

    dwMajorStatus = gss_accept_sec_context(
                        (OM_uint32 *)&dwMinorStatus,
                        &pContext->gssCtx,
                        NULL,
                        &input_desc,
                        NULL,
                        NULL,
                        &gss_spnego_mech_oid,
                        &output_desc,
                        &ret_flags,
                        NULL,
                        NULL);

    LsaGssDisplayStatus(
        "gss_accept_sec_context",
        dwMajorStatus,
        dwMinorStatus);

    BAIL_ON_SEC_ERROR(dwMajorStatus, dwMinorStatus);

    switch (dwMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pContext->status = LSA_GSS_CONTEXT_STATUS_IN_PROGRESS;

            bContinueNeeded = TRUE;

            break;

        case GSS_S_COMPLETE:

            pContext->status = LSA_GSS_CONTEXT_STATUS_COMPLETE;

            bContinueNeeded = FALSE;

            break;

        default:

            dwError = LW_ERROR_GSS_CALL_FAILED;
            BAIL_ON_LSA_GSS_ERROR(dwError);
    }

    if (output_desc.length)
    {
        dwError = LwAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pOutBytes);
        BAIL_ON_LSA_GSS_ERROR(dwError);

        memcpy(pOutBytes, output_desc.value, output_desc.length);

        dwOutBytes = output_desc.length;
    }

    *ppOutBytes = pOutBytes;
    *pdwOutBytes = dwOutBytes;
    *pbContinueNeeded = bContinueNeeded;

cleanup:

    gss_release_buffer(&dwMinorStatus, &output_desc);

    LSA_GSS_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return dwError;

error:

    if (pOutBytes)
    {
        LwFreeMemory(pOutBytes);
    }
    if (ppOutBytes)
    {
        *ppOutBytes = NULL;
    }
    if (pdwOutBytes)
    {
        *pdwOutBytes = 0;
    }
    if (pbContinueNeeded)
    {
        *pbContinueNeeded = FALSE;
    }

    if (pContext)
    {
        pContext->status = LSA_GSS_CONTEXT_STATUS_ERROR;
    }

    goto cleanup;
}

DWORD
LsaGssGetRoles(
    PLSA_GSS_CONTEXT pContext,
    PSTR**           pppszRoles,
    PDWORD           pdwNumRoles
    )
{
    DWORD dwError = 0;
    PACCESS_TOKEN pAccessToken = NULL;
    PTOKEN_GROUPS pTokenInfo = NULL;
    DWORD dwSize = 0;
    DWORD iGroup = 0;
    PSTR* ppszSIDs = NULL;
    PSTR* ppszRoles = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD   dwNumRoles = 0;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    LSA_GSS_LOCK_MUTEX(bInLock, &pContext->mutex);

    if (pContext->status != LSA_GSS_CONTEXT_STATUS_COMPLETE)
    {
        dwError = ERROR_INVALID_STATE;
        BAIL_ON_LSA_GSS_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(
                    LwMapSecurityCreateAccessTokenFromGssContext(
                            gLsaGSSGlobals.pSecurityContext,
                            &pAccessToken,
                            pContext->gssCtx)
                    );
    BAIL_ON_LSA_GSS_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                    RtlQueryAccessTokenInformation(
                            pAccessToken,
                            TokenGroups,
                            pTokenInfo,
                            dwSize,
                            &dwSize)
                    );
    if (dwError == ERROR_INSUFFICIENT_BUFFER)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_GSS_ERROR(dwError);

    dwError = LwAllocateMemory(dwSize, (PVOID*)&pTokenInfo);
    BAIL_ON_LSA_GSS_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
                    RtlQueryAccessTokenInformation(
                            pAccessToken,
                            TokenGroups,
                            pTokenInfo,
                            dwSize,
                            &dwSize)
                    );
    BAIL_ON_LSA_GSS_ERROR(dwError);

    if (pTokenInfo->GroupCount)
    {
        LSA_QUERY_LIST queryList = {0};
        DWORD iGroup2 = 0;

        dwError = LwAllocateMemory(
                        sizeof(PSTR*) * pTokenInfo->GroupCount,
                        (PVOID*)&ppszSIDs);
        BAIL_ON_LSA_GSS_ERROR(dwError);

        for (iGroup = 0; iGroup < pTokenInfo->GroupCount; iGroup++)
        {
            dwError = LwNtStatusToWin32Error(
                            RtlAllocateCStringFromSid(
                                    &ppszSIDs[iGroup],
                                    pTokenInfo->Groups[iGroup].Sid)
                            );
            BAIL_ON_LSA_GSS_ERROR(dwError);
        }

        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_GSS_ERROR(dwError);

        queryList.ppszStrings = (PCSTR*)ppszSIDs;

        dwError = LsaFindObjects(
                    hLsaConnection,
                    NULL,
                    0,
                    LSA_OBJECT_TYPE_UNDEFINED,
                    LSA_QUERY_TYPE_BY_SID,
                    pTokenInfo->GroupCount,
                    queryList,
                    &ppObjects);
        BAIL_ON_LSA_GSS_ERROR(dwError);

        for (iGroup = 0; iGroup < pTokenInfo->GroupCount; iGroup++)
        {
            if (    ppObjects[iGroup] &&
                    ppObjects[iGroup]->type == LSA_OBJECT_TYPE_GROUP)
            {
                dwNumRoles++;
            }
        }

        if (dwNumRoles)
        {
            dwError = LwAllocateMemory(
                            sizeof(PSTR*) * dwNumRoles,
                            (PVOID*)&ppszRoles);
            BAIL_ON_LSA_GSS_ERROR(dwError);

            for (iGroup = 0; iGroup < pTokenInfo->GroupCount; iGroup++)
            {
                if (    ppObjects[iGroup] &&
                        ppObjects[iGroup]->type == LSA_OBJECT_TYPE_GROUP)
                {
                    dwError = LwAllocateString(
                                    ppObjects[iGroup]->groupInfo.pszUnixName,
                                    &ppszRoles[iGroup2++]);
                    BAIL_ON_LSA_GSS_ERROR(dwError);
                }
            }
        }
    }

    *pppszRoles = ppszRoles;
    *pdwNumRoles = dwNumRoles;

cleanup:

    if (pAccessToken)
    {
        RtlReleaseAccessToken(&pAccessToken);
    }

    if (ppObjects)
    {
        LsaFreeSecurityObjectList(pTokenInfo->GroupCount, ppObjects);
    }

    if (ppszSIDs)
    {
        LsaGssFreeStringArray(ppszSIDs, pTokenInfo->GroupCount);
    }

    if (pTokenInfo)
    {
        LwFreeMemory(pTokenInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    if (ppszRoles)
    {
        LsaGssFreeStringArray(ppszRoles, dwNumRoles);
    }

    goto cleanup;
}

DWORD
LsaGssGetClientPrincipalName(
    PLSA_GSS_CONTEXT pContext,
    PSTR*            ppszclientName
    )
{
    DWORD dwError = 0;
    OM_uint32 gssMajor = GSS_S_COMPLETE;
    OM_uint32 gssMinor = 0;
    gss_buffer_desc nameBuffer = {0};
    gss_buffer_set_t clientName = NULL;
    gss_name_t initiatorName = {0};
    gss_buffer_desc clientNameBuffer = {0};
    gss_OID nameOid = {0};
    PSTR pszClientPrincipalName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    LSA_GSS_LOCK_MUTEX(bInLock, &pContext->mutex);

    if (pContext->status != LSA_GSS_CONTEXT_STATUS_COMPLETE)
    {
        dwError = ERROR_INVALID_STATE;
        BAIL_ON_LSA_GSS_ERROR(dwError);
    }

    /* Try the old way first */

    gssMajor = gss_inquire_context(
                   &gssMinor,
                   pContext->gssCtx,
                   &initiatorName,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL);
    if (gssMajor == GSS_S_COMPLETE)
    {
        gssMajor = gss_display_name(
                       &gssMinor,
                       initiatorName,
                       &clientNameBuffer,
                       &nameOid);
        BAIL_ON_SEC_ERROR(gssMajor, gssMinor);

        nameBuffer = clientNameBuffer;
    }
    else
    {
        /* Fallback to using the newer inquire_by_oid() method */

        gssMajor = gss_inquire_sec_context_by_oid(
                        &gssMinor,
                        pContext->gssCtx,
                        GSS_C_NT_STRING_UID_NAME,
                        &clientName);
        BAIL_ON_SEC_ERROR(gssMajor, gssMinor);

        if (!clientName || (clientName->count == 0))
        {
            dwError = LW_ERROR_NOT_MAPPED;
            BAIL_ON_LSA_GSS_ERROR(dwError);
        }

        nameBuffer = clientName->elements[0];
    }

    dwError = LwAllocateMemory(
                   (nameBuffer.length + 1) * sizeof(CHAR),
                   (PVOID*)&pszClientPrincipalName);
    BAIL_ON_LSA_GSS_ERROR(dwError);

    memcpy(pszClientPrincipalName, nameBuffer.value, nameBuffer.length);
    pszClientPrincipalName[nameBuffer.length] = '\0';

    *ppszclientName = pszClientPrincipalName;

cleanup:

    LSA_GSS_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    gss_release_buffer_set(&gssMinor, &clientName);
    gss_release_name(&gssMinor, &initiatorName);
    gss_release_buffer(&gssMinor, &clientNameBuffer);

    return dwError;

error:

    dwError = LW_ERROR_GSS_CALL_FAILED;

    if (pszClientPrincipalName)
    {
        LwFreeMemory(pszClientPrincipalName);
    }

    goto cleanup;
}

VOID
LsaGssFreeContext(
    PLSA_GSS_CONTEXT pContext
    )
{
    if (pContext)
    {
        if (pContext->gssCtx != GSS_C_NO_CONTEXT)
        {
            DWORD dwMinorStatus = 0;

            gss_delete_sec_context(
                    &dwMinorStatus,
                    &pContext->gssCtx,
                    GSS_C_NO_BUFFER);
        }

        if (pContext->pMutex)
        {
            pthread_mutex_destroy(&pContext->mutex);
            // pContext->pMutex = NULL;
        }

        LwFreeMemory(pContext);
    }
}

static
void
LsaGssDisplayStatus(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
     LsaGssDisplayStatus_1(pszId, maj_stat, GSS_C_GSS_CODE);
     LsaGssDisplayStatus_1(pszId, min_stat, GSS_C_MECH_CODE);
}

static
void
LsaGssDisplayStatus_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    )
{
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1)
    {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        switch(code)
        {
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
            case 40157:   /* What minor code is this? */
                LSA_GSS_LOG_VERBOSE(
                        "GSS-API error calling %s: %d (%s)\n",
                        pszId,
                        code,
                        (char *)msg.value);
                break;

            default:

                LSA_GSS_LOG_ERROR(
                        "GSS-API error calling %s: %d (%s)\n",
                        pszId,
                        code,
                        (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
        {
            break;
        }
    }
}

