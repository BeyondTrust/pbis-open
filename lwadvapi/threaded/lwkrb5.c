/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwkrb5.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) 
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "includes.h"
#include <gssapi/gssapi_krb5.h>
#include <lw/swab.h>

#define LW_GSS_LOG_CALL_FORMAT \
    "GSS API error calling %s(): majorStatus = 0x%08x, minorStatus = 0x%08x"

#define _LW_GSS_LOG_CALL_ERROR(GssFunctionName, MajorStatus, MinorStatus) \
    do { \
        PSTR pszLocalErrorMessage = NULL; \
        DWORD dwLocalError = LwGssGetErrorMessage(&pszLocalErrorMessage, GssFunctionName, MajorStatus, MinorStatus); \
        if (dwLocalError) \
        { \
            LW_RTL_LOG_ERROR(LW_GSS_LOG_CALL_FORMAT, GssFunctionName, MajorStatus, MinorStatus); \
        } \
        else \
        { \
            LW_RTL_LOG_ERROR("%s", pszLocalErrorMessage); \
            LwFreeString(pszLocalErrorMessage); \
        } \
    } while (0)

#define _LW_GSS_LOG_CALL_DEBUG(GssFunctionName, MajorStatus, MinorStatus) \
    do { \
        PSTR pszLocalErrorMessage = NULL; \
        DWORD dwLocalError = LwGssGetErrorMessage(&pszLocalErrorMessage, GssFunctionName, MajorStatus, MinorStatus); \
        if (dwLocalError) \
        { \
            LW_RTL_LOG_DEBUG(LW_GSS_LOG_CALL_FORMAT, GssFunctionName, MajorStatus, MinorStatus); \
        } \
        else \
        { \
            LW_RTL_LOG_DEBUG("%s", pszLocalErrorMessage); \
            LwFreeString(pszLocalErrorMessage); \
        } \
    } while (0)


#define LW_GSS_LOG_IF_NOT_COMPLETE(GssFunctionName, MajorStatus, MinorStatus) \
    do { \
        switch (MajorStatus) \
        { \
            case GSS_S_COMPLETE: \
                break; \
            default: \
                _LW_GSS_LOG_CALL_ERROR(GssFunctionName, MajorStatus, MinorStatus); \
        } \
    } while (0)

#define LW_GSS_LOG_IF_NOT_COMPLETE_OR_CONTINUE(GssFunctionName, MajorStatus, MinorStatus) \
    do { \
        switch (MajorStatus) \
        { \
            case GSS_S_COMPLETE: \
                break; \
            case GSS_S_CONTINUE_NEEDED: \
                _LW_GSS_LOG_CALL_DEBUG(GssFunctionName, MajorStatus, MinorStatus); \
                break; \
            default: \
                _LW_GSS_LOG_CALL_ERROR(GssFunctionName, MajorStatus, MinorStatus); \
        } \
    } while (0)

#define BAIL_ON_GSS_ERROR(dwError, MajorStatus, MinorStatus) \
    do { \
        if (((MajorStatus) != GSS_S_COMPLETE) && \
            ((MajorStatus) != GSS_S_CONTINUE_NEEDED)) \
        { \
            LW_RTL_LOG_DEBUG("[%s() %s:%d] GSS API error: majorStatus = 0x%08x, minorStatus = 0x%08x", \
                         __FUNCTION__, \
                         __FILE__, \
                         __LINE__, \
                         MajorStatus, MinorStatus); \
            dwError = LW_ERROR_GSS_CALL_FAILED; \
            goto error; \
        } \
    } while (0)

//
// KG_EMPTY_CCACHE is defined inside of gssapi but it is not exposed
// externally.  It means that the credentials cache does not have a
// TGT inside of it.
//
        
#define KG_EMPTY_CCACHE 0x25ea10c
        

static volatile LONG glLibraryRefCount = 0;

static
DWORD
LwGssGetSingleErrorMessage(
    OUT PSTR* ppszErrorMessage,
    IN OM_uint32 Status,
    IN BOOLEAN IsMajor
    )
{
    DWORD dwError = 0;
    PSTR pszErrorMessage = NULL;
    OM_uint32 majorStatus = 0;
    OM_uint32 minorStatus = 0;
    gss_buffer_desc message = GSS_C_EMPTY_BUFFER;
    OM_uint32 messageContext = 0;
    int statusType = IsMajor ? GSS_C_GSS_CODE : GSS_C_MECH_CODE;

    do {
        majorStatus = gss_display_status(
                            &minorStatus,
                            Status,
                            statusType,
                            GSS_C_NULL_OID,
                            &messageContext,
                            &message);
        if (majorStatus != GSS_S_COMPLETE)
        {
            LW_RTL_LOG_ERROR("Call to gss_display_status() failed with "
                         "majorStatus = 0x%08x, minorStatus = 0x%08x",
                         majorStatus, minorStatus);

            dwError = ERROR_INTERNAL_ERROR;
            BAIL_ON_LW_ERROR(dwError);
        }

        if (!pszErrorMessage)
        {
            dwError = LwAllocateString((PSTR)message.value, &pszErrorMessage);
            BAIL_ON_LW_ERROR(dwError);
        }
        else
        {
            PSTR pszNewErrorMessage = NULL;

            dwError = LwAllocateStringPrintf(&pszNewErrorMessage,
                                             "%s; %s",
                                             pszErrorMessage,
                                             (PSTR)message.value);
            BAIL_ON_LW_ERROR(dwError);

            LW_SAFE_FREE_STRING(pszErrorMessage);
            pszErrorMessage = pszNewErrorMessage;
        }

        majorStatus = gss_release_buffer(&minorStatus, &message);
    } while (messageContext);

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszErrorMessage);
    }

    if (message.value)
    {
        majorStatus = gss_release_buffer(&minorStatus, &message);
    }

    *ppszErrorMessage = pszErrorMessage;

    return dwError;
}

DWORD
LwGssGetErrorMessage(
    OUT PSTR* ppszErrorMessage,
    IN OPTIONAL PCSTR pszGssFunction,
    IN OM_uint32 MajorStatus,
    IN OM_uint32 MinorStatus
    )
{
    DWORD dwError = 0;
    PSTR pszErrorMessage = NULL;
    PSTR pszMajorMessage = NULL;
    PSTR pszMinorMessage = NULL;

    dwError = LwGssGetSingleErrorMessage(&pszMajorMessage, MajorStatus, TRUE);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwGssGetSingleErrorMessage(&pszMinorMessage, MinorStatus, FALSE);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszErrorMessage,
                                     "GSS API error calling %s(): "
                                     "majorStatus = 0x%08x (%s), "
                                     "minorStatus = 0x%08x (%s)",
                                     pszGssFunction,
                                     MajorStatus, pszMajorMessage,
                                     MinorStatus, pszMinorMessage);
    BAIL_ON_LW_ERROR(dwError);

error:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszErrorMessage);
    }

    LW_SAFE_FREE_STRING(pszMajorMessage);
    LW_SAFE_FREE_STRING(pszMinorMessage);

    *ppszErrorMessage = pszErrorMessage;

    return dwError;
}


DWORD
LwKrb5GetDefaultRealm(
    PSTR* ppszRealm
    )
{
    DWORD dwError = 0;
    krb5_context ctx = NULL;
    PSTR pszKrb5Realm = NULL;
    PSTR pszRealm = NULL;

    krb5_init_context(&ctx);
    krb5_get_default_realm(ctx, &pszKrb5Realm);

    if (LW_IS_NULL_OR_EMPTY_STR(pszKrb5Realm)) {
        dwError = LW_ERROR_NO_DEFAULT_REALM;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateString(pszKrb5Realm, &pszRealm);
    BAIL_ON_LW_ERROR(dwError);

    *ppszRealm = pszRealm;
    
cleanup:

    if (pszKrb5Realm)
    {
        krb5_free_default_realm(ctx, pszKrb5Realm);
    }

    krb5_free_context(ctx);

    return(dwError);

error:

    *ppszRealm = NULL;
    
    LW_SAFE_FREE_STRING(pszRealm);

    goto cleanup;
}


DWORD
LwKrb5GetDefaultCachePath(
    OUT PSTR* ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    krb5_context ctx = NULL;
    const char *pszKrbDefault = NULL;
    krb5_error_code ret = 0;
    
    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszKrbDefault = krb5_cc_default_name(ctx);

    dwError = LwAllocateString(
                pszKrbDefault,
                &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    *ppszCachePath = pszCachePath;
    
cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
LwKrb5GetSupportedEncryptionTypes(
    OUT PDWORD pdwSupportedEncryptionTypes
    )
{
    DWORD dwError = 0;
    DWORD dwEncTypes = 0;
    krb5_context ctx = NULL;
    krb5_enctype *list = NULL;
	krb5_enctype *ptr;
    krb5_error_code ret = 0;
    
    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_permitted_enctypes(ctx, &list);
    BAIL_ON_KRB_ERROR(ctx, ret);

    for (ptr = list; *ptr; ptr++)
    {
        switch (*ptr) 
        {
            case ENCTYPE_DES_CBC_CRC:
                dwEncTypes |= LW_MSDS_DES_CBC_CRC;
                break;
            case ENCTYPE_DES_CBC_MD5:
                dwEncTypes |= LW_MSDS_DES_CBC_MD5;
                break;
            case ENCTYPE_ARCFOUR_HMAC:
                dwEncTypes |= LW_MSDS_RC4_HMAC;
                break;
            case ENCTYPE_AES128_CTS_HMAC_SHA1_96:
                dwEncTypes |= LW_MSDS_AES128_CTS;
                break;
            case ENCTYPE_AES256_CTS_HMAC_SHA1_96:
                dwEncTypes |= LW_MSDS_AES256_CTS;
                break;
        }
    }

    *pdwSupportedEncryptionTypes = dwEncTypes;
    
cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }

    if (list)
    {
        free(list);
	}

    return dwError;
    
error:

    *pdwSupportedEncryptionTypes = 0;
    
    goto cleanup;
}


DWORD
LwKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    
    switch (cacheType)
    {
        case KRB5_InMemory_Cache:
            
            dwError = LwAllocateStringPrintf(
                        &pszCachePath,
                        "MEMORY:krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LW_ERROR(dwError);
            
            break;
            
        case KRB5_File_Cache:
            
            dwError = LwAllocateStringPrintf(
                        &pszCachePath,
                        "FILE:/tmp/krb5cc_%ld",
                        (long)uid);
            BAIL_ON_LW_ERROR(dwError);
            
            break;
            
        default:
            
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
            
            break;
    }
    
    *ppszCachePath = pszCachePath;
    
cleanup:

    return dwError;
    
error:

    *ppszCachePath = NULL;
    
    goto cleanup;
}


DWORD
LwKrb5SetThreadDefaultCachePath(
    IN PCSTR pszCachePath,
    OUT PSTR* ppszPreviousCachePath
    )
{
    DWORD dwError       = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    // Set the default for gss
    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            (ppszPreviousCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_GSS_ERROR(dwError, dwMajorStatus, dwMinorStatus);

    LW_RTL_LOG_DEBUG("Switched gss krb5 credentials path from %s to %s",
            LW_SAFE_LOG_STRING(pszOrigCachePath),
            LW_SAFE_LOG_STRING(pszCachePath));
    
    if (ppszPreviousCachePath) {
        if (!LW_IS_NULL_OR_EMPTY_STR(pszOrigCachePath)) {
            dwError = LwAllocateString(pszOrigCachePath, ppszPreviousCachePath);
            BAIL_ON_LW_ERROR(dwError);
        } else {
            *ppszPreviousCachePath = NULL;
        }
    }
    
cleanup:

    return dwError;
    
error:

    if (ppszPreviousCachePath) {
        *ppszPreviousCachePath = NULL;
    }

    goto cleanup;
}

DWORD
LwKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    )
{
    DWORD dwError = 0;
    PSTR pszEnvironmentEntry = NULL;
    static volatile PSTR pszSavedEnvironmentEntry = NULL;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    BOOLEAN bLocked = FALSE;

    dwError = pthread_mutex_lock(&lock);
    if (dwError)
    {
        dwError = LwMapErrnoToLwError(dwError);
        BAIL_ON_LW_ERROR(dwError);
    }
    bLocked = TRUE;

    dwError = LwAllocateStringPrintf(&pszEnvironmentEntry,
                                      "KRB5CCNAME=%s",
                                      pszCachePath);
    BAIL_ON_LW_ERROR(dwError);

    /*
     * putenv requires that the buffer not be free'd.
     */
    if (putenv(pszEnvironmentEntry) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    LW_SAFE_FREE_STRING(pszSavedEnvironmentEntry);
    pszSavedEnvironmentEntry = pszEnvironmentEntry;
    pszEnvironmentEntry = NULL;

error:
    LW_SAFE_FREE_STRING(pszEnvironmentEntry);
    
    if (bLocked)
    {
        pthread_mutex_unlock(&lock);
    }
    
    return dwError;
}

DWORD
LwKrb5InitializeCredentials(
    IN PCSTR pszUserPrincipalName,
    IN PCSTR pszPassword,
    IN PCSTR pszCachePath,
    OUT OPTIONAL PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwGoodUntilTime = 0;
    PSTR pszTempCachePath = NULL;

    if (!pszCachePath)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (!strncmp(pszCachePath, "FILE:", sizeof("FILE:") - 1))
    {
        dwError = LwAllocateStringPrintf(&pszTempCachePath, "%s.new",
                                         pszCachePath);
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwKrb5GetTgt(
                    pszUserPrincipalName,
                    pszPassword,
                    pszTempCachePath ? pszTempCachePath : pszCachePath,
                    &dwGoodUntilTime);
    BAIL_ON_LW_ERROR(dwError);

    if (pszTempCachePath)
    {
        dwError = LwMoveFile(pszTempCachePath + sizeof("FILE:") - 1,
                             pszCachePath + sizeof("FILE:") - 1);
        BAIL_ON_LW_ERROR(dwError);
    }

error:
    if (dwError)
    {
        dwGoodUntilTime = 0;
    }

    LW_SAFE_FREE_STRING(pszTempCachePath);

    if (pdwGoodUntilTime)
    {
        *pdwGoodUntilTime = dwGoodUntilTime;
    }

    return dwError;
}

DWORD
LwKrb5CopyFromUserCache(
                krb5_context ctx,
                krb5_ccache destCC,
                uid_t uid
                )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszCachePath = NULL;
    krb5_ccache srcCC = NULL;
    krb5_cc_cursor srcPos = NULL;
    krb5_cc_cursor destPos = NULL;
    // Free with krb5_free_cred_contents
    krb5_creds srcCreds = {0};
    // Free with krb5_free_cred_contents
    krb5_creds destCreds = {0};
    krb5_error_code ret = 0;
    krb5_principal destClient = 0;
    BOOLEAN bIncludeTicket = TRUE;
    DWORD dwTime = 0;

    ret = krb5_cc_get_principal(
            ctx,
            destCC,
            &destClient);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    ret = krb5_cc_resolve(
            ctx,
            pszCachePath,
            &srcCC);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_start_seq_get(
            ctx,
            srcCC,
            &srcPos);
    if (ret == KRB5_FCC_NOFILE)
    {
        // The cache file does not exist
        ret = 0;
        goto cleanup;
    }
    if (ret == KRB5_CC_FORMAT)
    {
        // Some other user put a bad cc in place - don't copy anything
        // from it.
        ret = 0;
        goto cleanup;
    }
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwTime = time(NULL);

    while (1)
    {
        krb5_free_cred_contents(
                ctx,
                &srcCreds);

        ret = krb5_cc_next_cred(
                ctx,
                srcCC,
                &srcPos,
                &srcCreds);
        if (ret == KRB5_CC_FORMAT) {
            break;
        } else if (ret == KRB5_CC_END) {
            break;
        } else {
            BAIL_ON_KRB_ERROR(ctx, ret);
        }

        if (!krb5_principal_compare(ctx, destClient, srcCreds.client))
        {
            /* Can't keep these creds. The client principal doesn't
             * match. */
            continue;
        }

        if ( srcCreds.times.endtime < dwTime )
        {
            /* Credentials are too old. */
            continue;
        }

        if (destPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    destCC,
                    &destPos);
            destPos = NULL;
        }

        ret = krb5_cc_start_seq_get(
                ctx,
                destCC,
                &destPos);
        BAIL_ON_KRB_ERROR(ctx, ret);

        bIncludeTicket = TRUE;

        while(bIncludeTicket)
        {
            krb5_free_cred_contents(
                    ctx,
                    &destCreds);

            ret = krb5_cc_next_cred(
                    ctx,
                    destCC,
                    &destPos,
                    &destCreds);
            if (ret == KRB5_CC_END) {
                break;
            } else {
                BAIL_ON_KRB_ERROR(ctx, ret);
            }

            if (krb5_principal_compare(
                        ctx,
                        destCreds.server,
                        srcCreds.server))
            {
                /* These credentials are already in the dest cache
                 */
                bIncludeTicket = FALSE;
            }
        }

        if (bIncludeTicket)
        {
            // These creds can go in the new cache
            ret = krb5_cc_store_cred(ctx, destCC, &srcCreds);
            BAIL_ON_KRB_ERROR(ctx, ret);
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);

    if (ctx != NULL)
    {
        if (srcPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    srcCC,
                    &srcPos);
        }
        if (destPos != NULL)
        {
            krb5_cc_end_seq_get(
                    ctx,
                    destCC,
                    &destPos);
        }
        if (srcCC != NULL)
        {
            krb5_cc_close(ctx, srcCC);
        }
        krb5_free_cred_contents(ctx, &srcCreds);
        krb5_free_cred_contents(ctx, &destCreds);
        if (destClient != NULL)
        {
            krb5_free_principal(ctx, destClient);
        }
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LwKrb5MoveCCacheToUserPath(
    krb5_context ctx,
    PCSTR pszNewCacheName,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszCachePath = NULL;
    PCSTR  pszCachePathReal = NULL;

    dwError = LwKrb5GetUserCachePath(
                    uid,
                    KRB5_File_Cache,
                    &pszCachePath);
    BAIL_ON_LW_ERROR(dwError);
    
    if (strncasecmp(pszCachePath, "FILE:", sizeof("FILE:")-1)) {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    } else {
        pszCachePathReal = pszCachePath + sizeof("FILE:") - 1;
    }

    dwError = LwMoveFile(pszNewCacheName,
                pszCachePathReal);
    BAIL_ON_LW_ERROR(dwError);

    /* Let the user read and write to their cache file (before this, only
     * root was allowed to read and write the file).
     */
    dwError = LwChangeOwner(pszCachePathReal, uid, gid);
    BAIL_ON_LW_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszCachePath);
    return dwError;

error:
    goto cleanup;
}

static
__attribute__((constructor))
VOID
LwKrb5Constructor(
    VOID
    )
{
    LwInterlockedIncrement(&glLibraryRefCount);
}

static
void
__attribute__((destructor))
LwKrb5Shutdown(
    VOID
    )
{
    if (!LwInterlockedDecrement(&glLibraryRefCount))
    {
        pthread_mutex_destroy(&gLwKrb5State.ExistingClientLock);
        pthread_mutex_destroy(&gLwKrb5State.UserCacheMutex);
    }
}

DWORD
LwTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFunction,
    PCSTR pszFile,
    DWORD dwLine
    )
{
    PCSTR pszKrb5Error = NULL;

    if (ctx)
    {
        pszKrb5Error = krb5_get_error_message(ctx, krbError);
    }
    if (pszKrb5Error)
    {
        LW_RTL_LOG_WARNING("[%s %s:%d] KRB5 Error code: %d (Message: %s)",
                pszFunction,
                pszFile,
                dwLine,
                krbError,
                pszKrb5Error);
    }
    else
    {
        LW_RTL_LOG_WARNING("[%s %s:%d] KRB5 Error code: %d",
                pszFunction,
                pszFile,
                dwLine,
                krbError);
    }

    if (pszKrb5Error)
    {
        krb5_free_error_message(ctx, pszKrb5Error);
    }

    return LwMapKrb5ErrorToLwError(krbError);
}

DWORD
LwMapKrb5ErrorToLwError(
    krb5_error_code krbError
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    unsigned int i = 0;

    switch (krbError)
    {
        case ENOENT:
            dwError = LW_ERROR_KRB5_NO_KEYS_FOUND;
            break;
        default:
            for (i = 0; krb5err_lwerr_map[i].pszKrb5errStr; i++)
            {
                if (krb5err_lwerr_map[i].krb5err == krbError)
                {
                    dwError = krb5err_lwerr_map[i].lwerr;
                    break;
                }
            }

            if (!dwError)
            {
                dwError = LW_ERROR_KRB5_CALL_FAILED;
            }
            break;
    }

    return dwError;
}

DWORD
LwKrb5VerifyPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const struct berval *pPacBerVal,
    const krb5_keyblock *serviceKey,
    char** ppchLogonInfo,
    size_t* psLogonInfo
    )
{
    krb5_error_code ret = 0;
    PAC_DATA *pPacData = NULL;
    DWORD i;
    char *pchPacCopy = NULL;
    //Do not free
    krb5_data krbPacData = {0};
    //Do not free
    krb5_checksum checksum = {0};
    //Do not free
    PAC_SIGNATURE_DATA *pServerSig = NULL;
    PAC_LOGON_NAME *pLogonName = NULL;
    size_t sServerSig = 0;
    //Do not free
    char *pchLogonInfoStart = NULL;
    size_t sLogonInfoLen = 0;
    krb5_boolean bHasGoodChecksum = FALSE;
    uint64_t qwNtAuthTime;
    DWORD dwError = LW_ERROR_SUCCESS;
    //Free with krb5_free_unparsed_name
    PSTR pszClientPrincipal = NULL;
    PSTR pszLogonName = NULL;
    char* pchLogonInfo = NULL;

    WORD * pwNameLocal = NULL;
    DWORD dwCount = 0;

    dwError = LwAllocateMemory(
                pPacBerVal->bv_len,
                OUT_PPVOID(&pPacData));
    BAIL_ON_LW_ERROR(dwError);

    memcpy(pPacData, pPacBerVal->bv_val, pPacBerVal->bv_len);

    pPacData->dwBufferCount = LW_LTOH32(pPacData->dwBufferCount);
    pPacData->dwVersion = LW_LTOH32(pPacData->dwVersion);

    // We only know about version 0
    if (pPacData->dwVersion != 0)
    {
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }
    // Make sure that the last buffer in the pac data doesn't go out of bounds
    // of the parent buffer
    if ((void *)&pPacData->buffers[pPacData->dwBufferCount] -
            (void *)pPacData > pPacBerVal->bv_len)
    {
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    // Make sure the data associated with each buffer doesn't go out of
    // bounds
    for (i = 0; i < pPacData->dwBufferCount; i++)
    {
        pPacData->buffers[i].dwType = LW_LTOH32(pPacData->buffers[i].dwType);
        pPacData->buffers[i].dwSize = LW_LTOH32(pPacData->buffers[i].dwSize);
        pPacData->buffers[i].qwOffset = LW_LTOH64(pPacData->buffers[i].qwOffset);

        if (pPacData->buffers[i].qwOffset + pPacData->buffers[i].dwSize <
                pPacData->buffers[i].qwOffset)
        {
            dwError = LW_ERROR_INVALID_MESSAGE;
            BAIL_ON_LW_ERROR(dwError);
        }
        if (pPacData->buffers[i].qwOffset + pPacData->buffers[i].dwSize >
                pPacBerVal->bv_len)
        {
            dwError = LW_ERROR_INVALID_MESSAGE;
            BAIL_ON_LW_ERROR(dwError);
        }
    }

    dwError = LwAllocateMemory(
                pPacBerVal->bv_len,
                OUT_PPVOID(&pchPacCopy));
    BAIL_ON_LW_ERROR(dwError);

    memcpy(pchPacCopy, pPacBerVal->bv_val, pPacBerVal->bv_len);

    krbPacData.magic = KV5M_DATA;
    krbPacData.length = pPacBerVal->bv_len;
    krbPacData.data = pchPacCopy;

    for (i = 0; i < pPacData->dwBufferCount; i++)
    {
    	switch (pPacData->buffers[i].dwType)
    	{
    	    case PAC_TYPE_LOGON_INFO:
    	        pchLogonInfoStart = (char *)pPacData + pPacData->buffers[i].qwOffset;
                sLogonInfoLen = pPacData->buffers[i].dwSize;
                break;
            case PAC_TYPE_SRV_CHECKSUM:
                pServerSig = (PAC_SIGNATURE_DATA *)((char *)pPacData +
                             pPacData->buffers[i].qwOffset);

                pServerSig->dwType = LW_LTOH32(pServerSig->dwType);

                sServerSig = pPacData->buffers[i].dwSize -
                        (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature;
                /* The checksum is calculated with the signatures zeroed out. */
                memset(pchPacCopy + pPacData->buffers[i].qwOffset +
                       (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature,
                       0,
                       pPacData->buffers[i].dwSize -
                           (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature);
                break;
            case PAC_TYPE_KDC_CHECKSUM:
                /* The checksum is calculated with the signatures zeroed out. */
    		memset(pchPacCopy + pPacData->buffers[i].qwOffset +
    	               (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature,
    		       0,
    		       pPacData->buffers[i].dwSize -
    		           (size_t)&((PAC_SIGNATURE_DATA *)0)->pchSignature);
    		break;
            case PAC_TYPE_LOGON_NAME:
                pLogonName = (PAC_LOGON_NAME *)((char *)pPacData +
                             pPacData->buffers[i].qwOffset);

                pLogonName->ticketTime = LW_LTOH64(pLogonName->ticketTime);
                pLogonName->wAccountNameLen = LW_LTOH16(pLogonName->wAccountNameLen);
                pwNameLocal = pLogonName->pwszName;

                for ( dwCount = 0 ;
                      dwCount < pLogonName->wAccountNameLen / 2 ;
                      dwCount++ )
                {
                    pwNameLocal[dwCount] = LW_LTOH16(pwNameLocal[dwCount]);
                }

                if ((char *)&pLogonName->pwszName +
                    pLogonName->wAccountNameLen >
                    (char *)pPacData + pPacData->buffers[i].qwOffset +
                    pPacData->buffers[i].dwSize)
                {
                    // The message is invalid because the terminating null
                    // of the name lands outside of the buffer.
                    dwError = LW_ERROR_INVALID_MESSAGE;
                    BAIL_ON_LW_ERROR(dwError);
                }
                break;
            default:
                break;
    	}
    }

    if (pServerSig == NULL)
    {
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pLogonName == NULL)
    {
        //We need the logon name to verify the pac is for the right user
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (pchLogonInfoStart == NULL)
    {
        /* The buffer we really care about isn't in the pac. */
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    checksum.magic = KV5M_CHECKSUM;
    checksum.checksum_type = pServerSig->dwType;
    checksum.length = sServerSig;
    checksum.contents = (unsigned char *)pServerSig->pchSignature;

    ret = krb5_c_verify_checksum(
                    ctx,
                    serviceKey,
                    KRB5_KEYUSAGE_APP_DATA_CKSUM,
                    &krbPacData,
                    &checksum,
                    &bHasGoodChecksum);
    BAIL_ON_KRB_ERROR(ctx, ret);

    if (!bHasGoodChecksum)
    {
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    // Make sure the pac was issued with this ticket, not an old ticket
    qwNtAuthTime = pTgsTicket->enc_part2->times.authtime;
    qwNtAuthTime += 11644473600LL;
    qwNtAuthTime *= 1000*1000*10;
    if (pLogonName->ticketTime != qwNtAuthTime)
    {
        dwError = LW_ERROR_CLOCK_SKEW;
        BAIL_ON_LW_ERROR(dwError);
    }
    ret = krb5_unparse_name(
                    ctx,
                    pTgsTicket->enc_part2->client,
                    &pszClientPrincipal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    // Strip off the domain name
    if (strchr(pszClientPrincipal, '@') != NULL)
    {
        strchr(pszClientPrincipal, '@')[0] = '\0'; 
    }

    dwError = LwWc16snToMbs(
        pLogonName->pwszName,
        &pszLogonName,
        pLogonName->wAccountNameLen / 2);
    BAIL_ON_LW_ERROR(dwError);    

    if (strcasecmp(pszClientPrincipal, pszLogonName))
    {
        // The pac belongs to a different user
        dwError = LW_ERROR_INVALID_LOGIN_ID;
        BAIL_ON_LW_ERROR(dwError);    
    }

    dwError = LwAllocateMemory(
                sLogonInfoLen,
                OUT_PPVOID(&pchLogonInfo));
    BAIL_ON_LW_ERROR(dwError);

    memcpy(pchLogonInfo, pchLogonInfoStart, sLogonInfoLen);
    *ppchLogonInfo = pchLogonInfo;
    *psLogonInfo = sLogonInfoLen;

cleanup:
    LW_SAFE_FREE_STRING(pszLogonName);
    LW_SAFE_FREE_MEMORY(pPacData);
    LW_SAFE_FREE_MEMORY(pchPacCopy);
    if (pszClientPrincipal != NULL)
    {
        krb5_free_unparsed_name(ctx, pszClientPrincipal);
    }
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pchLogonInfo);
    *ppchLogonInfo = NULL;
    goto cleanup;
}

DWORD
LwKrb5FindPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const krb5_keyblock *serviceKey,
    OUT PVOID* ppchLogonInfo,
    OUT size_t* psLogonInfo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    //Do not free
    struct berval bv = {0};
    struct berval contents = {0};
    //Do not free
    krb5_authdata **ppCur = NULL;
    //Do not free associated buffer
    BerElement *ber = NULL;
    ber_tag_t tag = 0;
    ber_len_t len = 0;
    // Do not free
    char *cookie = NULL;
    int adType;
    ber_tag_t seqTag, context0Tag, context1Tag;
    char* pchLogonInfo = NULL;
    size_t sLogonInfo = 0;
    
    ber = ber_alloc_t(0);
    
    if (pTgsTicket && pTgsTicket->enc_part2)
    {
        ppCur = pTgsTicket->enc_part2->authorization_data;
    }

    while (ppCur && (*ppCur != NULL))
    {
        if (ppCur[0]->ad_type == AD_IF_RELEVANT_TYPE)
        {
            // This auth data contains a DER encoded sequence of more
            // auth data. One of them could be a pac.
            bv.bv_len = ppCur[0]->length;
            bv.bv_val = (char *)ppCur[0]->contents;
            ber_init2(ber, &bv, 0);

            tag = ber_first_element(ber, &len, &cookie);
            while (tag != LBER_ERROR)
            {
                // Free does nothing if pointer is NULL
                ber_memfree(contents.bv_val);
                contents.bv_val = NULL;

                tag = ber_scanf(ber,
                        "t{t[i]t[",
                        &seqTag,
                        &context0Tag,
                        &adType,
                        &context1Tag);
                if (tag == LBER_ERROR)
                {
                    // This auth data is invalid. Skip it and try
                    // the next one
                    break;
                }
                tag = ber_scanf(ber,
                        "o]}",
                        &contents);
                if (tag == LBER_ERROR)
                {
                    // This auth data is invalid. Skip it and try
                    // the next one
                    break;
                }

                if (adType == AD_WIN2K_PAC)
                {
                    dwError = LwKrb5VerifyPac(
                        ctx,
                        pTgsTicket,
                        &contents,
                        serviceKey,
                        &pchLogonInfo,
                        &sLogonInfo);
                    if (dwError == LW_ERROR_INVALID_MESSAGE)
                    {
                        dwError = LW_ERROR_SUCCESS;
                        continue;
                    }
                    BAIL_ON_LW_ERROR(dwError);
                    // Found a good PAC !
                    goto end_search;
                }

                //returns LBER_ERROR when there are no more elements left.
                tag = ber_next_element(ber, &len, cookie);
            }
        }

        ppCur++;
    }
end_search:

    *ppchLogonInfo = pchLogonInfo;
    *psLogonInfo = sLogonInfo;

cleanup:
    if (contents.bv_val != NULL)
    {
        ber_memfree(contents.bv_val);
    }
    if (ber != NULL)
    {
        ber_free(ber, 0);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pchLogonInfo);
    *ppchLogonInfo = NULL;
    goto cleanup;
}

DWORD
LwKrb5InitializeUserLoginCredentials(
    IN PCSTR pszUserPrincipalName,
    IN PCSTR pszPassword,
    IN uid_t uid,
    IN gid_t gid,
    IN LW_KRB5_LOGIN_FLAGS Flags,
    IN PCSTR pszServicePrincipal,
    IN PCSTR pszServiceRealm,
    IN PCSTR pszServicePassword,
    OUT PVOID* ppNdrPacInfo,
    OUT size_t* pNdrPacInfoSize,
    OUT PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    // Free with krb5_free_cred_contents
    krb5_creds credsRequest = {0};
    krb5_creds *pTgsCreds = NULL;
    krb5_ticket *pTgsTicket = NULL;
    krb5_ticket *pDecryptedTgs = NULL;
    krb5_auth_context authContext = NULL;
    krb5_data apReqPacket = {0};
    krb5_keyblock serviceKey = {0};
    krb5_data salt = {0};
    // Do not free
    krb5_data machinePassword = {0};
    krb5_flags flags = 0;
    krb5_int32 authcon_flags = 0;
    BOOLEAN bInLock = FALSE;
    PCSTR pszTempCacheName = NULL;
    PSTR pszTempCachePath = NULL;
    PVOID pNdrPacInfo = NULL;
    size_t ndrPacInfoSize = 0;
    DWORD dwGoodUntilTime = 0;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Generates a new filed based credentials cache in /tmp. The file will
     * be owned by root and only accessible by root.
     */
    ret = krb5_cc_new_unique(
            ctx, 
            "FILE",
            "hint",
            &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);


    if (Flags & LW_KRB5_LOGIN_FLAG_SMART_CARD)
    {
        dwError = LwKrb5GetTgtWithSmartCard(
                pszUserPrincipalName,
                pszPassword,
                krb5_cc_get_name(ctx, cc),
                &dwGoodUntilTime);
    }
    else
    {
        dwError = LwKrb5GetTgt(
                pszUserPrincipalName,
                pszPassword,
                krb5_cc_get_name(ctx, cc),
                &dwGoodUntilTime);
    }

    BAIL_ON_LW_ERROR(dwError);

    ret = krb5_parse_name(ctx, pszServicePrincipal, &credsRequest.server);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_get_principal(ctx, cc, &credsRequest.client);
    BAIL_ON_KRB_ERROR(ctx, ret);
 
    /* Get a TGS for our service using the tgt in the cache */
    ret = krb5_get_credentials(
            ctx,
            0, /*no options (not user to user encryption,
                 and not only cached) */
            cc,
            &credsRequest,
            &pTgsCreds);

    // Don't trust pTgsCreds on an unsuccessful return
    // This may be non-zero due to the krb5 libs following referrals
    // but has been freed in the krb5 libs themselves and any useful
    // tickets have already been cached.
    if (ret != 0) {
        pTgsCreds = NULL;
    }
    
    BAIL_ON_KRB_ERROR(ctx, ret);

    //No need to store the tgs in the cc. Kerberos does that automatically

    /* Generate an ap_req message, but don't send it anywhere. Just decode it
     * immediately. This is the only way to get kerberos to decrypt the tgs
     * using public APIs */
    ret = krb5_mk_req_extended(
            ctx,
            &authContext,
            0, /* no options necessary */
            NULL, /* since this isn't a real ap_req, we don't have any
                     supplemental data to send with it. */
            pTgsCreds,
            &apReqPacket);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Decode (but not decrypt) the tgs ticket so that we can figure out
     * which encryption type was used in it. */
    ret = krb5_decode_ticket(&pTgsCreds->ticket, &pTgsTicket);

    /* The TGS ticket is encrypted with the machine password and salted with
     * the service principal. pszServicePrincipal could probably be used
     * directly, but it's safer to unparse pTgsCreds->server, because the KDC
     * sent that to us.
     */
    ret = krb5_principal2salt(ctx, pTgsCreds->server, &salt);
    BAIL_ON_KRB_ERROR(ctx, ret);

    machinePassword.magic = KV5M_DATA;
    machinePassword.data = (PSTR)pszServicePassword,
    machinePassword.length = strlen(pszServicePassword),

    /* Generate a key to decrypt the TGS */
    ret = krb5_c_string_to_key(
            ctx,
            pTgsTicket->enc_part.enctype,
            &machinePassword,
            &salt,
            &serviceKey);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Typically krb5_rd_req would decode the AP_REQ using the keytab, but
     * we don't want to depend on the keytab. As a side effect of kerberos'
     * user to user authentication support, if a key is explictly set on the
     * auth context, that key will be used to decrypt the TGS instead of the
     * keytab.
     *
     * By manually generating the key and setting it, we don't require
     * a keytab.
     */
    if (authContext != NULL)
    {
        ret = krb5_auth_con_free(ctx, authContext);
        BAIL_ON_KRB_ERROR(ctx, ret);
    }
    
    ret = krb5_auth_con_init(ctx, &authContext);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_auth_con_setuseruserkey(
            ctx,
            authContext,
            &serviceKey);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Disable replay detection which is unnecessary and
     * can fail when authenticating large numbers of users.
     */
    krb5_auth_con_getflags(ctx,
                           authContext,
                           &authcon_flags);
    krb5_auth_con_setflags(ctx,
                           authContext,
                           authcon_flags & ~KRB5_AUTH_CONTEXT_DO_TIME);


    if (pszServiceRealm)
    {
        ret = krb5_set_default_realm(ctx, pszServiceRealm);
        BAIL_ON_KRB_ERROR(ctx, ret);
    }

    /* This decrypts the TGS. As a side effect it ensures that the KDC that
     * the user's TGT came from is in the same realm that the machine was
     * joined to (this prevents users from spoofing the KDC).
     */
    ret = krb5_rd_req(
            ctx,
            &authContext,
            &apReqPacket,
            pTgsCreds->server,
            NULL, /* we're not using the keytab */
            &flags,
            &pDecryptedTgs);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwKrb5FindPac(
        ctx,
        pDecryptedTgs,
        &serviceKey,
        &pNdrPacInfo,
        &ndrPacInfoSize);
    BAIL_ON_LW_ERROR(dwError);

    if (Flags & LW_KRB5_LOGIN_FLAG_UPDATE_CACHE)
    {
        /* 1. Copy old credentials from the existing user creds cache to
         *      the temporary cache.
         * 2. Delete the existing creds cache.
         * 3. Move the temporary cache file into the final path.
         */
        dwError = pthread_mutex_lock(&gLwKrb5State.UserCacheMutex);
        BAIL_ON_LW_ERROR(dwError);
        bInLock = TRUE;

        dwError = LwKrb5CopyFromUserCache(
                    ctx,
                    cc,
                    uid
                    );
        BAIL_ON_LW_ERROR(dwError);

        pszTempCacheName = krb5_cc_get_name(ctx, cc);
        if (!strncasecmp(pszTempCacheName, "FILE:", sizeof("FILE:")-1)) {
            pszTempCacheName += sizeof("FILE:") - 1;
        }

        dwError = LwAllocateString(pszTempCacheName, &pszTempCachePath);
        BAIL_ON_LW_ERROR(dwError);

        krb5_cc_close(ctx, cc);
        // Just to make sure no one accesses this now invalid pointer
        cc = NULL;

        dwError = LwKrb5MoveCCacheToUserPath(
                    ctx,
                    pszTempCachePath,
                    uid,
                    gid);
        if (dwError != LW_ERROR_SUCCESS)
        {
            /* Let the user login, even if we couldn't create the ccache for
             * them. Possible causes are:
             * 1. /tmp is readonly
             * 2. Another user maliciously setup a weird file (such as a
             *    directory) where the ccache would go.
             * 3. Someone created a ccache in the small window after we delete
             *    the old one and before we move in the new one.
             */
            LW_RTL_LOG_WARNING("Unable to set up credentials cache with tgt for uid %ld", (long)uid);
            dwError = LwRemoveFile(pszTempCachePath);
            BAIL_ON_LW_ERROR(dwError);
        }
    }

error:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pNdrPacInfo);
        ndrPacInfoSize = 0;
        dwGoodUntilTime = 0;
    }

    if (ctx)
    {
        // This function skips fields which are NULL
        krb5_free_cred_contents(ctx, &credsRequest);
    
        if (pTgsCreds != NULL)
        {
            krb5_free_creds(ctx, pTgsCreds);
        }
        
        if (pTgsTicket != NULL)
        {
            krb5_free_ticket(ctx, pTgsTicket);
        }
        
        if (pDecryptedTgs != NULL)
        {
            krb5_free_ticket(ctx, pDecryptedTgs);
        }
        
        if (authContext != NULL)
        {
            krb5_auth_con_free(ctx, authContext);
        }
        
        krb5_free_data_contents(ctx, &apReqPacket);
        krb5_free_data_contents(ctx, &salt);
        krb5_free_keyblock_contents(ctx, &serviceKey);

        if (cc != NULL)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }
    if (bInLock)
    {
        pthread_mutex_unlock(&gLwKrb5State.UserCacheMutex);
    }
    LW_SAFE_FREE_STRING(pszTempCachePath);

    *ppNdrPacInfo = pNdrPacInfo;
    *pNdrPacInfoSize = ndrPacInfoSize;
    *pdwGoodUntilTime = dwGoodUntilTime;

    return dwError;
}

DWORD
LwKrb5CheckInitiatorCreds(
    IN PCSTR pszTargetPrincipalName,
    OUT PBOOLEAN pbNeedCredentials
    )
{
    DWORD dwError = 0;
    BOOLEAN bNeedCredentials = FALSE;
    OM_uint32 majorStatus = 0;
    OM_uint32 minorStatus = 0;
    gss_ctx_id_t gssContext = GSS_C_NO_CONTEXT;
    gss_buffer_desc importName  = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc inputBufferDesc  = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc outputBufferDesc = GSS_C_EMPTY_BUFFER;
    OM_uint32 outputFlags = 0;
    gss_name_t targetName = GSS_C_NO_NAME;

    // discard const
    importName.value = (PSTR) pszTargetPrincipalName;
    importName.length = strlen(pszTargetPrincipalName);

    // discard const from GSS_KRB5_NT_PRINCIPAL_NAME
    majorStatus = gss_import_name(&minorStatus,
                                  &importName,
                                  (gss_OID) GSS_KRB5_NT_PRINCIPAL_NAME,
                                  &targetName);
    LW_GSS_LOG_IF_NOT_COMPLETE("gss_import_name", majorStatus, minorStatus);
    BAIL_ON_GSS_ERROR(dwError, majorStatus, minorStatus);

    // discard const from GSS_KRB5_NT_PRINCIPAL_NAME
    majorStatus = gss_init_sec_context(&minorStatus,
                                       NULL,
                                       &gssContext,
                                       targetName,
                                       (gss_OID) gss_mech_krb5,
                                       GSS_C_REPLAY_FLAG | GSS_C_MUTUAL_FLAG,
                                       0,
                                       NULL,
                                       &inputBufferDesc,
                                       NULL,
                                       &outputBufferDesc,
                                       &outputFlags,
                                       NULL);
    LW_GSS_LOG_IF_NOT_COMPLETE_OR_CONTINUE("gss_init_sec_context", majorStatus, minorStatus);

    // Need to cast below for correct comparison.  Note that
    // some compilers do not catch the mismatch.
    if (((majorStatus == GSS_S_FAILURE) &&
         ((minorStatus == (OM_uint32) KRB5KRB_AP_ERR_TKT_EXPIRED) ||
          (minorStatus == (OM_uint32) KRB5KDC_ERR_NEVER_VALID) ||
          (minorStatus == (OM_uint32) KRB5KDC_ERR_TGT_REVOKED))) ||
        ((majorStatus == GSS_S_CRED_UNAVAIL) &&
         (minorStatus == KG_EMPTY_CCACHE)))
    {
        // Need (new) Kerberos credentials because there are no
        // credentials or the credentials are expired or otherwise
        // invalid.

        bNeedCredentials = TRUE;
        goto error;
    }

    if (majorStatus == GSS_S_FAILURE)
    {
        switch (minorStatus)
        {
            case (OM_uint32) KRB5KRB_AP_ERR_SKEW:
                dwError = ERROR_TIME_SKEW;
                BAIL_ON_LW_ERROR(dwError);
                break;
            default:
                BAIL_ON_GSS_ERROR(dwError, majorStatus, minorStatus);
                break;
        }
    }

    if ((majorStatus != GSS_S_COMPLETE) &&
        (majorStatus != GSS_S_CONTINUE_NEEDED))
    {
        BAIL_ON_GSS_ERROR(dwError, majorStatus, minorStatus);
    }

error:
    if (targetName)
    {
        gss_release_name(&minorStatus, &targetName);
    }

    if (outputBufferDesc.value)
    {
        majorStatus = gss_release_buffer(&minorStatus, &outputBufferDesc);
    }

    if (gssContext)
    {
        majorStatus = gss_delete_sec_context(&minorStatus, &gssContext, GSS_C_NO_BUFFER);
    }

    *pbNeedCredentials = bNeedCredentials;

    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
