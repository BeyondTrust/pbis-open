#include "includes.h"

static GPAPOLLERINFO gPollerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    GPO_DEFAULT_POLL_TIMEOUT_SECS,
    1
};

extern GPOCSEINFO gCSEInfo;

static PSTR gpszMachineDN = NULL;

static pthread_t gMachinePolicyThread;
static PVOID     pgMachinePolicyThread = NULL;

static
CENTERROR
ProcessMachineGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList
    )
{
    CENTERROR ceError =  CENTERROR_SUCCESS;
    DWORD dwFlags = 0;
    PGROUP_POLICY_CLIENT_EXTENSION pGPCSEList = NULL;
    PGROUP_POLICY_OBJECT pGPOModifiedList = NULL;
    PGROUP_POLICY_OBJECT pGPODeletedList = NULL;
    PGROUP_POLICY_OBJECT pGPOCSEDelList = NULL;
    PGROUP_POLICY_OBJECT pGPOCSEModList = NULL;
    BOOLEAN bEnableEventLog = FALSE;

    pthread_rwlock_rdlock(&gCSEInfo.lock);

    GPA_LOG_VERBOSE("Computing Modifications and Deletions...");
    GPA_LOG_VERBOSE("pGPOCurrentList=[%.8x];pGPOExistingList=[%.8x]",pGPOCurrentList,pGPOExistingList);

    ceError = GPAComputeModDelGroupPolicyList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPOModifiedList,
        &pGPODeletedList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ( !pGPOModifiedList && !pGPODeletedList ) {
        GPA_LOG_VERBOSE("There are no new changes to apply from the last policy cycle.");
        goto cleanup;
    }

    pGPCSEList = gCSEInfo.pGPCSEList;

    GPA_LOG_VERBOSE("pGPCSEList=[%.8x]",pGPCSEList);

    while (pGPCSEList) {

        GPA_LOG_VERBOSE("Processing Group Policy Extension: %s", pGPCSEList->pszName);

        if (pGPCSEList->dwNoMachinePolicy) {
            GPA_LOG_VERBOSE("Group policiy extension (%s) does not handle machine policy types", pGPCSEList->pszName);
            pGPCSEList = pGPCSEList->pNext;
            continue;
        }

        if ( pGPCSEList->pfnProcessGroupPolicy == NULL ) {
            GPA_LOG_ALWAYS("Cannot process group policies for extension (%s), since client side extension failed to load", pGPCSEList->pszName);
            pGPCSEList = pGPCSEList->pNext;
            continue;
        }

        ceError = GPAComputeCSEModDelList(
            MACHINE_GROUP_POLICY,
            pGPCSEList->pszGUID,
            pGPOModifiedList,
            pGPODeletedList,
            &pGPOCSEModList,
            &pGPOCSEDelList
            );
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pGPCSEList->pszName) {
           GPA_LOG_VERBOSE("Processing group policy extension: %s (CSE Modified list: %s, CSE Deleted list: %s)", pGPCSEList->pszName, pGPOCSEModList ? "yes" : "no", pGPOCSEDelList ? "yes" : "no");
        } else if (pGPCSEList->pszGUID) {
           GPA_LOG_VERBOSE("Processing group policy extension: %s (CSE Modified list: %s, CSE Deleted list: %s)", pGPCSEList->pszGUID, pGPOCSEModList ? "yes" : "no", pGPOCSEDelList ? "yes" : "no");
        }
		
        ceError = pGPCSEList->pfnProcessGroupPolicy(
                dwFlags,
                NULL,
                pGPOCSEModList,
                pGPOCSEDelList
                );

        GetEnableEventLog(&bEnableEventLog);
        if(bEnableEventLog) {

	        GPAPostCSEEvent(ceError,
                            NULL,
                            pGPCSEList->pszName ? pGPCSEList->pszName :
                            pGPCSEList->pszGUID ? pGPCSEList->pszGUID :
                            "Unknown Group Policy client-side extention",
                            pGPCSEList->pszDllName,
                            pGPOCSEModList,
                            pGPOCSEDelList);
        } else {

            GPA_LOG_VERBOSE("EnableEventLog bit is not set. Hence not posting events");
        }

        BAIL_ON_CENTERIS_ERROR(ceError);

        GPA_SAFE_FREE_GPO_LIST(pGPOCSEModList);
        GPA_SAFE_FREE_GPO_LIST(pGPOCSEDelList);

        pGPCSEList = pGPCSEList->pNext;
    }

cleanup:

    GPA_SAFE_FREE_GPO_LIST(pGPOModifiedList);
    GPA_SAFE_FREE_GPO_LIST(pGPODeletedList);

    pthread_rwlock_unlock(&gCSEInfo.lock);

    return (ceError);

error:

    GPA_SAFE_FREE_GPO_LIST(pGPOCSEDelList);
    GPA_SAFE_FREE_GPO_LIST(pGPOCSEModList);

    goto cleanup;
}

#define LOG_ERROR_AND_WAIT(code, id, message)                           \
    do {                                                                \
        GPAPostAgentEvent(code, id, message);                           \
        if (code) {                                                     \
            GPA_LOG_ERROR("Error processing group policies while %s, error: [0x%8X] (%s)", message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto gpo_wait;                                              \
        }                                                               \
    }                                                                   \
    while (0)

#define LOG_VERBOSE_AND_WAIT(code, id, message)                         \
    do {                                                                \
        if (code) {                                                     \
            GPA_LOG_VERBOSE("Error processing group policies while %s, error: [0x%8X] (%s)", message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto gpo_wait;                                              \
        }                                                               \
    }                                                                   \
    while (0)

static
PVOID
GPAMachinePolicyLoop()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct timespec timeout;
    PGROUP_POLICY_OBJECT pGPOCurrentList = NULL;
    PGROUP_POLICY_OBJECT pGPOExistingList = NULL;
    HANDLE               hDirectory = (HANDLE)NULL;
    PSTR                 pszMachineName = NULL;
    PSTR                 pszADDomain = NULL;
    PSTR                 pszDomainControllerName = NULL;
    DWORD                dwPollingInterval;
    BOOLEAN              bExtensionsLoaded = FALSE;
#if defined(__LWI_DARWIN__)
    BOOLEAN              fFlushedMacCache = FALSE;
#endif

    GPA_LOG_INFO("Group Policy Poller started");

    pthread_mutex_lock(&gPollerInfo.lock);

    while (gPollerInfo.dwRefresh) {
        if (!bExtensionsLoaded)
        {
            ceError = LoadClientSideExtensions();
            LOG_ERROR_AND_WAIT(ceError, 1, "loading client-side extensions");
            if (ceError == 0)
            {
                bExtensionsLoaded = TRUE;
            }
        }

        GPA_LOG_INFO("Starting to refresh computer group policies...");

        gPollerInfo.dwPollingInterval = 5 * 60;

        ceError = LoadGroupPolicyConfigurationSettings(TRUE);
        LOG_ERROR_AND_WAIT(ceError, 2, "loading group policy configuration settings");

        ceError = GetComputerPollingInterval(&dwPollingInterval);
        LOG_ERROR_AND_WAIT(ceError, 3, "retrieving computer policy refresh interval");

        GPA_LOG_VERBOSE("Computer policy refresh interval: [%d] seconds",
                        dwPollingInterval);

        ceError = GPAInitKerberos(dwPollingInterval);
        LOG_VERBOSE_AND_WAIT(ceError, 4, "initializing kerberos");

        ceError = GPOGetADDomain( &pszADDomain);
        LOG_VERBOSE_AND_WAIT(ceError, 5, "determining Active Directory domain name");

        ceError = GPAGetDomainController( pszADDomain, &pszDomainControllerName );
        LOG_VERBOSE_AND_WAIT(ceError, 6, "determining domain controller name");

        ceError = GPAGetDnsSystemNames(NULL, &pszMachineName, NULL);
        LOG_ERROR_AND_WAIT(ceError, 7, "determining computer system names");

        ceError = GPOOpenDirectory(pszADDomain, pszDomainControllerName, &hDirectory);
        LOG_VERBOSE_AND_WAIT(ceError, 8, "opening LDAP directory");

#if defined(__LWI_DARWIN__)
       if (fFlushedMacCache == FALSE) {
           ceError = FlushDirectoryServiceCache();
           if (!CENTERROR_IS_OK(ceError)) {
               GPA_LOG_ALWAYS("Failed to flush the Mac DirectoryService cache. Error: [%d] (%s)",
                              ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
           } else {
               fFlushedMacCache = TRUE;
           }
       }
#endif

        LW_SAFE_FREE_STRING(gpszMachineDN);

        ceError = GPAFindComputerDN(hDirectory,
                                    pszMachineName,
                                    pszADDomain,
                                    &gpszMachineDN);
        LOG_ERROR_AND_WAIT(ceError, 9, "determining machine distinguished name in Active Directory domain");

        GPA_LOG_VERBOSE("Machine DN: %s", gpszMachineDN);

        LW_SAFE_FREE_STRING(pszADDomain);
        LW_SAFE_FREE_STRING(pszDomainControllerName);
        LW_SAFE_FREE_STRING(pszMachineName);

        gPollerInfo.dwRefresh = 0;

        ceError = GPAGetGPOList( NULL,
                                 hDirectory,
                                 gpszMachineDN,
                                 &pGPOCurrentList);
        LOG_ERROR_AND_WAIT(ceError, 10, "determining group policy object list for computer");

        ceError = ProcessMachineGroupPolicyList(
            pGPOCurrentList,
            pGPOExistingList
            );
        if (ceError) {
            GPA_SAFE_FREE_GPO_LIST(pGPOCurrentList);
            LOG_ERROR_AND_WAIT(ceError, 11, "processing list of group policy objects for computer");
        }

        GPA_SAFE_FREE_GPO_LIST(pGPOExistingList);

        pGPOExistingList = pGPOCurrentList;
        pGPOCurrentList = NULL;
        gPollerInfo.dwPollingInterval = dwPollingInterval;

        GPA_LOG_INFO("Completed refreshing computer group policies...");

    gpo_wait:

        if (hDirectory) {
            GPOCloseDirectory(hDirectory);
            hDirectory = (HANDLE)NULL;
        }

        timeout.tv_sec = time(NULL) + gPollerInfo.dwPollingInterval;
        timeout.tv_nsec = 0;

        ceError = pthread_cond_timedwait(&gPollerInfo.pollCondition,
                                         &gPollerInfo.lock,
                                         &timeout);
        if (ProcessShouldExit())
            break;

        if (ceError == ETIMEDOUT)
            gPollerInfo.dwRefresh = 1;

    }

    pthread_mutex_unlock(&gPollerInfo.lock);

    GPA_LOG_INFO("Group Policy Poller stopped");

    LW_SAFE_FREE_STRING(gpszMachineDN);
    LW_SAFE_FREE_STRING(pszADDomain);
    LW_SAFE_FREE_STRING(pszDomainControllerName);
    LW_SAFE_FREE_STRING(pszMachineName);
    
    GPA_SAFE_FREE_GPO_LIST(pGPOCurrentList);
    GPA_SAFE_FREE_GPO_LIST(pGPOExistingList);
    
    if (hDirectory) {
        GPOCloseDirectory(hDirectory);
    }

    return NULL;

/*error:*/

    pthread_mutex_unlock(&gPollerInfo.lock);

    GPA_LOG_INFO("Group Policy Poller stopped [Error code:%d]", CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");

    LW_SAFE_FREE_STRING(gpszMachineDN);
    LW_SAFE_FREE_STRING(pszADDomain);
    LW_SAFE_FREE_STRING(pszDomainControllerName);
    LW_SAFE_FREE_STRING(pszMachineName);
    
    GPA_SAFE_FREE_GPO_LIST(pGPOCurrentList);
    GPA_SAFE_FREE_GPO_LIST(pGPOExistingList);

    if (hDirectory) {
        GPOCloseDirectory(hDirectory);
    }

    return NULL;
}

CENTERROR
GPAStartMachinePolicyThread()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = pthread_create(&gMachinePolicyThread,
    		                 NULL,
    		                 GPAMachinePolicyLoop,
    		                 NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pgMachinePolicyThread = &gMachinePolicyThread;

    return (ceError);

error:

    pgMachinePolicyThread = NULL;

    return (ceError);
}

CENTERROR
GPAStopMachinePolicyThread()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (pgMachinePolicyThread) {
        /* On some platforms (e.g. Darwin), pthread_cond_timedwait is not
         * a cancellation point.  So, wake up the poller thread so that it can
         * check the termination condition and exit.
         */
    	GPARefreshMachinePolicies();
        pthread_join(gMachinePolicyThread, NULL);
    }

    return (ceError);
}

CENTERROR
GPARefreshMachinePolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    pthread_mutex_lock(&gPollerInfo.lock);

    gPollerInfo.dwRefresh = 1;
    ceError = pthread_cond_signal(&gPollerInfo.pollCondition);

    pthread_mutex_unlock(&gPollerInfo.lock);

    return (ceError);
}

