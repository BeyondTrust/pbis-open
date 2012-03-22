/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

static
NTSTATUS
LwIoCredentialCacheToTgt(
    PIO_CREDS pCacheToken,
    PIO_CREDS* ppCreds
    );

NTSTATUS
LwIoCreatePlainCredsA(
    PCSTR pszUsername,
    PCSTR pszDomain,
    PCSTR pszPassword,
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszPassword = NULL;
    
    Status = LwRtlWC16StringAllocateFromCString(&pwszUsername, pszUsername);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(&pwszDomain, pszDomain);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(&pwszPassword, pszPassword);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCreatePlainCredsW(pwszUsername, pwszDomain, pwszPassword, ppCreds);
    BAIL_ON_NT_STATUS(Status);

error:
    
    IO_SAFE_FREE_MEMORY(pwszUsername);
    IO_SAFE_FREE_MEMORY(pwszDomain);
    IO_SAFE_FREE_MEMORY(pwszPassword);

    return Status;
}

NTSTATUS
LwIoCreatePlainCredsW(
    PCWSTR pwszUsername,
    PCWSTR pwszDomain,
    PCWSTR pwszPassword,
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_CREDS pCreds = NULL;

    Status = LwIoAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_NT_STATUS(Status);

    pCreds->type = IO_CREDS_TYPE_PLAIN;
    
    Status = RtlWC16StringDuplicate(
        &pCreds->payload.plain.pwszUsername,
        pwszUsername);
    BAIL_ON_NT_STATUS(Status);

   Status = RtlWC16StringDuplicate(
        &pCreds->payload.plain.pwszDomain,
        pwszDomain);

    Status = RtlWC16StringDuplicate(
        &pCreds->payload.plain.pwszPassword,
        pwszPassword);
    BAIL_ON_NT_STATUS(Status);
    
    *ppCreds = pCreds;
    
cleanup:

    return Status;

error:

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}

NTSTATUS
LwIoCreateKrb5CredsA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwszPrincipal = NULL;
    PWSTR pwszCachePath = NULL;
    
    Status = LwRtlWC16StringAllocateFromCString(&pwszPrincipal, pszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(&pwszCachePath, pszCachePath);
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoCreateKrb5CredsW(pwszPrincipal, pwszCachePath, ppCreds);
    BAIL_ON_NT_STATUS(Status);

error:
    
    IO_SAFE_FREE_MEMORY(pwszPrincipal);
    IO_SAFE_FREE_MEMORY(pwszCachePath);

    return Status;
}

NTSTATUS
LwIoCreateKrb5CredsW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_CREDS pCreds = NULL;

    Status = LwIoAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_NT_STATUS(Status);

    pCreds->type = IO_CREDS_TYPE_KRB5_CCACHE;
    
    Status = RtlWC16StringDuplicate(
        &pCreds->payload.krb5Ccache.pwszPrincipal,
        pwszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = RtlWC16StringDuplicate(
        &pCreds->payload.krb5Ccache.pwszCachePath,
        pwszCachePath);
    BAIL_ON_NT_STATUS(Status);
    
    *ppCreds = pCreds;
    
cleanup:

    return Status;

error:

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NTSTATUS
LwIoCopyCreds(
    PIO_CREDS pCreds,
    PIO_CREDS* ppCredsCopy
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_CREDS pCredsCopy = NULL;

    if (pCreds)
    {
        Status = LwIoAllocateMemory(sizeof(*pCredsCopy), OUT_PPVOID(&pCredsCopy));
        BAIL_ON_NT_STATUS(Status);
        
        pCredsCopy->type = pCreds->type;
        
        switch (pCreds->type)
        {
        case IO_CREDS_TYPE_PLAIN:
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.plain.pwszUsername,
                pCreds->payload.plain.pwszUsername);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.plain.pwszDomain,
                pCreds->payload.plain.pwszDomain);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.plain.pwszPassword,
                pCreds->payload.plain.pwszPassword);
            BAIL_ON_NT_STATUS(Status);
            break;
        case IO_CREDS_TYPE_KRB5_CCACHE:
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.krb5Ccache.pwszPrincipal,
                pCreds->payload.krb5Ccache.pwszPrincipal);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.krb5Ccache.pwszCachePath,
                pCreds->payload.krb5Ccache.pwszCachePath);
            BAIL_ON_NT_STATUS(Status);
            break;
        case IO_CREDS_TYPE_KRB5_TGT:
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.krb5Tgt.pwszClientPrincipal,
                pCreds->payload.krb5Tgt.pwszClientPrincipal);
            BAIL_ON_NT_STATUS(Status);
            Status = RtlWC16StringDuplicate(
                &pCredsCopy->payload.krb5Tgt.pwszServerPrincipal,
                pCreds->payload.krb5Tgt.pwszServerPrincipal);
            BAIL_ON_NT_STATUS(Status);
            pCredsCopy->payload.krb5Tgt.authTime = pCreds->payload.krb5Tgt.authTime;
            pCredsCopy->payload.krb5Tgt.startTime = pCreds->payload.krb5Tgt.startTime;
            pCredsCopy->payload.krb5Tgt.endTime = pCreds->payload.krb5Tgt.endTime;
            pCredsCopy->payload.krb5Tgt.renewTillTime = pCreds->payload.krb5Tgt.renewTillTime;
            pCredsCopy->payload.krb5Tgt.keyType = pCreds->payload.krb5Tgt.keyType;
            pCredsCopy->payload.krb5Tgt.ulKeySize = pCreds->payload.krb5Tgt.ulKeySize;
            Status = LwIoAllocateMemory(
                pCreds->payload.krb5Tgt.ulKeySize,
                OUT_PPVOID(&pCredsCopy->payload.krb5Tgt.pKeyData));
            BAIL_ON_NT_STATUS(Status);
            memcpy(
                pCredsCopy->payload.krb5Tgt.pKeyData,
                pCreds->payload.krb5Tgt.pKeyData,
                pCreds->payload.krb5Tgt.ulKeySize);
            pCredsCopy->payload.krb5Tgt.tgtFlags = pCreds->payload.krb5Tgt.tgtFlags;
            pCredsCopy->payload.krb5Tgt.ulTgtSize = pCreds->payload.krb5Tgt.ulTgtSize;
            Status = LwIoAllocateMemory(
                pCreds->payload.krb5Tgt.ulTgtSize,
                OUT_PPVOID(&pCredsCopy->payload.krb5Tgt.pTgtData));
            BAIL_ON_NT_STATUS(Status);
            memcpy(
                pCredsCopy->payload.krb5Tgt.pTgtData,
                pCreds->payload.krb5Tgt.pTgtData,
                pCreds->payload.krb5Tgt.ulTgtSize);
            break;
        }
        
        *ppCredsCopy = pCredsCopy;
    }
    else
    {
        *ppCredsCopy = NULL;
    }
    
cleanup:

    return Status;

error:

    if (pCredsCopy)
    {
        LwIoDeleteCreds(pCredsCopy);
    }

    goto cleanup;
}

VOID
LwIoDeleteCreds(
    PIO_CREDS pCreds
    )
{
    if (pCreds)
    {
        switch (pCreds->type)
        {
        case IO_CREDS_TYPE_PLAIN:
            IO_SAFE_FREE_MEMORY(pCreds->payload.plain.pwszUsername);
            IO_SAFE_FREE_MEMORY(pCreds->payload.plain.pwszDomain);
            IO_SAFE_FREE_MEMORY(pCreds->payload.plain.pwszPassword);
            break;
        case IO_CREDS_TYPE_KRB5_CCACHE:
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Ccache.pwszPrincipal);
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Ccache.pwszCachePath);
            break;
        case IO_CREDS_TYPE_KRB5_TGT:
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Tgt.pwszClientPrincipal);
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Tgt.pwszServerPrincipal);
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Tgt.pKeyData);
            IO_SAFE_FREE_MEMORY(pCreds->payload.krb5Tgt.pTgtData);
            break;
        }

        LwIoFreeMemory(pCreds);
    }
}

BOOLEAN
LwIoCompareCreds(
    PIO_CREDS pCredsOne,
    PIO_CREDS pCredsTwo
    )
{
    if (pCredsOne == NULL && pCredsTwo == NULL)
    {
        return TRUE;
    }
    else if (pCredsOne != NULL && pCredsTwo != NULL &&
             pCredsOne->type == pCredsTwo->type)
    {
        switch (pCredsOne->type)
        {
        case IO_CREDS_TYPE_PLAIN:
            return (!SMBWc16sCmp(pCredsOne->payload.plain.pwszUsername,
                                 pCredsTwo->payload.plain.pwszUsername) &&
                    !SMBWc16sCmp(pCredsOne->payload.plain.pwszPassword,
                                 pCredsTwo->payload.plain.pwszPassword));
        case IO_CREDS_TYPE_KRB5_CCACHE:
            return (!SMBWc16sCmp(pCredsOne->payload.krb5Ccache.pwszPrincipal,
                                 pCredsTwo->payload.krb5Ccache.pwszPrincipal) &&
                    !SMBWc16sCmp(pCredsOne->payload.krb5Ccache.pwszCachePath,
                                 pCredsTwo->payload.krb5Ccache.pwszCachePath));
        case IO_CREDS_TYPE_KRB5_TGT:
            return (!SMBWc16sCmp(pCredsOne->payload.krb5Tgt.pwszClientPrincipal,
                                 pCredsTwo->payload.krb5Tgt.pwszClientPrincipal) &&
                    !SMBWc16sCmp(pCredsOne->payload.krb5Tgt.pwszServerPrincipal,
                                 pCredsTwo->payload.krb5Tgt.pwszServerPrincipal) &&
                    (pCredsOne->payload.krb5Tgt.ulTgtSize ==
                     pCredsTwo->payload.krb5Tgt.ulTgtSize) &&
                    memcpy(pCredsOne->payload.krb5Tgt.pTgtData,
                           pCredsTwo->payload.krb5Tgt.pTgtData,
                           pCredsOne->payload.krb5Tgt.ulTgtSize) == 0);
        }
    }

    return FALSE;
}

NTSTATUS
LwIoResolveCreds(
    PIO_CREDS pBaseToken,
    PIO_CREDS* ppResolvedToken
    )
{
    if (pBaseToken)
    {
        switch (pBaseToken->type)
        {
        case IO_CREDS_TYPE_KRB5_TGT:
        case IO_CREDS_TYPE_PLAIN:
            return LwIoCopyCreds(pBaseToken, ppResolvedToken);
        case IO_CREDS_TYPE_KRB5_CCACHE:
            return LwIoCredentialCacheToTgt(pBaseToken, ppResolvedToken);
        }
    }

    *ppResolvedToken = NULL;
    return STATUS_SUCCESS;
}

static
NTSTATUS
LwIoCredentialCacheToTgt(
    PIO_CREDS pCacheToken,
    PIO_CREDS* ppCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    krb5_context pContext = NULL;
    krb5_error_code krb5Error = 0;
    krb5_ccache pCache = NULL;
    PSTR pszClientPrincipalName = NULL;
    PSTR pszServerPrincipalName = NULL;
    PSTR pszDesiredPrincipal = NULL;
    PSTR pszCredCachePath = NULL;
    PIO_CREDS pCreds = NULL;
    BOOLEAN bFoundTgt = FALSE;
    BOOLEAN bStartSeq = FALSE;
    krb5_creds creds;
    krb5_cc_cursor cursor;

    Status = LwRtlCStringAllocateFromWC16String(
        &pszDesiredPrincipal,
        pCacheToken->payload.krb5Ccache.pwszPrincipal);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlCStringAllocateFromWC16String(
        &pszCredCachePath,
        pCacheToken->payload.krb5Ccache.pwszCachePath);
    BAIL_ON_NT_STATUS(Status);

    /* Open credentials cache */
    krb5Error = krb5_init_context(&pContext);
    if (krb5Error)
    {
        if (krb5Error == KRB5_CONFIG_BADFORMAT)
        {
            /* If krb5.conf is corrupted, give up */
            goto cleanup;
        }

        Status = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(Status);
    }

    krb5Error = krb5_cc_resolve(pContext, pszCredCachePath, &pCache);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Look for a TGT */
    krb5Error = krb5_cc_start_seq_get(pContext, pCache, &cursor);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    bStartSeq = TRUE;

    while ((krb5Error = krb5_cc_next_cred(pContext, pCache, &cursor, &creds)) == 0)
    {
        /* Look tickets with the intial flag set */
        if (creds.ticket_flags & TKT_FLG_INITIAL)
        {
            /* Extract and compare client principal with desired principal */
            krb5Error = krb5_unparse_name(pContext, creds.client, &pszClientPrincipalName);
            if (krb5Error)
            {
                Status = STATUS_UNSUCCESSFUL;
                BAIL_ON_NT_STATUS(Status);
            }
            if (!strcmp(pszClientPrincipalName, pszDesiredPrincipal))
            {
                bFoundTgt = TRUE;
                break;
            }
            
            krb5_free_unparsed_name(pContext, pszClientPrincipalName);
            pszClientPrincipalName = NULL;
        }
        
        krb5_free_cred_contents(pContext, &creds);
    }
    
    if (!bFoundTgt)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Extract server principal name */
    krb5Error = krb5_unparse_name(pContext, creds.server, &pszServerPrincipalName);
    if (krb5Error)
    {
        Status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(Status);
    }

    /* Construct token from krb5 credential data */   
    Status = LwIoAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_NT_STATUS(Status);

    pCreds->type = IO_CREDS_TYPE_KRB5_TGT;

    /* Copy principal names */
    Status = LwRtlWC16StringAllocateFromCString(
        &pCreds->payload.krb5Tgt.pwszClientPrincipal,
        pszClientPrincipalName);
    BAIL_ON_NT_STATUS(Status);

    Status = LwRtlWC16StringAllocateFromCString(
        &pCreds->payload.krb5Tgt.pwszServerPrincipal,
        pszServerPrincipalName);
    BAIL_ON_NT_STATUS(Status);

    /* Set time fields */
    pCreds->payload.krb5Tgt.authTime = creds.times.authtime;
    pCreds->payload.krb5Tgt.startTime = creds.times.starttime;
    pCreds->payload.krb5Tgt.endTime = creds.times.endtime;
    pCreds->payload.krb5Tgt.renewTillTime = creds.times.renew_till;

    /* Copy encryption key */
    pCreds->payload.krb5Tgt.keyType = creds.keyblock.enctype;
    pCreds->payload.krb5Tgt.ulKeySize = creds.keyblock.length;
    Status = LwIoAllocateMemory(
        creds.keyblock.length,
        OUT_PPVOID(&pCreds->payload.krb5Tgt.pKeyData));
    BAIL_ON_NT_STATUS(Status);
    memcpy(
        pCreds->payload.krb5Tgt.pKeyData,
        creds.keyblock.contents,
        creds.keyblock.length);

    /* Copy tgt */
    pCreds->payload.krb5Tgt.tgtFlags = creds.ticket_flags;
    pCreds->payload.krb5Tgt.ulTgtSize = creds.ticket.length;
    Status = LwIoAllocateMemory(
        creds.ticket.length,
        OUT_PPVOID(&pCreds->payload.krb5Tgt.pTgtData));
    BAIL_ON_NT_STATUS(Status);
    memcpy(
        pCreds->payload.krb5Tgt.pTgtData,
        creds.ticket.data,
        creds.ticket.length);

    *ppCreds = pCreds;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pszDesiredPrincipal);
    LWIO_SAFE_FREE_MEMORY(pszCredCachePath);

    if (pszClientPrincipalName)
    {
        krb5_free_unparsed_name(pContext, pszClientPrincipalName);
    }

    if (pszServerPrincipalName)
    {
        krb5_free_unparsed_name(pContext, pszServerPrincipalName);
    }

    if (bFoundTgt)
    {
        krb5_free_cred_contents(pContext, &creds);
    }

    if (bStartSeq)
    {
        krb5_cc_end_seq_get(pContext, pCache, &cursor);
    }

    if (pCache)
    {
        krb5_cc_close(pContext, pCache);
    }

    if (pContext)
    {
        krb5_free_context(pContext);
    }

    return Status;

error:

    *ppCreds = NULL;

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}
