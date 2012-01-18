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
 * Module Name:
 *
 *        auth.c
 *
 * Abstract:
 *
 *        Methods related to AD authentication.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 17, 2010
 *
 */

#include "includes.h"

/**
 * Destroy Krb5 cached credentials.
 *
 * @param cachePath Path to the cache file.
 * @return 0 on success; error code on failure.
 */
static DWORD DestroyKrb5Cache(IN PCSTR cachePath)
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    ret = krb5_init_context(&ctx);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, cachePath, &cc);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_destroy(ctx, cc);

    if (ret != 0) {
        if (ret != KRB5_FCC_NOFILE) {
            ADT_BAIL_ON_KRB_ERROR(ctx, ret);
        }
        else {
            ret = 0;
        }
    }

    error:

        if (ctx) {
            krb5_free_context(ctx);
        }

        return (dwError);
}

/**
 * Get and cache TGT using the provided UPN and password.
 *
 * @param userPrincipal User principal name.
 * @param pszPassword Password.
 * @param cachePath Path to the cache file.
 * @return 0 on success; error code on failure.
 */
DWORD GetTgtFromNamePassword(
    PCSTR  userPrincipal,
    PCSTR  password,
    PCSTR  cachePath,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    krb5_get_init_creds_opt opts;
    krb5_principal client = NULL;
    krb5_creds creds = {
        0 };
    PSTR pszPass = NULL;
    PSTR pszUPN = NULL;
    PSTR pszRealmIdx = NULL;
    //BOOLEAN bUnlockExistingClientLock = FALSE;
    PWSTR pwszPass = NULL;

    dwError = LwAllocateString(userPrincipal, &pszUPN);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    if ((pszRealmIdx = strchr(pszUPN, '@')) == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    LwStrToUpper(++pszRealmIdx);

    ret = krb5_init_context(&ctx);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_tkt_life(&opts, ADT_KRB5_DEFAULT_TKT_LIFE);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    if (LW_IS_NULL_OR_EMPTY_STR(cachePath)) {
        ret = krb5_cc_default(ctx, &cc);
        ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    }
    else {
        ret = krb5_cc_resolve(ctx, cachePath, &cc);
        ADT_BAIL_ON_KRB_ERROR(ctx, ret);
    }

    ret = krb5_parse_name(ctx, pszUPN, &client);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    if (!LW_IS_NULL_OR_EMPTY_STR(password)) {
        dwError = LwAllocateString(password, &pszPass);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    // The converted string is not used, but the error code is.
    // krb5_get_init_creds_will return EINVAL if it cannot convert the name
    // from UTF8 to UCS2. By pretesting the string first, we know it is
    // convertable.
    dwError = LwMbsToWc16s(pszPass, &pwszPass);
    ADT_BAIL_ON_ERROR_NP(dwError);

    ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass, NULL,
                                       NULL, 0, NULL, &opts);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    if (ret == KRB5KRB_AP_ERR_SKEW) {

        ret = krb5_get_init_creds_password(ctx, &creds, client, pszPass, NULL,
                                           NULL, 0, NULL, &opts);
        ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    }
    else {
        ADT_BAIL_ON_KRB_ERROR(ctx, ret);
    }

    /*
    dwError = pthread_mutex_lock(&gLwKrb5State.ExistingClientLock);
    ADT_BAIL_ON_ERROR_NP(dwError);
    bUnlockExistingClientLock = TRUE;
    */

    /* Blow away the old credentials cache so that the old TGT is removed.
     * Otherwise, the new TGT will be stored at the end of the credentials
     * cache, and kerberos will use the old TGT instead.
     *
     * See bug 6908.
     */
    ret = krb5_cc_initialize(ctx, cc, client);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    if (pdwGoodUntilTime) {
        *pdwGoodUntilTime = creds.times.endtime;
    }

    cleanup:
        /*
        if (bUnlockExistingClientLock) {
            pthread_mutex_unlock(&gLwKrb5State.ExistingClientLock);
        }
        */

        if (ctx) {

            if (client) {
                krb5_free_principal(ctx, client);
            }

            krb5_cc_close(ctx, cc);
            krb5_free_cred_contents(ctx, &creds);
            krb5_free_context(ctx);
        }

        LW_SAFE_FREE_STRING(pszUPN);

        if (pwszPass) {
            size_t len;

            if (!LwWc16sLen(pwszPass, &len)) {
                memset(pwszPass, 0, len * sizeof(WCHAR));
            }
            LW_SAFE_FREE_MEMORY(pwszPass);
        }

        LW_SECURE_FREE_STRING(pszPass);

        return dwError;

    error:

        if (pdwGoodUntilTime) {
            *pdwGoodUntilTime = 0;
        }

        goto cleanup;
}

/**
 * Get and cache TGT using the keytab file. Keytab file path can be
 * set via KRB5_KTNAME environment variable.
 *
 * @param userPrincipal User principal name.
 * @param cachePath Path to the cache file.
 * @return 0 on success; error code on failure.
 */
DWORD GetTgtFromKeytab(
    PCSTR  userPrincipal,
    PCSTR  cachePath,
    PDWORD pdwGoodUntilTime
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_creds creds = {
        0 };
    krb5_ccache cc = NULL;
    krb5_keytab keytab = 0;
    krb5_principal client_principal = NULL;

    dwError = DestroyKrb5Cache(cachePath);
    ADT_BAIL_ON_ERROR_NP(dwError);

    ret = krb5_init_context(&ctx);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, userPrincipal, &client_principal);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, cachePath, &cc);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_kt_default(ctx, &keytab);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_init_creds_keytab(ctx, &creds, client_principal, keytab, 0, /* start time     */
    NULL, /* in_tkt_service */
    NULL /* options        */
    );
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client_principal);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    ADT_BAIL_ON_KRB_ERROR(ctx, ret);

    *pdwGoodUntilTime = creds.times.endtime;

cleanup:

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

error:

    *pdwGoodUntilTime = 0;

    goto cleanup;
}

/**
 * Process k5 options.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
DWORD ProcessK5Creds(IN AppContextTP appContext)
{
    DWORD dwError = 0;
    DWORD goodUntil = 0;

    PSTR tmpStr = NULL;
    PSTR tmpStrKT = NULL;
    PSTR upn = NULL;
    PSTR uName = NULL;
    PSTR ccPath = NULL;
    PSTR realm = NULL;

    PSTR pwdEnv = NULL;
    PSTR ktbEnv = NULL;
    PSTR uNameEnv = NULL;
    PSTR p = NULL;
    PSTR domainComp = NULL;

    if(appContext->aopts.isNonSec) {
        goto cleanup;
    }

    if (appContext->aopts.ticketCache || getenv("KRB5CCNAME")) {
        if(appContext->aopts.ticketCache) {
            dwError = LwAllocateStringPrintf(&tmpStr, "KRB5CCNAME=%s",
                                             appContext->aopts.ticketCache);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            putenv(tmpStr);
        }
    }
    else {
        if (appContext->aopts.logonAs) {
            uName = GetNameComp(appContext->aopts.logonAs);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(uName);

            domainComp = GetDomainCompFromUserName(appContext->aopts.logonAs);

            if (!IsUPN(appContext->aopts.logonAs)) {
                if(!domainComp) {
                    dwError = LwAllocateStringPrintf(&upn, "%s@%s", uName,
                                                     appContext->workConn->domainName);
                    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                }
                else {
                    p = strchr(appContext->workConn->domainName, '.');
                    dwError = LwAllocateStringPrintf(&upn, "%s@%s%s", uName,
                                                     domainComp, p);
                    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                }
            }
            else {
                realm = GetRealmComp(appContext->aopts.logonAs);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(realm);

                if(realm == NULL) {
                    dwError = LwStrDupOrNull(appContext->aopts.logonAs, &realm);
                    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                }

                LwStrToUpper(realm);

                dwError = LwAllocateStringPrintf(&upn, "%s@%s", uName, realm);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
            }

            LW_SAFE_FREE_MEMORY(appContext->aopts.logonAs);
            appContext->aopts.logonAs = upn;
            upn = NULL;
        }
        else {
            uNameEnv = getenv("ADT_LOGON_AS");
            if (uNameEnv) {
                if (!IsUPN(uNameEnv)) {
                    uName = GetNameComp(uNameEnv);
                    ADT_BAIL_ON_ALLOC_FAILURE_NP(uName);

                    domainComp = GetDomainCompFromUserName(uNameEnv);

                    if(!domainComp) {
                        dwError = LwAllocateStringPrintf(&upn, "%s@%s", uName,
                                                         appContext->workConn->domainName);
                        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                    }
                    else {
                        p = strchr(appContext->workConn->domainName, '.');
                        dwError = LwAllocateStringPrintf(&upn, "%s@%s%s", uName,
                                                         domainComp, p);
                        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
                    }

                    LW_SAFE_FREE_MEMORY(appContext->aopts.logonAs);
                    appContext->aopts.logonAs = upn;
                    upn = NULL;
                }
            }
        }

        if (appContext->aopts.logonAs) {
            dwError = LwAllocateStringPrintf(&ccPath, "/tmp/krb5cc_%s",
                                         appContext->aopts.logonAs);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }

        if (appContext->aopts.keytabFile) {
            dwError = LwAllocateStringPrintf(&tmpStrKT, "KRB5_KTNAME=%s",
                                             appContext->aopts.keytabFile);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            putenv(tmpStrKT);
        }

        ktbEnv = getenv("KRB5_KTNAME");

        if (ktbEnv) {
            if(!appContext->aopts.logonAs) {
                dwError = ADT_ERR_ARG_MISSING_UPN_FOR_KEYTAB;
                ADT_BAIL_ON_ERROR_NP(dwError);
            }

            PrintStderr(appContext,
                        LogLevelTrace,
                        "%s: Getting TGT from keytab file %s for UPN %s ...\n",
                        appContext->actionName,
                        ktbEnv,
                        appContext->aopts.logonAs);

            dwError = GetTgtFromKeytab((PCSTR) appContext->aopts.logonAs,
                                        (PCSTR) ccPath, &goodUntil);
            if(dwError > 99999) {
                dwError = ADT_ERR_AUTH_FAILED;
            }
            ADT_BAIL_ON_ERROR_NP(dwError);

            PrintStderr(appContext, LogLevelTrace, "%s: Got TGT for UPN %s.\n",
                        appContext->actionName, appContext->aopts.logonAs);

            dwError = LwAllocateStringPrintf(&tmpStr, "KRB5CCNAME=%s", ccPath);
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

            putenv(tmpStr);
        }
        else {
            if (!appContext->aopts.password) {
                pwdEnv = getenv("ADT_PASSWORD");
            }
            else {
                pwdEnv = appContext->aopts.password;
            }

            if (pwdEnv) {
                PrintStderr(appContext,
                            LogLevelTrace,
                            "%s: Getting TGT for UPN %s using name/password ...\n",
                            appContext->actionName,
                            appContext->aopts.logonAs);

                dwError = GetTgtFromNamePassword((PCSTR) appContext->aopts.logonAs,
                                                  (PCSTR)appContext->aopts.password,
                                                  (PCSTR) ccPath, &goodUntil);
                if(dwError > 99999) {
                    dwError = ADT_ERR_AUTH_FAILED;
                }
                ADT_BAIL_ON_ERROR_NP(dwError);

                PrintStderr(appContext, LogLevelTrace, "%s: Got TGT for UPN %s.\n",
                            appContext->actionName, appContext->aopts.logonAs);

                dwError = LwAllocateStringPrintf(&tmpStr, "KRB5CCNAME=%s",
                                                 ccPath);
                ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

                putenv(tmpStr);
            }
        }
    }

    if (appContext->aopts.logonAs) {
        PrintStderr(appContext,
                    LogLevelTrace,
                    "%s: Setting credentials for netapi calls for UPN %s ...\n",
                    appContext->actionName,
                    appContext->aopts.logonAs);

        dwError
                = NetCreateKrb5CredentialsA(appContext->aopts.logonAs,
                                            getenv("KRB5CCNAME"),
                                            (NET_CREDS_HANDLE *) &(appContext->creds));
        if(dwError) {
            dwError += ADT_WIN_ERR_BASE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        dwError = NetSetCredentials(appContext->creds);

        if(dwError) {
            dwError += ADT_WIN_ERR_BASE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }

        PrintStderr(appContext, LogLevelTrace, "%s: netapi credentials are set for UPN %s.\n",
                    appContext->actionName, appContext->aopts.logonAs);
    }

    appContext->isCredsSet = TRUE;

    cleanup:
        LW_SAFE_FREE_MEMORY(domainComp);
        LW_SAFE_FREE_MEMORY(realm);
        LW_SAFE_FREE_MEMORY(upn);
        LW_SAFE_FREE_MEMORY(uName);
        LW_SAFE_FREE_MEMORY(ccPath);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Process the password option.
 *
 * @param aopts Authentication options reference.
 * @return 0 on success; error code on failure.
 */
DWORD ProcessPassword(IN AuthOptionsTP aopts)
{
    DWORD dwError = 0;
    struct termios tty;
    int ttyErr;
    PCSTR buf;

    if(!aopts->password) {
        buf= getenv("ADTOOL_PASSWORD");
        if(buf != NULL) {
            dwError = LwAllocateString(buf, &(aopts->password));
            ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
        }
    }
    else {
        if(!strcmp(aopts->password, "-")) {
            ttyErr = tcgetattr(0, &tty);

            if(ttyErr < 0) {
                dwError = ProcessDash(&(aopts->password));
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
            else {
                dwError = ReadPasswdTty(&(aopts->password));
                ADT_BAIL_ON_ERROR_NP(dwError);
            }
        }
    }

    error:
        return dwError;
}

/**
 * Process the password option for a new user.
 *
 * @param aopts Authentication options reference.
 * @return 0 on success; error code on failure.
 */
DWORD ProcessADUserPassword(OUT PSTR *pwd)
{
    DWORD dwError = 0;
    struct termios tty;
    int ttyErr;

    if (*pwd && !strcmp(*pwd, "-")) {
        ttyErr = tcgetattr(0, &tty);

        if (ttyErr < 0) {
            dwError = ProcessDash(pwd);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
        else {
            dwError = ReadPasswdTty(pwd);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    error:
        return dwError;
}

/**
 * Read password from tty.
 *
 * @param passRet Read password.
 * @return 0 on success; error code on failure.
*/
DWORD ReadPasswdTty(IN PSTR* passRet)
{
    DWORD dwError = 0;
    CHAR szBuf[129];
    DWORD idx = 0;
    struct termios old, new;
    CHAR ch;

    fprintf(stdout, "Enter password:\n");

    memset(szBuf, 0, sizeof(szBuf));

    tcgetattr(0, &old);
    memcpy(&new, &old, sizeof(struct termios));
    new.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &new);

    while ( (idx < 128) ) {
        if (read(0, &ch, 1)) {
            if (ch != '\n') {
                szBuf[idx++] = ch;
            } else {
                break;
            }

        } else {
            dwError = LwMapErrnoToLwError(errno);
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    if (idx == 128) {
        dwError = LW_ERROR_ERRNO_ENOBUFS;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    if (idx > 0) {
        dwError = LwAllocateString(szBuf, passRet);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        *passRet = NULL;
    }

error:
    tcsetattr(0, TCSANOW, &old);

    return dwError;
}
