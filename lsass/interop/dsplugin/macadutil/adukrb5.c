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

#include "../includes.h"


#define MACADUTIL_KRB5_CACHEPATH LWDS_ADMIN_CACHE_DIR "/krb5_cc_lwedsplugin"

time_t gdwKrbTicketExpiryTime = 0;
const double gdwExpiryGraceSeconds = (60 * 60);
pthread_mutex_t g_ADULock = PTHREAD_MUTEX_INITIALIZER;

#define ENTER_KRB5_LOCK(bInLock)              \
        if (!bInLock) {                       \
           pthread_mutex_lock(&g_ADULock);    \
           bInLock = TRUE;                    \
        }

#define LEAVE_KRB5_LOCK(bInLock)              \
        if (bInLock) {                        \
           pthread_mutex_unlock(&g_ADULock);  \
           bInLock = FALSE;                   \
        }


static
BOOLEAN
ADUKrb5TicketHasExpired()
{
    BOOLEAN bExpired = FALSE;

    if (gdwKrbTicketExpiryTime == 0) {
        bExpired = TRUE;
    } else if (difftime(gdwKrbTicketExpiryTime,time(NULL)) < gdwExpiryGraceSeconds) {
        bExpired = TRUE;
    }

    return bExpired;
}

static
void
ADUStrToUpper(
    PSTR pszString
    )
{
    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = toupper(*pszString);
            pszString++;
        }
    }
}

DWORD
ADUKerb5DestroyCache(
    PSTR pszCachePath
    )
{
    DWORD dwError = 0;
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

    return(dwError);
}

static
DWORD
ADUKerb5GetTGTFromKeytab(
    char *szUserName,
    char *szPassword,
    char *pszCachePath,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_creds creds = { 0 };
    krb5_ccache cc = NULL;
    krb5_keytab keytab = 0;
    krb5_principal client_principal = NULL;

    dwError = ADUKerb5DestroyCache(pszCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_MAC_ERROR(dwError);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, szUserName, &client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_kt_default(ctx, &keytab);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_init_creds_keytab(
        ctx,
        &creds,
        client_principal,
        keytab,
        0,    /* start time     */
        NULL, /* in_tkt_service */
        NULL    /* options        */
        );
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);

    *pdwGoodUntilTime = creds.times.endtime;

error:

    if (creds.client == client_principal) {
        creds.client = NULL;
    }

    if (ctx) {
        if (client_principal) {
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

    return(dwError);
}

DWORD
ADUInitKrb5(
    PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    CHAR  szKrb5HostName[PATH_MAX];
    PSTR  pszKrb5CachePath = NULL;
    DWORD dwTicketExpiryTime = 0;
    char * szLocal = NULL;
    char * szDot = NULL;
    unsigned int   len = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_KRB5_LOCK(bInLock);

    if (ADUKrb5TicketHasExpired()) {
        memset(szKrb5HostName, 0, sizeof(szKrb5HostName));

        if (gethostname(szKrb5HostName, sizeof(szKrb5HostName)) != 0)
        {
            dwError = MAC_AD_ERROR_INVALID_NAME;;
            BAIL_ON_MAC_ERROR(dwError);
        }

        len = strlen(szKrb5HostName);
        if ( len > strlen(".local") )
        {
            szLocal = &szKrb5HostName[len - strlen(".local")];
            if ( !strcasecmp( szLocal, ".local" ) )
            {
                szLocal[0] = '\0';
            }
        }

        /* Test to see if the name is still dotted. If so we will chop it down to
           just the hostname field. */
        szDot = strchr(szKrb5HostName, '.');
        if ( szDot )
        {
            szDot[0] = '\0';
        }

        strcat(szKrb5HostName, "$@");
        strcat(szKrb5HostName, pszDomainName);
        ADUStrToUpper(szKrb5HostName);

        dwError = ADUKrb5GetSystemCachePath(&pszKrb5CachePath);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = ADUKerb5GetTGTFromKeytab(szKrb5HostName, NULL, pszKrb5CachePath, &dwTicketExpiryTime);
        BAIL_ON_MAC_ERROR(dwError);

        gdwKrbTicketExpiryTime = dwTicketExpiryTime;
    }

cleanup:

    LEAVE_KRB5_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADUKrb5GetSystemCachePath(
    PSTR*         ppszCachePath
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bDirExists = FALSE;

    dwError = LwCheckFileTypeExists(LWDS_ADMIN_CACHE_DIR, LWFILE_DIRECTORY, &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!bDirExists)
    {
        dwError = LwCreateDirectory(LWDS_ADMIN_CACHE_DIR, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LwAllocateString(
                "FILE:" MACADUTIL_KRB5_CACHEPATH,
                &pszCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszCachePath = pszCachePath;

cleanup:

    return dwError;

error:

    *ppszCachePath = NULL;

    goto cleanup;
}

DWORD
ADUKrb5GetUserCachePathAndSID(
    PCSTR pszUserUPN,
    PSTR* ppszCachePath,
    PSTR* ppszHomeDirPath,
    PSTR* ppszSID,
    uid_t* pUid
    )
{
    DWORD dwError = 0;
    char szPath[PATH_MAX];
    PSTR  pszCachePath = NULL;
    PSTR  pszHomeDirPath = NULL;
    PSTR  pszSID = NULL;
    HANDLE hLsaConnection = (HANDLE) NULL;
    PLSA_USER_INFO_0 pUserInfo_0 = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindUserByName(hLsaConnection, pszUserUPN, 0, (PVOID*) &pUserInfo_0);
    BAIL_ON_MAC_ERROR(dwError);

    memset(szPath, 0, sizeof(szPath));
    sprintf(szPath, "FILE:/tmp/krb5cc_%ld",(long)pUserInfo_0->uid);

    dwError = LwAllocateString(szPath, &pszCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(pUserInfo_0->pszSid, &pszSID);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateString(pUserInfo_0->pszHomedir, &pszHomeDirPath);
    BAIL_ON_MAC_ERROR(dwError);

    if (ppszCachePath)
    {
        *ppszCachePath = pszCachePath;
        pszCachePath = NULL;
    }

    if (ppszSID)
    {
        *ppszSID = pszSID;
        pszSID = NULL;
    }

    if (ppszHomeDirPath)
    {
        *ppszHomeDirPath = pszHomeDirPath;
        pszHomeDirPath = NULL;
    }

    if (pUid)
    {
        *pUid = pUserInfo_0->uid;
    }

cleanup:

    if (pszCachePath)
        LwFreeString(pszCachePath);

    if (pszSID)
        LwFreeString(pszSID);

    if (pszHomeDirPath)
        LwFreeString(pszHomeDirPath);

    if (pUserInfo_0)
        LsaFreeUserInfo(0, pUserInfo_0);

    if (hLsaConnection != (HANDLE)NULL)
        LsaCloseServer(hLsaConnection);

    return dwError;

error:

    if (ppszCachePath)
        *ppszCachePath = NULL;

    if (ppszSID)
        *ppszSID = NULL;

    if (ppszHomeDirPath)
        *ppszHomeDirPath = NULL;

    if (pUid)
        *pUid = -1;

    goto cleanup;
}

DWORD
ADUKrb5SetDefaultCachePath(
    PSTR  pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    DWORD dwError       = 0;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(dwMajorStatus, dwMinorStatus);

    if (ppszOrigCachePath) {
        if (!IsNullOrEmptyString(pszOrigCachePath)) {
            dwError = LwAllocateString(pszOrigCachePath, ppszOrigCachePath);
            BAIL_ON_MAC_ERROR(dwError);
        } else {
            *ppszOrigCachePath = NULL;
        }
    }

cleanup:

    return dwError;

error:

    if (ppszOrigCachePath) {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}

DWORD
ADUKrb5GetDefaultCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    PCSTR pszCurrentPath = NULL;
    PSTR  pszPath = NULL;
    krb5_context ctx = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszCurrentPath = krb5_cc_default_name(ctx);

    if (IsNullOrEmptyString(pszCurrentPath))
    {
        dwError = ENOENT;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LwAllocateString(
                  pszCurrentPath,
                  &pszPath);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszPath = pszPath;

cleanup:

    if (ctx) {
       krb5_free_context(ctx);
    }

    return dwError;

error:

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
ADUKrb5GetPrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context    ctx = NULL;
    krb5_ccache     cc = NULL;
    krb5_principal  pKrb5Principal = NULL;
    PSTR  pszKrb5PrincipalName = NULL;
    PSTR  pszPrincipalName = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_get_principal(ctx, cc, &pKrb5Principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_unparse_name(ctx, pKrb5Principal, &pszKrb5PrincipalName);
    BAIL_ON_KRB_ERROR(ctx, ret);

    dwError = LwAllocateString(pszKrb5PrincipalName, &pszPrincipalName);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszPrincipalName = pszPrincipalName;

cleanup:

    if (ctx)
    {
        if (pszKrb5PrincipalName)
        {
            krb5_free_unparsed_name(ctx, pszKrb5PrincipalName);
        }
        if (pKrb5Principal)
        {
            krb5_free_principal(ctx, pKrb5Principal);
        }
        if (cc)
        {
            krb5_cc_close(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    return dwError;

error:

    *ppszPrincipalName = NULL;

    goto cleanup;
}

