/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        authsvc.c
 *
 * Abstract:
 *
 *        Likewise Group Policy
 *
 *        LSASS Authentication Service Interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

CENTERROR
GPAInitAuthService(
    VOID
    )
{
    return LWNetExtendEnvironmentForKrb5Affinity(TRUE);
}

CENTERROR
GPAGetCurrentDomain(
    PSTR* ppszDomain
    )
{
	DWORD dwError = 0;
	PSTR  pszDomain_lwnet = NULL;
	PSTR  pszDomain = NULL;

	dwError = LWNetGetCurrentDomain(&pszDomain_lwnet);
	if (dwError != 0)
	{
            goto error;
	}

	dwError = LwAllocateString(pszDomain_lwnet, &pszDomain);
	BAIL_ON_CENTERIS_ERROR(dwError);

	*ppszDomain = pszDomain;

cleanup:

	if (pszDomain_lwnet)
	{
		LWNetFreeString(pszDomain_lwnet);
	}

	return dwError;

error:

	*ppszDomain = NULL;

	LW_SAFE_FREE_STRING(pszDomain);

	goto cleanup;
}

CENTERROR
GPAIsJoinedToAD(
    PBOOLEAN pbIsJoined
    )
{
	DWORD dwError = 0;
	PSTR  pszDomain_lwnet = NULL;

	dwError = LWNetGetCurrentDomain(&pszDomain_lwnet);
	if (dwError != 0)
	{
            goto error;
	}

	*pbIsJoined = TRUE;

cleanup:

	if (pszDomain_lwnet)
	{
		LWNetFreeString(pszDomain_lwnet);
	}

	return dwError;

error:

	*pbIsJoined = FALSE;

	goto cleanup;
}

CENTERROR
GPAInitKerberos(
    DWORD dwPollingInterval
    )
{
    DWORD dwError = 0;
    PSTR  pszMachineName = NULL;
    PSTR  pszMachineAccountName = NULL;
    PSTR  pszKrb5CachePath = NULL;
    PSTR  pszDomainName = NULL;
    DWORD lifetime = 0;

    if (GPAKrb5TicketHasExpired()) {

       DWORD dwTicketExpiryTime = 0;

       dwError = GPAGetDnsSystemNames(NULL, &pszMachineName, NULL);
       BAIL_ON_CENTERIS_ERROR(dwError);

       dwError = GPAGetCurrentDomain(&pszDomainName);
       if (dwError) goto error;

       dwError = LwAllocateStringPrintf(
                               &pszMachineAccountName,
                               "%s@%s",
                               pszMachineName,
                               pszDomainName);
       BAIL_ON_CENTERIS_ERROR(dwError);

       LwStrToUpper(pszMachineAccountName);

       dwError = GPAGetKrb5CachePath(&pszKrb5CachePath);
       BAIL_ON_CENTERIS_ERROR(dwError);

       dwError = GPAGetTGTFromKeytab(
    		   			pszMachineAccountName,
    		   			NULL,
    		   			pszKrb5CachePath,
    		   			&dwTicketExpiryTime);
       if (dwError) goto error;

       gdwKrbTicketExpiryTime = dwTicketExpiryTime;

       lifetime = difftime(gdwKrbTicketExpiryTime, time(NULL));
       gdwExpiryGraceSeconds = LW_MAX(lifetime / 2, 2 * dwPollingInterval);
    }

cleanup:

	LW_SAFE_FREE_STRING(pszMachineName);
	LW_SAFE_FREE_STRING(pszDomainName);
	LW_SAFE_FREE_STRING(pszMachineAccountName);

	return dwError;

error:

	goto cleanup;
}

BOOLEAN
GPAKrb5TicketHasExpired(
    VOID
    )
{
	BOOLEAN bExpired = FALSE;

    if (gdwKrbTicketExpiryTime == 0) {

        GPA_LOG_VERBOSE("Acquiring new Krb5 ticket...");
        bExpired = TRUE;

    } else if (difftime(gdwKrbTicketExpiryTime,time(NULL)) < gdwExpiryGraceSeconds) {

        GPA_LOG_VERBOSE("Renewing Krb5 Ticket...");
        bExpired = TRUE;
    }

    return bExpired;
}

DWORD
GPAGetKrb5CachePath(
    PSTR* ppszKrb5CachePath
    )
{
    DWORD dwError = 0;
    PSTR pszCachePath = NULL;

    dwError = LwAllocateStringPrintf(
    				&pszCachePath,
    				"%s/krb5cc_gpagentd",
    				CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(dwError);

    *ppszKrb5CachePath = pszCachePath;

cleanup:

	return dwError;

error:

    *ppszKrb5CachePath = NULL;

    LW_SAFE_FREE_STRING(pszCachePath);

    goto cleanup;
}

DWORD
GPAGetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  pszCachePath,
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

    dwError = GPADestroyKrb5Cache(pszCachePath);
    BAIL_ON_CENTERIS_ERROR(dwError);

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszUserName, &client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    GPA_LOG_VERBOSE("Generated KRB5 principal name for %s (realm:%s, data:%s)",
    				pszUserName,
    				client_principal->realm.data,
    				client_principal->data->data);

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

CENTERROR
GPAGetPrincipalFromKrb5Cache(
    PCSTR pszCachePath,
    PSTR * ppszPrincipal
    )
{
    DWORD dwError = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    krb5_principal client_principal = NULL;
    PSTR pszPrincipal = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    GPA_LOG_VERBOSE("Getting principal from krb5 cache");
    ret = krb5_cc_get_principal(ctx, cc, &client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    if (client_principal->realm.length &&
        client_principal->data->length)
    {
        dwError = LwAllocateStringPrintf(&pszPrincipal,
                                         "%s@%s",
                                         client_principal->data->data,
                                         client_principal->realm.data);
        BAIL_ON_CENTERIS_ERROR(dwError);
    }
    else
    {
        dwError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(dwError);
    }

    *ppszPrincipal = pszPrincipal;
    pszPrincipal = NULL;

error:

    if (ctx)
    {
        if (client_principal)
        {
            krb5_free_principal(ctx, client_principal);
        }

       krb5_free_context(ctx);
    }

    LW_SAFE_FREE_STRING(pszPrincipal);

    return(dwError);
}

DWORD
GPADestroyKrb5Cache(
    PCSTR pszCachePath
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

    GPA_LOG_VERBOSE("Destroying krb5 cache");
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

CENTERROR
GPAGetShortDomainName(
    PCSTR pszDomainName,
    PSTR* ppszShortDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLWNET_DC_INFO pDCInfo = NULL;
    PSTR pszShortDomainName = NULL;

    ceError = LWNetGetDCName(
                NULL,
                pszDomainName,
                NULL,  //pszSiteName
                0,     //dwFlags
                &pDCInfo);
    if (ceError != 0)
    {
        goto error;
    }

    ceError = LwAllocateString(
                  pDCInfo->pszNetBIOSDomainName,
                  &pszShortDomainName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszShortDomainName = pszShortDomainName;

cleanup:

    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    return ceError;

error:

    *ppszShortDomainName = NULL;

    LW_SAFE_FREE_STRING(pszShortDomainName);

    goto cleanup;
}

CENTERROR
GPAEnsureAuthIsRunning(
    VOID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int i = 0;
    BOOLEAN bRunning = FALSE;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
 
    for (i = 0; i < 24; i++) {

        GPA_LOG_VERBOSE("Checking for Likewise LSASS Daemon ...");

        ceError = GPAIsProgramRunning( GP_LSASS_PID_FILE,
                                      GP_LSASS_DAEMON_NAME,
                                      NULL,
                                      &bRunning );
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bRunning)
           break;

        GPA_LOG_VERBOSE("Waiting for Likewise LSASS Daemon ...");

        sleep(5);
    }

    if (bRunning)
    {
        // The lsass daemon is running as a process, now confirm that it is responsive to API calls. This
        // check is needed when lsass is looking for trusted domains, we need to wait till it is ready.

        for (i = 0; i < 12; i++) {
            GPA_LOG_VERBOSE("Checking that Likewise LSASS Daemon is responding to API calls ...");

            ceError = LsaOpenServer(&hLsaConnection);
            if (ceError)
            {
                GPA_LOG_VERBOSE("Likewise LSASS Daemon is not able to open handle for API calls. Last error: %d", ceError);
                ceError = 0;
            }
            else
            {
                ceError = LsaGetStatus(hLsaConnection,
                                       &pLsaStatus);
                if (ceError)
                {
                    GPA_LOG_VERBOSE("Likewise LSASS Daemon is not yet responding to API calls. Last error: %d", ceError);
                    ceError = 0;
                }
                else
                {
                    break;
                }
            }

            sleep(5);
        }
    }

    if (!bRunning) {
        GPA_LOG_ALWAYS("Likewise LSASS Daemon not running!");
        ceError = CENTERROR_LSASS_NOT_RUNNING;
    }

error:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return ceError;
}

CENTERROR
GPAGetPreferredDC(
    PSTR* ppszDC
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PLWNET_DC_INFO pDCInfo = NULL;
    PSTR pszDC = NULL;
    PSTR pszDomain = NULL;

    ceError = GPAGetCurrentDomain(&pszDomain);
    if (ceError != 0)
    {
        goto error;
    }

    ceError = LWNetGetDCName(
                NULL,
                pszDomain,
                NULL,  //pszSiteName
                0,     //dwFlags
                &pDCInfo);
    if (ceError != 0)
    {
        goto error;
    }

    ceError = LwAllocateString(
                  pDCInfo->pszDomainControllerName,
                  &pszDC);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszDC = pszDC;

cleanup:

    if (pDCInfo)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    LW_SAFE_FREE_STRING(pszDomain);

    return ceError;

error:

    *ppszDC = NULL;

    LW_SAFE_FREE_STRING(pszDC);

    goto cleanup;
}

CENTERROR
GPAGetPreferredDomainController(
    PCSTR pszDomain,
    PSTR* ppszDomainController
    )
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszDCFQDN = NULL;
	PSTR pszDomainController = NULL;

	//
	// TODO: Check offline mode
	//
	ceError = LWNetGetDomainController(
	                pszDomain,
	                &pszDCFQDN);
	if (ceError != 0) {
	    goto error;
	}

	ceError = LwAllocateString(pszDCFQDN,
                                   &pszDomainController);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszDomainController = pszDomainController;

cleanup:

	if (pszDCFQDN)
	{
		LWNetFreeString(pszDCFQDN);
	}

	return ceError;

error:

	*ppszDomainController = NULL;

	LW_SAFE_FREE_STRING(pszDomainController);

	goto cleanup;
}

CENTERROR
GPAGetDomainController(
    PCSTR pszDomain,
    PSTR* ppszDomainController
    )
{
	return GPAGetPreferredDomainController(pszDomain, ppszDomainController);
}

CENTERROR
GPAGetDnsSystemNames(
    PSTR* ppszHostName,
    PSTR* ppszMachineName,
    PSTR* ppszDomain
    )
{
    return GetDnsSystemNames(ppszHostName, ppszMachineName, ppszDomain);
}

CENTERROR
GPAGetDomainSID(
    PSTR* ppszDomainSID
    )
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszDomainSID = NULL;

	// TODO: Get a real domain SID
	ceError = LwAllocateString("S-1-22-1", &pszDomainSID);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszDomainSID = pszDomainSID;

cleanup:

	return ceError;

error:

	*ppszDomainSID = NULL;

	LW_SAFE_FREE_STRING(pszDomainSID);

	goto cleanup;
}

CENTERROR
GPAFindUserByName(
    PCSTR pszLoginId,
    PGPUSER* ppGPUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hLsa = (HANDLE)NULL;
    PGPUSER pUser = NULL;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;

    ceError = LsaOpenServer(&hLsa);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LsaFindUserByName(
                            hLsa,
                            pszLoginId,
                            dwUserInfoLevel,
                            (PVOID*)&pUserInfo);
    if (ceError == LW_ERROR_NO_SUCH_USER)
    {
        ceError = CENTERROR_GP_NOT_AD_USER;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory(
                    sizeof(GPUSER),
                    (PVOID*)&pUser);
                    BAIL_ON_CENTERIS_ERROR(ceError);

    pUser->uid = pUserInfo->uid;

    ceError = LwAllocateString(
                    pUserInfo->pszSid,
                    &pUser->pszSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
        ceError = LwAllocateString(
                        pUserInfo->pszHomedir,
                        &pUser->pszHomeDir);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (!IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        ceError = LwAllocateString(
                        pUserInfo->pszUPN,
                        &pUser->pszUserPrincipalName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = LwAllocateString(
                    pszLoginId,
                    &pUser->pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppGPUser = pUser;

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    if (hLsa != (HANDLE)NULL)
    {
        LsaCloseServer(hLsa);
    }

    return ceError;

error:

    *ppGPUser = NULL;

    if (pUser)
    {
        GPAFreeUser(pUser);
    }

    goto cleanup;
}

CENTERROR
GPAFindUserById(
    PCSTR    pszLoginId,
    uid_t    uid,
    PGPUSER* ppGPUser
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hLsa = (HANDLE)NULL;
    PGPUSER pUser = NULL;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;

    ceError = LsaOpenServer(&hLsa);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LsaFindUserById(
                            hLsa,
                            uid,
                            dwUserInfoLevel,
                            (PVOID*)&pUserInfo);
    if (ceError == LW_ERROR_NO_SUCH_USER)
    {
        ceError = CENTERROR_GP_NOT_AD_USER;
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory(
                    sizeof(GPUSER),
                    (PVOID*)&pUser);
                    BAIL_ON_CENTERIS_ERROR(ceError);

    pUser->uid = pUserInfo->uid;

    ceError = LwAllocateString(
                    pUserInfo->pszSid,
                    &pUser->pszSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
        ceError = LwAllocateString(
                        pUserInfo->pszHomedir,
                        &pUser->pszHomeDir);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (!IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        ceError = LwAllocateString(
                        pUserInfo->pszUPN,
                        &pUser->pszUserPrincipalName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (IsNullOrEmptyString(pszLoginId)) {
       ceError = LwAllocateString(
                        pUserInfo->pszName,
                        &pUser->pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {

        ceError = LwAllocateString(
                        pszLoginId,
                        &pUser->pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppGPUser = pUser;

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    if (hLsa != (HANDLE)NULL)
    {
        LsaCloseServer(hLsa);
    }

    return ceError;

error:

    *ppGPUser = NULL;

    if (pUser)
    {
        GPAFreeUser(pUser);
    }

    goto cleanup;
}

VOID
GPAFreeUser(
    PGPUSER pUser
    )
{
    LW_SAFE_FREE_STRING(pUser->pszName);
    LW_SAFE_FREE_STRING(pUser->pszUserPrincipalName);
    LW_SAFE_FREE_STRING(pUser->pszHomeDir);
    LW_SAFE_FREE_STRING(pUser->pszSID);
    LwFreeMemory(pUser);
}

