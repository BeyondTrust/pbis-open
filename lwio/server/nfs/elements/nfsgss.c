/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Copyright Likewise Software
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
 *        nfsgss.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Elements
 *
 *        GSS Support
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"
#include "nfsgss_p.h"

NTSTATUS
NfsGssAcquireContext(
    PNFS_HOST_INFO pHostinfo,
    HANDLE         hGssOrig,
    PHANDLE        phGssNew
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_KRB5_CONTEXT pContext = (PNFS_KRB5_CONTEXT)hGssOrig;

    if (!pContext)
    {
        ntStatus = NfsGssNewContext(pHostinfo, &pContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        InterlockedIncrement(&pContext->refcount);
    }

    *phGssNew = (HANDLE)pContext;

cleanup:

    return ntStatus;

error:

    *phGssNew = (HANDLE)NULL;

    goto cleanup;
}

BOOLEAN
NfsGssNegotiateIsComplete(
    HANDLE hGss,
    HANDLE hGssNegotiate
    )
{
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PNFS_GSS_NEGOTIATE_CONTEXT)hGssNegotiate;
    return pGssNegotiate->state == NFS_GSS_CONTEXT_STATE_COMPLETE;
}

static
NTSTATUS
NfsGssGetSessionKey(
    gss_ctx_id_t Context,
    PBYTE* ppSessionKey,
    PDWORD pdwSessionKeyLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLength = 0;
    OM_uint32 gssMajor = GSS_S_COMPLETE;
    OM_uint32 gssMinor = 0;
    gss_buffer_set_t sessionKey = NULL;

    gssMajor = gss_inquire_sec_context_by_oid(
                    &gssMinor,
                    Context,
                    GSS_C_INQ_SSPI_SESSION_KEY,
                    &sessionKey);
    if (gssMajor != GSS_S_COMPLETE)
    {
        nfs_display_status("gss_inquire_sec_context_by_oid", gssMajor, gssMinor);
        // TODO - error code conversion
        status = gssMajor;
        BAIL_ON_LWIO_ERROR(status);
    }

    // The key is in element 0 and the key type OID is in element 1
    if (!sessionKey ||
        (sessionKey->count < 1) ||
        !sessionKey->elements[0].value ||
        (0 == sessionKey->elements[0].length))
    {
        LWIO_ASSERT_MSG(FALSE, "Invalid session key");
        status = STATUS_ASSERTION_FAILURE;
        BAIL_ON_LWIO_ERROR(status);
    }

    status = LW_RTL_ALLOCATE(&pSessionKey, BYTE, sessionKey->elements[0].length);
    BAIL_ON_LWIO_ERROR(status);

    memcpy(pSessionKey, sessionKey->elements[0].value, sessionKey->elements[0].length);
    dwSessionKeyLength = sessionKey->elements[0].length;

cleanup:
    gss_release_buffer_set(&gssMinor, &sessionKey);

    *ppSessionKey = pSessionKey;
    *pdwSessionKeyLength = dwSessionKeyLength;

    return status;

error:
    LWIO_SAFE_FREE_MEMORY(pSessionKey);
    dwSessionKeyLength = 0;

    goto cleanup;
}


static
NTSTATUS
NfsGssGetClientPrincipalName(
    gss_ctx_id_t Context,
    PSTR *ppszClientName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    OM_uint32 gssMajor = GSS_S_COMPLETE;
    OM_uint32 gssMinor = 0;
    gss_buffer_desc nameBuffer = {0};
    gss_buffer_set_t ClientName = NULL;
    PSTR pszClientPrincipalName = NULL;
    gss_name_t initiatorName = {0};
    gss_buffer_desc clientNameBuffer = {0};
    gss_OID nameOid = {0};

    /* Try the old way first */

    gssMajor = gss_inquire_context(
                   &gssMinor,
                   Context,
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
        BAIL_ON_SEC_ERROR(gssMajor);

        nameBuffer = clientNameBuffer;
    }
    else
    {
        /* Fallback to using the newer inquire_by_oid() method */

        gssMajor = gss_inquire_sec_context_by_oid(
                        &gssMinor,
                        Context,
                        GSS_C_NT_STRING_UID_NAME,
                        &ClientName);
        BAIL_ON_SEC_ERROR(gssMajor);

        if (!ClientName || (ClientName->count == 0))
        {
            ntStatus = STATUS_NONE_MAPPED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        nameBuffer = ClientName->elements[0];
    }

    ntStatus = NfsAllocateMemory(
                   (nameBuffer.length + 1) * sizeof(CHAR),
                   (PVOID*)&pszClientPrincipalName);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pszClientPrincipalName, nameBuffer.value, nameBuffer.length);
    pszClientPrincipalName[nameBuffer.length] = '\0';

    *ppszClientName = pszClientPrincipalName;

cleanup:

    gss_release_buffer_set(&gssMinor, &ClientName);
    gss_release_name(&gssMinor, &initiatorName);
    gss_release_buffer(&gssMinor, &clientNameBuffer);

    return ntStatus;

sec_error:

    ntStatus = LWIO_ERROR_GSS;

error:

    goto cleanup;
}

NTSTATUS
NfsGssGetSessionDetails(
    HANDLE hGss,
    HANDLE hGssNegotiate,
    PBYTE* ppSessionKey,
    PULONG pulSessionKeyLength,
    PSTR* ppszClientPrincipalName,
    LW_MAP_SECURITY_GSS_CONTEXT* pContextHandle
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PNFS_GSS_NEGOTIATE_CONTEXT)hGssNegotiate;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLength = 0;
    PSTR pszClientPrincipalName = NULL;

    if (!NfsGssNegotiateIsComplete(hGss, hGssNegotiate))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppSessionKey)
    {
        ntStatus = NfsGssGetSessionKey(
                        *pGssNegotiate->pGssContext,
                        &pSessionKey,
                        &dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppszClientPrincipalName)
    {
        ntStatus = NfsGssGetClientPrincipalName(
                       *pGssNegotiate->pGssContext,
                       &pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppszClientPrincipalName)
    {
        *ppszClientPrincipalName = pszClientPrincipalName;
    }

    if (ppSessionKey)
    {
        *ppSessionKey = pSessionKey;
        *pulSessionKeyLength = dwSessionKeyLength;
    }

    if (pContextHandle)
    {
        *pContextHandle = *pGssNegotiate->pGssContext;
    }

cleanup:

    return ntStatus;

error:

    if (ppSessionKey)
    {
        *ppSessionKey = NULL;
    }
    if (pulSessionKeyLength)
    {
        *pulSessionKeyLength = 0;
    }

    if (pSessionKey)
    {
        NfsFreeMemory(pSessionKey);
    }
    if (pszClientPrincipalName)
    {
        NfsFreeMemory(pszClientPrincipalName);
    }

    goto cleanup;
}

NTSTATUS
NfsGssBeginNegotiate(
    HANDLE  hGss,
    PHANDLE phGssResume
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate = NULL;

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_GSS_NEGOTIATE_CONTEXT),
                    (PVOID*)&pGssNegotiate);
    BAIL_ON_NT_STATUS(ntStatus);

    pGssNegotiate->state = NFS_GSS_CONTEXT_STATE_INITIAL;

    ntStatus = NfsAllocateMemory(
                    sizeof(gss_ctx_id_t),
                    (PVOID*)&pGssNegotiate->pGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    *pGssNegotiate->pGssContext = GSS_C_NO_CONTEXT;

    *phGssResume = (HANDLE)pGssNegotiate;

cleanup:

    return ntStatus;

error:

    *phGssResume = NULL;

    if (pGssNegotiate)
    {
        NfsGssEndNegotiate(hGss, (HANDLE)pGssNegotiate);
    }

    goto cleanup;
}

NTSTATUS
NfsGssNegotiate(
    HANDLE  hGss,
    HANDLE  hGssResume,
    PBYTE   pSecurityInputBlob,
    ULONG   ulSecurityInputBlobLen,
    PBYTE*  ppSecurityOutputBlob,
    ULONG*  pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_KRB5_CONTEXT pGssContext = (PNFS_KRB5_CONTEXT)hGss;
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PNFS_GSS_NEGOTIATE_CONTEXT)hGssResume;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLen = 0;

    switch(pGssNegotiate->state)
    {
        case NFS_GSS_CONTEXT_STATE_INITIAL:

            if (pSecurityInputBlob)
            {
                ntStatus = STATUS_INVALID_PARAMETER_3;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = NfsGssInitNegotiate(
                            pGssContext,
                            pGssNegotiate,
                            pSecurityInputBlob,
                            ulSecurityInputBlobLen,
                            &pSecurityBlob,
                            &ulSecurityBlobLen);

            break;

        case NFS_GSS_CONTEXT_STATE_HINTS:

            ntStatus = NfsGssContinueNegotiate(
                            pGssContext,
                            pGssNegotiate,
                            NULL,
                            0,
                            &pSecurityBlob,
                            &ulSecurityBlobLen);

            break;

        case NFS_GSS_CONTEXT_STATE_NEGOTIATE:

            if (!pSecurityInputBlob)
            {
                ntStatus = STATUS_INVALID_PARAMETER_3;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = NfsGssContinueNegotiate(
                            pGssContext,
                            pGssNegotiate,
                            pSecurityInputBlob,
                            ulSecurityInputBlobLen,
                            &pSecurityBlob,
                            &ulSecurityBlobLen);

            break;

        case NFS_GSS_CONTEXT_STATE_COMPLETE:

            break;

    }

    BAIL_ON_NT_STATUS(ntStatus);

error:

    *ppSecurityOutputBlob = pSecurityBlob;
    *pulSecurityOutputBloblen = ulSecurityBlobLen;

    return ntStatus;
}

VOID
NfsGssEndNegotiate(
    HANDLE hGss,
    HANDLE hGssResume
    )
{
    ULONG ulMinorStatus = 0;
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiateContext = NULL;

    pGssNegotiateContext = (PNFS_GSS_NEGOTIATE_CONTEXT)hGssResume;

    if (pGssNegotiateContext->pGssContext &&
        (*pGssNegotiateContext->pGssContext != GSS_C_NO_CONTEXT))
    {
        gss_delete_sec_context(
                        &ulMinorStatus,
                        pGssNegotiateContext->pGssContext,
                        GSS_C_NO_BUFFER);

        NfsFreeMemory(pGssNegotiateContext->pGssContext);
    }

    NfsFreeMemory(pGssNegotiateContext);
}

VOID
NfsGssReleaseContext(
    HANDLE hGss
    )
{
    PNFS_KRB5_CONTEXT pContext = (PNFS_KRB5_CONTEXT)hGss;

    if (InterlockedDecrement(&pContext->refcount) == 0)
    {
        NfsGssFreeContext(pContext);
    }
}

static
NTSTATUS
NfsGssNewContext(
    PNFS_HOST_INFO     pHostinfo,
    PNFS_KRB5_CONTEXT* ppContext
    )
{
    NTSTATUS ntStatus = 0;
    PSTR     pszCachePath = NULL;
    PNFS_KRB5_CONTEXT pContext = NULL;
    BOOLEAN  bInLock = FALSE;

    ntStatus = NfsAllocateMemory(sizeof(NFS_KRB5_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->refcount = 1;

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pHostinfo->mutex);

    if (pHostinfo->bIsJoined)
    {        
        ntStatus = NfsAllocateStringPrintf(
                       &pContext->pszMachinePrincipal,
                       "%s$@%s",
                       pHostinfo->pszHostname,
                       pHostinfo->pszDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bInLock, &pHostinfo->mutex);

        SMBStrToUpper(pContext->pszMachinePrincipal);

        ntStatus = SMBAllocateString(NFS_KRB5_CACHE_PATH, &pszCachePath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsGetTGTFromKeytab(
                       pContext->pszMachinePrincipal,
                       NULL,
                       pszCachePath,
                       &pContext->ticketExpiryTime);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext->pszCachePath = pszCachePath;
    }
    else
    {
        ntStatus = NfsAllocateStringPrintf(
                       &pContext->pszMachinePrincipal,
                       "%s",
                       pHostinfo->pszHostname);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppContext = pContext;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pHostinfo->mutex);

    return ntStatus;

error:

    *ppContext = NULL;

    if (pContext)
    {
        NfsGssFreeContext(pContext);
    }

    if (pszCachePath)
    {
        NfsFreeMemory(pszCachePath);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGssInitNegotiate(
    PNFS_KRB5_CONTEXT          pGssContext,
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    gss_buffer_desc input_name = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc input_desc = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_desc = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc OidString = GSS_C_EMPTY_BUFFER;
    gss_name_t      target_name = GSS_C_NO_NAME;
    gss_cred_id_t pServerCreds = NULL;
    PBYTE pSessionKey = NULL;
    ULONG ulSessionKeyLength = 0;
    ULONG ret_flags = 0;
    PSTR  pszCurrentCachePath = NULL;
    BOOLEAN bInLock = FALSE;
    gss_OID_set DesiredMechs = {0};
    PSTR pszGssNtlmOid = "1.3.6.1.4.1.311.2.2.10";
    gss_OID NtlmOid = NULL;
    static gss_OID_desc gss_spnego_mech_oid_desc =
      {6, (void *)"\x2b\x06\x01\x05\x05\x02"};

    LWIO_LOCK_MUTEX(bInLock, pGssContext->pMutex);

    ulMajorStatus = gss_create_empty_oid_set(&ulMinorStatus, &DesiredMechs);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    OidString.value  = pszGssNtlmOid;
    OidString.length = LwRtlCStringNumChars(pszGssNtlmOid);

    ulMajorStatus = gss_str_to_oid(&ulMinorStatus, &OidString, &NtlmOid);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    ulMajorStatus = gss_add_oid_set_member(&ulMinorStatus, NtlmOid, &DesiredMechs);
    BAIL_ON_SEC_ERROR(ulMajorStatus);
    
    /* only do the Krb5 setup if we have a valid principal name */

    if (!IsNullOrEmptyString(pGssContext->pszCachePath))
    {
        ntStatus = NfsGssRenew(pGssContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsSetDefaultKrb5CachePath(
                       pGssContext->pszCachePath,
                       &pszCurrentCachePath);
        BAIL_ON_NT_STATUS(ntStatus);

        ulMajorStatus = gss_add_oid_set_member(
                            &ulMinorStatus,
                            (gss_OID)gss_mech_krb5,
                            &DesiredMechs);
        BAIL_ON_SEC_ERROR(ulMajorStatus);
    }

    input_name.value = pGssContext->pszMachinePrincipal;
    input_name.length = strlen(pGssContext->pszMachinePrincipal);

    ulMajorStatus = gss_import_name(
                        (OM_uint32 *)&ulMinorStatus,
                        &input_name,
                        (gss_OID) gss_nt_krb5_name,
                        &target_name);
    nfs_display_status("gss_import_name", ulMajorStatus, ulMinorStatus);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    ulMajorStatus = gss_acquire_cred(
                        &ulMinorStatus,
                        NULL,
                        0,
                        DesiredMechs,
                        GSS_C_ACCEPT,
                        &pServerCreds,
                        NULL,
                        NULL);
    nfs_display_status("gss_acquire_sec_context", ulMajorStatus, ulMinorStatus);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    ulMajorStatus = gss_init_sec_context(
                            (OM_uint32 *)&ulMinorStatus,
                            pServerCreds,
                            pGssNegotiate->pGssContext,
                            target_name,
                            &gss_spnego_mech_oid_desc,
                            0,
                            0,
                            GSS_C_NO_CHANNEL_BINDINGS,
                            &input_desc,
                            NULL,
                            &output_desc,
                            &ret_flags,
                            NULL);

    nfs_display_status("gss_init_sec_context", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    switch (ulMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pGssNegotiate->state = NFS_GSS_CONTEXT_STATE_NEGOTIATE;

            break;

        case GSS_S_COMPLETE:

            pGssNegotiate->state = NFS_GSS_CONTEXT_STATE_COMPLETE;

            break;

        default:

            ntStatus = LWIO_ERROR_GSS;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (output_desc.length)
    {
        ntStatus = NfsAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pSessionKey);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSessionKey, output_desc.value, output_desc.length);

        ulSessionKeyLength = output_desc.length;
    }

    *ppSecurityOutputBlob = pSessionKey;
    *pulSecurityOutputBloblen = ulSessionKeyLength;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, pGssContext->pMutex);

    gss_release_buffer(&ulMinorStatus, &output_desc);
    gss_release_name(&ulMinorStatus, &target_name);
    gss_release_oid_set(&ulMinorStatus, &DesiredMechs);
    gss_release_cred(&ulMinorStatus, &pServerCreds);
    gss_release_oid(&ulMinorStatus, &NtlmOid);

    if (pGssNegotiate->pGssContext &&
        (*pGssNegotiate->pGssContext != GSS_C_NO_CONTEXT))
    {
        gss_delete_sec_context(
                        &ulMinorStatus,
                        pGssNegotiate->pGssContext,
                        GSS_C_NO_BUFFER);

        *pGssNegotiate->pGssContext = GSS_C_NO_CONTEXT;
    }

    if (pszCurrentCachePath)
    {
        NfsSetDefaultKrb5CachePath(
            pszCurrentCachePath,
            NULL);

        NfsFreeMemory(pszCurrentCachePath);
    }

    return ntStatus;

sec_error:

    ntStatus = LWIO_ERROR_GSS;

error:

    *ppSecurityOutputBlob = NULL;
    *pulSecurityOutputBloblen = 0;

    if (pSessionKey)
    {
        NfsFreeMemory(pSessionKey);
    }

    goto cleanup;
}

static
NTSTATUS
NfsGssContinueNegotiate(
    PNFS_KRB5_CONTEXT          pGssContext,
    PNFS_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    gss_buffer_desc input_desc = {0};
    gss_buffer_desc output_desc = {0};
    ULONG ret_flags = 0;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLength = 0;
    BOOLEAN bInLock = FALSE;

    static gss_OID_desc gss_spnego_mech_oid_desc =
                              {6, (void *)"\x2b\x06\x01\x05\x05\x02"};
    static gss_OID gss_spnego_mech_oid = &gss_spnego_mech_oid_desc;

    LWIO_LOCK_MUTEX(bInLock, pGssContext->pMutex);

    input_desc.length = ulSecurityInputBlobLen;
    input_desc.value = pSecurityInputBlob;

    ulMajorStatus = gss_accept_sec_context(
                        (OM_uint32 *)&ulMinorStatus,
                        pGssNegotiate->pGssContext,
                        NULL,
                        &input_desc,
                        NULL,
                        NULL,
                        &gss_spnego_mech_oid,
                        &output_desc,
                        &ret_flags,
                        NULL,
                        NULL);

    nfs_display_status("gss_accept_sec_context", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    switch (ulMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pGssNegotiate->state = NFS_GSS_CONTEXT_STATE_NEGOTIATE;

            break;

        case GSS_S_COMPLETE:

            pGssNegotiate->state = NFS_GSS_CONTEXT_STATE_COMPLETE;

            break;

        default:

            ntStatus = LWIO_ERROR_GSS;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (output_desc.length)
    {
        ntStatus = NfsAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pSecurityBlob);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSecurityBlob, output_desc.value, output_desc.length);

        ulSecurityBlobLength = output_desc.length;
    }

    *ppSecurityOutputBlob = pSecurityBlob;
    *pulSecurityOutputBloblen = ulSecurityBlobLength;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, pGssContext->pMutex);

    gss_release_buffer(&ulMinorStatus, &output_desc);

    return ntStatus;

sec_error:

    ntStatus = STATUS_LOGON_FAILURE;

error:

    *ppSecurityOutputBlob = NULL;
    *pulSecurityOutputBloblen = 0;

    if (pSecurityBlob)
    {
        NfsFreeMemory(pSecurityBlob);
    }

    goto cleanup;
}

static
VOID
NfsGssFreeContext(
    PNFS_KRB5_CONTEXT pContext
    )
{
    if (!IsNullOrEmptyString(pContext->pszCachePath))
    {
        NTSTATUS ntStatus = 0;

        ntStatus = NfsDestroyKrb5Cache(pContext->pszCachePath);
        if (ntStatus)
        {
            LWIO_LOG_ERROR("Failed to destroy kerberos cache path [%s][code:%d]",
                          pContext->pszCachePath,
                          ntStatus);
        }
    }

    if (pContext->pszCachePath)
    {
        NfsFreeMemory(pContext->pszCachePath);
    }
    if (pContext->pszMachinePrincipal)
    {
        NfsFreeMemory(pContext->pszMachinePrincipal);
    }

    if (pContext->pMutex)
    {
        pthread_mutex_destroy(pContext->pMutex);
    }
}

static
void
nfs_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
     nfs_display_status_1(pszId, maj_stat, GSS_C_GSS_CODE);
     nfs_display_status_1(pszId, min_stat, GSS_C_MECH_CODE);
}

static
void
nfs_display_status_1(
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
#ifdef WIN32
            case SEC_E_OK:
            case SEC_I_CONTINUE_NEEDED:
#else
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
            case 40157:   /* What minor code is this? */
#endif
                LWIO_LOG_VERBOSE("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
                break;

            default:

                LWIO_LOG_ERROR("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

static
NTSTATUS
NfsGssRenew(
   PNFS_KRB5_CONTEXT pContext
   )
{
    NTSTATUS ntStatus = 0;

    if (!pContext->ticketExpiryTime ||
        difftime(time(NULL), pContext->ticketExpiryTime) >  60 * 60)
    {
        ntStatus = NfsGetTGTFromKeytab(
                        pContext->pszMachinePrincipal,
                        NULL,
                        pContext->pszCachePath,
                        &pContext->ticketExpiryTime);
    }

    return ntStatus;
}

static
NTSTATUS
NfsGetTGTFromKeytab(
    PCSTR   pszUserName,
    PCSTR   pszPassword,
    PCSTR   pszCachePath,
    time_t* pGoodUntilTime
    )
{
    NTSTATUS ntStatus = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_creds creds = { 0 };
    krb5_ccache cc = NULL;
    krb5_keytab keytab = 0;
    krb5_principal client_principal = NULL;
    krb5_get_init_creds_opt opts;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszUserName, &client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_kt_default(ctx, &keytab);
    BAIL_ON_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    ret = krb5_get_init_creds_keytab(
                    ctx,
                    &creds,
                    client_principal,
                    keytab,
                    0,    /* start time     */
                    NULL, /* in_tkt_service */
                    &opts  /* options        */
                    );
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);

    if (pGoodUntilTime)
    {
        *pGoodUntilTime = creds.times.endtime;
    }

error:

    if (creds.client == client_principal)
    {
        creds.client = NULL;
    }

    if (ctx)
    {
        if (client_principal)
        {
            krb5_free_principal(ctx, client_principal);
        }

        if (keytab) {
            krb5_kt_close(ctx, keytab);
        }

        if (cc) {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_cred_contents(ctx, &creds);

        krb5_free_context(ctx);
    }

    return ntStatus;
}

static
NTSTATUS
NfsDestroyKrb5Cache(
    PCSTR pszCachePath
    )
{
    NTSTATUS ntStatus = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_destroy(ctx, cc);
    if (ret != 0) {
        if (ret != KRB5_FCC_NOFILE) {
            BAIL_ON_KRB_ERROR(ctx, ret);
        } else {
            ret = 0;
        }
    }

error:

    if (ctx)
    {
       krb5_free_context(ctx);
    }

    return ntStatus;
}

static
NTSTATUS
NfsSetDefaultKrb5CachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    // Set the default for gss
    ulMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&ulMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    if (ppszOrigCachePath)
    {
        if (!IsNullOrEmptyString(pszOrigCachePath))
        {
            ntStatus = SMBAllocateString(
                            pszOrigCachePath,
                            ppszOrigCachePath);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            *ppszOrigCachePath = NULL;
        }
    }

    LWIO_LOG_DEBUG("Cache path set to [%s]", SMB_SAFE_LOG_STRING(pszCachePath));

cleanup:

    return ntStatus;

sec_error:
error:

    if (ppszOrigCachePath)
    {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}

/**
 * Caller must not free the memory since it is reused
 **/

NTSTATUS
NfsGssNegHints(
    HANDLE hGssContext,
    PBYTE *ppNegHints,
    ULONG *pulNegHintsLength
    )
{
    NTSTATUS ntStatus      = STATUS_SUCCESS;
    HANDLE   hGssNegotiate = NULL;
    BOOLEAN  bInLock       = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gNfsElements.mutex);

    if (!gNfsElements.pHintsBuffer)
    {
        ntStatus = NfsGssBeginNegotiate(hGssContext, &hGssNegotiate);
        BAIL_ON_NT_STATUS(ntStatus);

        /* MIT Krb5 1.7 returns the NegHints blob if you call
           gss_accept_sec_context() with a NULL input buffer */

        ((PNFS_GSS_NEGOTIATE_CONTEXT)hGssNegotiate)->state = NFS_GSS_CONTEXT_STATE_HINTS;
        ntStatus = NfsGssNegotiate(
                       hGssContext,
                       hGssNegotiate,
                       NULL,
                       0,
                       &gNfsElements.pHintsBuffer,
                       &gNfsElements.ulHintsLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppNegHints        = gNfsElements.pHintsBuffer;
    *pulNegHintsLength = gNfsElements.ulHintsLength;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gNfsElements.mutex);

    if (hGssNegotiate)
    {
        NfsGssEndNegotiate(hGssContext, hGssNegotiate);
    }
    
    return ntStatus;

error:

    *ppNegHints = NULL;
    *pulNegHintsLength = 0;

    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
