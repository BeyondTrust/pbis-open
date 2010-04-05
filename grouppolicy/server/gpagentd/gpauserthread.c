#include "includes.h"

#define FACILITY_USER_POLLER            "User poller"
#define FACILITY_USER_POLICY_APPLICATOR "User policy applicator"

#define SAFE_PRINT_STRING(str) (IsNullOrEmptyString(str) ? "<null>" : (str))

static GPAPOLLERINFO gUserPollerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    1 * 60,
    1
};

static GPAPOLLERINFO gUserPolicyApplierInfo =
{
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    5 * 60,
    1
};

static DWORD gdwUserPolicyRefreshInterval = 30 * 60;
static LoopbackProcessingMode glpmUserPolicyMode = LOOPBACK_PROCESSING_MODE_USER_ONLY;

extern GPOCSEINFO gCSEInfo;

static pthread_t gUserPolicyThread;
static PVOID     pgUserPolicyThread = NULL;

static pthread_t gUserPollerThread;
static PVOID     pgUserPollerThread = NULL;

static PGPUSERCONTEXT gCurrentUsers = NULL;
static pthread_mutex_t gCurrentUsersLock = PTHREAD_MUTEX_INITIALIZER;

#define LOG_ERROR_AND_WAIT(facility, code, message)                     \
    do {                                                                \
        if (code) {                                                     \
            GPA_LOG_ERROR("Error in %s (%s), error: [0x%8X](%s)", facility, message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto gpo_wait;                                              \
        }                                                               \
    }                                                                   \
    while (0)
    
#define LOG_VERBOSE_AND_WAIT(facility, code, message)                   \
    do {                                                                \
        if (code) {                                                     \
            GPA_LOG_VERBOSE("Error in %s (%s), error: [0x%8X](%s)", facility, message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto gpo_wait;                                              \
        }                                                               \
    }                                                                   \
    while (0)

#define LOG_ERROR_AND_EXIT(facility, code, message)                     \
    do {                                                                \
        if (code) {                                                     \
            GPA_LOG_ERROR("Error in %s (%s), error: [0x%8X](%s)", facility, message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto error;                                                 \
        }                                                               \
    }                                                                   \
    while (0)
    
#define LOG_VERBOSE_AND_CLEANUP(facility, code, message)                \
    do {                                                                \
        if (code) {                                                     \
            GPA_LOG_VERBOSE("Error in %s (%s), error: [0x%8X](%s)", facility, message, code, CTErrorName(code) != NULL ? CTErrorName(code) : "Unknown"); \
            goto cleanup;                                               \
        }                                                               \
    }                                                                   \
    while (0)

static
void
GPAAddUserContext_Unsafe(
     PGPUSERCONTEXT* ppList,
     PGPUSERCONTEXT  pContext
     )
{
     pContext->pNext = *ppList;
     *ppList = pContext;
}

static
BOOLEAN
GPALookupUserContext_Unsafe(
    PGPUSERCONTEXT  pContextList,
    uid_t uid,
    PGPUSERCONTEXT* ppContext
    )
{
    BOOLEAN bFound = FALSE;
    PGPUSERCONTEXT pContext = NULL;

    *ppContext = NULL;
    for (pContext = pContextList; pContext; pContext = pContext->pNext)
    {
        if (pContext->pUserInfo->uid == uid) {
           bFound = TRUE;
           *ppContext = pContext;
           break;
        }
    }

    return bFound;
}


static
CENTERROR
ProcessUserGroupPolicyList(
    PGROUP_POLICY_OBJECT pGPOCurrentList,
    PGROUP_POLICY_OBJECT pGPOExistingList,
    PGPUSER pUserInfo
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

    ceError = GPAComputeModDelGroupPolicyList(
        pGPOCurrentList,
        pGPOExistingList,
        &pGPOModifiedList,
        &pGPODeletedList
        );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pGPCSEList = gCSEInfo.pGPCSEList;
    while (pGPCSEList) {

        if (pGPCSEList->dwNoUserPolicy) {
            pGPCSEList = pGPCSEList->pNext;
            continue;
        }

        ceError = GPAComputeCSEModDelList(
            USER_GROUP_POLICY,
            pGPCSEList->pszGUID,
            pGPOModifiedList,
            pGPODeletedList,
            &pGPOCSEModList,
            &pGPOCSEDelList
            );
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = pGPCSEList->pfnProcessGroupPolicy(
            dwFlags,
            pUserInfo,
            pGPOCSEModList,
            pGPOCSEDelList
            );

        GetEnableEventLog(&bEnableEventLog);
        if(bEnableEventLog) {

            GPAPostCSEEvent(ceError,
                            pUserInfo,
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

        pGPCSEList = pGPCSEList->pNext;}

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

static
CENTERROR
GetDomainAndUserInfo(
        PCSTR pszUsername,
        PSTR* ppszPlainUsername,
        PSTR* ppszMachineName,
        PSTR* ppszMachineDomain,
        PSTR* ppszUserDomain
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    size_t idx = 0;
    PSTR pszIndex = NULL;
    PSTR pszUserDomain = NULL;
    PSTR pszMachineDomain = NULL;
    PSTR pszPlainUsername = NULL;
    PSTR pszMachineName = NULL;
    
    if (IsNullOrEmptyString(pszUsername)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    if ((pszIndex = index(pszUsername, '\\')) != NULL) {
        idx = pszIndex-pszUsername;
        ceError = LwStrndup(pszUsername, idx, &pszUserDomain);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
        if (!IsNullOrEmptyString(pszUsername+idx+1)) {
            ceError = LwAllocateString(pszUsername+idx+1, &pszPlainUsername);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    else if ((pszIndex = index(pszUsername, '@')) != NULL) {
        idx = pszIndex-pszUsername;
        ceError = LwStrndup(pszUsername, idx, &pszPlainUsername);
        BAIL_ON_CENTERIS_ERROR(ceError);
            
        if (!IsNullOrEmptyString(pszUsername+idx+1)) {
            ceError = LwAllocateString(pszUsername+idx+1, &pszUserDomain);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    else {
        
        ceError = LwAllocateString(pszUsername, &pszPlainUsername);
        BAIL_ON_CENTERIS_ERROR(ceError);
        
    }
    
    ceError = GPAGetDnsSystemNames(NULL, &pszMachineName, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOGetADDomain(&pszMachineDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (IsNullOrEmptyString(pszUserDomain)) {
       ceError = LwAllocateString(pszMachineDomain, &pszUserDomain);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    *ppszPlainUsername = pszPlainUsername;
    pszPlainUsername = NULL;
    
    *ppszMachineDomain = pszMachineDomain;
    pszMachineDomain = NULL;
    
    *ppszUserDomain = pszUserDomain;
    pszUserDomain = NULL;

    *ppszMachineName = pszMachineName;
    pszMachineName = NULL;
    
    error:
    
    LW_SAFE_FREE_STRING(pszPlainUsername);
    LW_SAFE_FREE_STRING(pszMachineDomain);
    LW_SAFE_FREE_STRING(pszUserDomain);
    LW_SAFE_FREE_STRING(pszMachineName);
    
    return ceError;
}

static
CENTERROR
GetUserCachePath(
    uid_t uid,
    PSTR * ppszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszCachePath = NULL;

    ceError = LwAllocateStringPrintf(&pszCachePath,
                                     "FILE:/tmp/krb5cc_%ld",
                                     (long)uid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszCachePath = pszCachePath;

cleanup:

    return ceError;

error:

    *ppszCachePath = NULL;

    goto cleanup;
}

static
CENTERROR
GetSystemCachePath(
    PSTR*         ppszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR  pszCachePath = NULL;

    ceError = LwAllocateStringPrintf(&pszCachePath,
                                     "FILE:%s/krb5cc_gpagentd",
                                     CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszCachePath = pszCachePath;

cleanup:

    return ceError;

error:

    *ppszCachePath = NULL;

    goto cleanup;
}

static
CENTERROR
SetDefaultCachePath(
    PSTR  pszCachePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwMajorStatus = 0;
    DWORD dwMinorStatus = 0;

    dwMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&dwMinorStatus,
                            pszCachePath,
                            NULL);
    if (dwMajorStatus)
    {
        GPA_LOG_ERROR("User group policy processing was unable to change the default Kerberos cred cache to (path: %s), gss error (dwMajorStatus: %d, dwMinorStatus: %d)", pszCachePath, dwMajorStatus, dwMinorStatus);
        ceError = CENTERROR_GP_GSS_CALL_FAILED;                 \
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

cleanup:

    return ceError;

error:

    goto cleanup;
}

static
CENTERROR
CacheUserAttributes(
    uid_t uid,
    PGPUSER_AD_ATTRS pUserADAttrs
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFileDir = NULL;
    PSTR pszFilePath = NULL;
    PGPACFGSECTION pUserSettingsList = NULL;
    PGPACFGSECTION pADSection = NULL;
    BOOLEAN bDirExists = FALSE;

    GPA_LOG_INFO("Saving user attributes to user logon cache [uid: %ld, display name: %s]",
                 (long)uid,
                 pUserADAttrs->pszDisplayName ? pUserADAttrs->pszDisplayName : "<null>");

    ceError = LwAllocateStringPrintf(&pszFileDir, "%s/user-cache/%ld", CACHEDIR, (long) uid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf(&pszFilePath, "%s/user-cache/%ld/ad-user-attrs", CACHEDIR, (long) uid);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    ceError = GPACheckDirectoryExists(pszFileDir, &bDirExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bDirExists == FALSE)
    {                           
        ceError = LwCreateDirectory(pszFileDir, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPACreateConfigSection(&pUserSettingsList,
                                    &pADSection,
                                    "User AD Name Attributes");
    BAIL_ON_CENTERIS_ERROR(ceError);
                                
    if (pUserADAttrs->pszDisplayName)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Name Attributes",
                                                "displayName",
                                                pUserADAttrs->pszDisplayName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszFirstName)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Name Attributes",
                                                "givenName",
                                                pUserADAttrs->pszFirstName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszLastName)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Name Attributes",
                                                "sn",
                                                pUserADAttrs->pszLastName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszADDomain)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Name Attributes",
                                                "userDomain",
                                                pUserADAttrs->pszADDomain);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszKerberosPrincipal)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Name Attributes",
                                                "userPrincipalName",
                                                pUserADAttrs->pszKerberosPrincipal);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszEMailAddress)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Email Attributes",
                                                "mail",
                                                pUserADAttrs->pszEMailAddress);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszMSExchHomeServerName)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Email Attributes",
                                                "msExchHomeServerName",
                                                pUserADAttrs->pszMSExchHomeServerName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszMSExchHomeMDB)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Email Attributes",
                                                "homeMDB",
                                                pUserADAttrs->pszMSExchHomeMDB);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszTelephoneNumber)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Phone Attributes",
                                                "telephoneNumber",
                                                pUserADAttrs->pszTelephoneNumber);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszFaxTelephoneNumber)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Phone Attributes",
                                                "facsimileTelephoneNumber",
                                                pUserADAttrs->pszFaxTelephoneNumber);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszMobileTelephoneNumber)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Phone Attributes",
                                                "mobile",
                                                pUserADAttrs->pszMobileTelephoneNumber);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszStreetAddress)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "streetAddress",
                                                pUserADAttrs->pszStreetAddress);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszPostOfficeBox)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "postOfficeBox",
                                                pUserADAttrs->pszPostOfficeBox);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszCity)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "l",
                                                pUserADAttrs->pszCity);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszState)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "st",
                                                pUserADAttrs->pszState);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszPostalCode)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "postalCode",
                                                pUserADAttrs->pszPostalCode);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszCountry)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Address Attributes",
                                                "co",
                                                pUserADAttrs->pszCountry);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszTitle)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Work Attributes",
                                                "title",
                                                pUserADAttrs->pszTitle);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszCompany)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Work Attributes",
                                                "company",
                                                pUserADAttrs->pszCompany);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszDepartment)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Work Attributes",
                                                "department",
                                                pUserADAttrs->pszDepartment);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszHomeDirectory)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "homeDirectory",
                                                pUserADAttrs->pszHomeDirectory);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszHomeDrive)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "homeDrive",
                                                pUserADAttrs->pszHomeDrive);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszPasswordLastSet)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "pwdLastSet",
                                                pUserADAttrs->pszPasswordLastSet);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
                                
    if (pUserADAttrs->pszUserAccountControl)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "userAccountControl",
                                                pUserADAttrs->pszUserAccountControl);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "maxPwdAge",
                                                pUserADAttrs->pszMaxMinutesUntilChangePassword);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    if (pUserADAttrs->pszMinMinutesUntilChangePassword)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "minPwdAge",
                                                pUserADAttrs->pszMinMinutesUntilChangePassword);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    if (pUserADAttrs->pszMaxFailedLoginAttempts)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "lockoutThreshhold",
                                                pUserADAttrs->pszMaxFailedLoginAttempts);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pUserADAttrs->pszAllowedPasswordHistory)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "pwdHistoryLength",
                                                pUserADAttrs->pszAllowedPasswordHistory);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pUserADAttrs->pszMinCharsAllowedInPassword)
    {
        ceError = GPASetConfigValueBySectionName(pUserSettingsList,
                                                "User AD Network Settings Attributes",
                                                "minPwdLength",
                                                pUserADAttrs->pszMinCharsAllowedInPassword);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPASaveConfigSectionList( pszFilePath,
                                       pUserSettingsList);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    LW_SAFE_FREE_STRING(pszFileDir);

    GPAFreeConfigSectionList(pUserSettingsList);
    pUserSettingsList = NULL;

    return ceError;
}

static
PVOID
ApplyPolicyForUser(
    PVOID data
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszUserOUDN = NULL;
    PSTR pszComputerOUDN = NULL;
    PSTR pszMachineDomain = NULL;
    PSTR pszUserDomain = NULL;
    PSTR pszUserDCName = NULL;
    PSTR pszComputerDCName = NULL;
    PSTR pszPlainUserName = NULL;
    PSTR pszMachineName = NULL;
    PSTR pszUserCachePath = NULL;
    PSTR pszSystemCachePath = NULL;
    PGPUSER_AD_ATTRS pUserADAttrs = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pCurrentGPOList = NULL;
    PGROUP_POLICY_OBJECT pCurrentComputerGPOList = NULL;
    PGPUSERCONTEXT pUserContext = (PGPUSERCONTEXT)data;
    BOOLEAN bDirExists = FALSE;
    
    GPA_LOG_LOCK("Take pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&pUserContext->lock);
    GPA_LOG_LOCK("Got pContext->lock at %s:%d",__FILE__,__LINE__);

    if (UserPoliciesAreAvailable())
    {
       GPA_LOG_INFO("Begin applying policy for user [name: %s, uid:%ld]",
                    pUserContext->pUserInfo->pszUserPrincipalName ? pUserContext->pUserInfo->pszUserPrincipalName : "<null string>",
                    (long)pUserContext->pUserInfo->uid);

       if (pUserContext->pUserInfo->pszHomeDir)
       {
           ceError = GPACheckDirectoryExists(pUserContext->pUserInfo->pszHomeDir, &bDirExists);
           LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error finding valid homedir directory for user");

           if (bDirExists == FALSE)
           {
               GPA_LOG_WARNING("User group policy processing did not find a home directory created for the user (name: %s, UPN:%s, HomeDir:%s). Skipping GPO processing for this user this time, will try again later",
                pUserContext->pUserInfo->pszName ?
                pUserContext->pUserInfo->pszName :
                "<null string>",
                pUserContext->pUserInfo->pszUserPrincipalName ?
                pUserContext->pUserInfo->pszUserPrincipalName :
                "<null string>",
                pUserContext->pUserInfo->pszHomeDir ?
                pUserContext->pUserInfo->pszHomeDir :
                "<null string>");

                // Skip this user's GPO policy processing, as the user profile looks to be not setup yet.
                ceError = CENTERROR_GP_LOGIN_POLICY_FAILED;
                LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error encountered while accessing user profile settings");
           }
       }
    
       pUserContext->policyStatus = GP_USER_POLICY_PROCESSING;
    
       ceError = GetDomainAndUserInfo(pUserContext->pUserInfo->pszUserPrincipalName, 
                                      &pszPlainUserName, 
                                      &pszMachineName, 
                                      &pszMachineDomain,
                                      &pszUserDomain
                                      );
       LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while determining domain and information for user");

       GPA_LOG_VERBOSE("Found user information: (plain user name: %s, hostname: %s, machine domain: %s, user domain: %s)",
                    pszPlainUserName ? pszPlainUserName : "<null string>",
                    pszMachineName ? pszMachineName : "<null string>",
                    pszMachineDomain ? pszMachineDomain : "<null string>",
                    pszUserDomain ? pszUserDomain : "<null string>");
    
       if (!strcasecmp(pszMachineDomain, pszUserDomain)) {
           ceError = GPAGetPreferredDomainController( pszMachineDomain, &pszUserDCName );
           LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while determining domain controller name for user and computer domain");

           ceError = LwAllocateString(pszUserDCName, &pszComputerDCName);
           BAIL_ON_CENTERIS_ERROR(ceError);
       } else {
           /* Machine and User domains are different */ 
           if (!strchr(pszUserDomain, '.')) {
              GPA_LOG_ERROR("User group policy processing was unable to determine the FQDN for the user (name: %s, UPN:%s)",
                 pUserContext->pUserInfo->pszName ?
                 pUserContext->pUserInfo->pszName :
                 "<null string>",
                 pUserContext->pUserInfo->pszUserPrincipalName ?
                 pUserContext->pUserInfo->pszUserPrincipalName :
                 "<null string>");
           }
        
           ceError = GPAGetPreferredDomainController(pszUserDomain, &pszUserDCName);
           LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while determining domain controller name for user domain");
        
           ceError = GPAGetPreferredDomainController(pszMachineDomain, &pszComputerDCName);
           LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while determining domain controller name for computer domain");
       }
    
       GPA_LOG_VERBOSE("Connecting to DC for user policy processing: %s",
                    pszUserDCName ? pszUserDCName : "<null string>");

       /* Switch credentials to impersonate the user */
       ceError = GetUserCachePath(pUserContext->pUserInfo->uid, &pszUserCachePath);
       LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while determining user credentials cache path (user may have a stale logon session)");

       ceError = SetDefaultCachePath(pszUserCachePath);
       LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while setting user credentials");

       ceError = GPOOpenDirectory(pszUserDomain, pszUserDCName, &hDirectory);
       LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while contacting domain controller for user domain");
    
       ceError = GPAFindUserDN(hDirectory,
                               pUserContext->pUserInfo->pszSID,
                               pszUserDomain,
                               &pszUserOUDN);
       LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while looking up user in domain");

       ceError = GPAGetUserAttributes(hDirectory,
                                      pUserContext->pUserInfo->pszSID,
                                      pszUserDomain,
                                      &pUserADAttrs);
       LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while reading user AD attributes from domain DC");

       ceError = CacheUserAttributes(pUserContext->pUserInfo->uid,
                                     pUserADAttrs);
       LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while saving user AD attributes to cache");
    
#if defined(__LWI_DARWIN__)
       ceError = FlushDirectoryServiceCache();
       if (!CENTERROR_IS_OK(ceError)) {
           GPA_LOG_ALWAYS("Failed to flush the Mac DirectoryService cache. Error: [%d] (%s)",
                          ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
       }
#endif
    
       GPA_LOG_VERBOSE("User DN: %s", pszUserOUDN);
    
       ceError = GPAGetGPOList(pUserContext->pUserInfo,
                               hDirectory,
                               pszUserOUDN,
                               &pCurrentGPOList);
       LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while reading GPOs for user from domain DC");

       if (hDirectory) {
          GPOCloseDirectory(hDirectory);
          hDirectory = (HANDLE)NULL;
       }

       if (glpmUserPolicyMode != LOOPBACK_PROCESSING_MODE_USER_ONLY) {

           /* We need to also determine the User policies that apply to the DN of the Computer */

           ceError = GPOOpenDirectory(pszMachineDomain, pszComputerDCName, &hDirectory);
           LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while contacting domain controller for computer domain");

           ceError = GPAFindComputerDN(hDirectory,
                                       pszMachineName,
                                       pszMachineDomain,
                                       &pszComputerOUDN);
           LOG_ERROR_AND_EXIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while looking up computer in domain");
    
           GPA_LOG_VERBOSE("Computer DN: %s", pszComputerOUDN);
    
           ceError = GPAGetGPOList(pUserContext->pUserInfo,
                                   hDirectory,
                                   pszComputerOUDN,
                                   &pCurrentComputerGPOList);
           LOG_VERBOSE_AND_CLEANUP(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error while reading GPOs for user from domain DC");

           if (glpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_REPLACE_USER) {

               /* Need to delete the user's GPOs and just use pCurrentComputerGPOList */
               GPA_LOG_VERBOSE("User loopback processing mode is to skip user's GPOs and replacing with computer's GPOs");
               GPA_SAFE_FREE_GPO_LIST(pCurrentGPOList);
               pCurrentGPOList = pCurrentComputerGPOList;
               pCurrentComputerGPOList = NULL;

           } else if (glpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_MERGED) {
               /* Need to add the GPOs from pCurrentComputerGPOList to the end of the user's GPOs */
               GPA_LOG_VERBOSE("User loopback processing mode is to merge the user's GPOs with the computer's GPOs");
               ceError = GPAPrependGPList(pCurrentComputerGPOList,
                                          pCurrentGPOList,
                                          &pCurrentGPOList);
               BAIL_ON_CENTERIS_ERROR(ceError);
               pCurrentComputerGPOList = NULL;
           } else {
               GPA_LOG_ALWAYS("Invalid user policy loopback processing mode, defaulting to user only");
           }
       }
       else {
           GPA_LOG_VERBOSE("User loopback processing mode is to use only the user's GPOs");
       }
    
       ceError = ProcessUserGroupPolicyList(pCurrentGPOList,
                                            pUserContext->pGPOList,
                                            pUserContext->pUserInfo);
       if (!CENTERROR_IS_OK(ceError)) {
           GPA_SAFE_FREE_GPO_LIST(pCurrentGPOList);
       }
    
       GPA_SAFE_FREE_GPO_LIST(pUserContext->pGPOList);
    
       pUserContext->pGPOList = pCurrentGPOList;
       pCurrentGPOList = NULL;
    }

    pUserContext->lastRefreshTime = time(NULL);
    GPA_LOG_INFO("Completed refreshing group policies for user:%s [uid:%ld] time:%d",
                    SAFE_PRINT_STRING(pUserContext->pUserInfo->pszName),
                    (long)pUserContext->pUserInfo->uid,
                    pUserContext->lastRefreshTime);
    
cleanup:

    /* Revert thread back to machine creds */
    if (GetSystemCachePath(&pszSystemCachePath) == CENTERROR_SUCCESS)
    {
        SetDefaultCachePath(pszSystemCachePath);
    }
    
    GPA_SAFE_FREE_GPO_LIST(pCurrentGPOList);
    GPA_SAFE_FREE_GPO_LIST(pCurrentComputerGPOList);
    
    if (hDirectory) {
       GPOCloseDirectory(hDirectory);
    }
    
    LW_SAFE_FREE_STRING(pszUserCachePath);
    LW_SAFE_FREE_STRING(pszSystemCachePath);
    LW_SAFE_FREE_STRING(pszUserOUDN);
    LW_SAFE_FREE_STRING(pszComputerOUDN);
    LW_SAFE_FREE_STRING(pszMachineDomain);
    LW_SAFE_FREE_STRING(pszMachineName);
    LW_SAFE_FREE_STRING(pszUserDomain);
    LW_SAFE_FREE_STRING(pszUserDCName);
    LW_SAFE_FREE_STRING(pszComputerDCName);
    LW_SAFE_FREE_STRING(pszPlainUserName);

    GPAFreeUserAttributes(pUserADAttrs);
    
    pUserContext->policyStatus = (CENTERROR_IS_OK(ceError) ? GP_USER_POLICY_APPLIED : GP_USER_POLICY_FAILED);
    GPA_LOG_VERBOSE("Marking user context status:%s %d",
                    SAFE_PRINT_STRING(pUserContext->pUserInfo->pszName),
                    pUserContext->policyStatus);
    
    // Notify any threads waiting on this user policy having
    // finished applying.
    pthread_cond_broadcast(&pUserContext->cond);
    
    GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&pUserContext->lock);
    GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);
    
    return NULL;

error:

    GPA_LOG_ERROR("Failed to apply policy for user [uid:%ld]", (long)pUserContext->pUserInfo->uid);

    goto cleanup;
}

static
BOOLEAN
ShouldApplyPolicyForUser(
    PGPUSERCONTEXT pUserContext,
    DWORD  dwPolicyRefreshInterval,
    time_t referenceTime
    )
{
    BOOLEAN bRefreshPolicy = FALSE;

    GPA_LOG_LOCK("Take pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&pUserContext->lock);
    GPA_LOG_LOCK("Got pContext->lock at %s:%d",__FILE__,__LINE__);

    GPA_LOG_VERBOSE("Do User Policy processing?");
    GPA_LOG_VERBOSE("Name: %s", SAFE_PRINT_STRING(pUserContext->pUserInfo->pszName));
    GPA_LOG_VERBOSE("policyStatus: %d", pUserContext->policyStatus);
    GPA_LOG_VERBOSE("last refresh time: %d", pUserContext->lastRefreshTime);
    GPA_LOG_VERBOSE("reference time: %d", referenceTime);
    GPA_LOG_VERBOSE("refresh interval: %d", dwPolicyRefreshInterval);

    if ((pUserContext->policyStatus != GP_USER_POLICY_APPLIED) ||
        (difftime(referenceTime,
                  pUserContext->lastRefreshTime) > dwPolicyRefreshInterval)) {
        bRefreshPolicy = TRUE;
    }

    GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&pUserContext->lock);
    GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);

    GPA_LOG_VERBOSE("Returning: %s", bRefreshPolicy ? "TRUE" : "FALSE");
    return bRefreshPolicy;
}

static
CENTERROR
ApplyUserPolicies()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pContext = NULL;
    BOOLEAN bReleaseLock = FALSE;
    pthread_t* pUserThreads = NULL;
    PGPUSERCONTEXT *ppContexts = NULL;
    int iCtx = 0, nCtx = 0;
    time_t curTime = 0;

    GPA_LOG_VERBOSE("Begin checking User policies...");

    GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&gCurrentUsersLock);
    bReleaseLock = TRUE;
    GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    curTime = time(NULL);
    for (pContext = gCurrentUsers; pContext != NULL; pContext = pContext->pNext)
    {
        if (ShouldApplyPolicyForUser(pContext,
                                     gdwUserPolicyRefreshInterval,
                                     curTime))
        {
           nCtx++;
        }
    }
   
    if (nCtx) {
       int nActiveContexts = 0;

       GPA_LOG_VERBOSE("Begin applying policies for [%d] users.", nCtx);

       ceError = LwAllocateMemory(sizeof(pthread_t) * nCtx,
                                  (PVOID*)&pUserThreads);
       BAIL_ON_CENTERIS_ERROR(ceError);

       ceError = LwAllocateMemory(sizeof(PGPUSERCONTEXT) * nCtx,
                                  (PVOID*)&ppContexts);
       BAIL_ON_CENTERIS_ERROR(ceError);

       iCtx = 0;
       for (pContext = gCurrentUsers; pContext != NULL; pContext = pContext->pNext)
       {
           if (ShouldApplyPolicyForUser(pContext,
                                        gdwUserPolicyRefreshInterval,
                                        curTime))
           {
              *(ppContexts+iCtx) = pContext;
              if (pthread_create(pUserThreads+iCtx,
                                 NULL,
                                 ApplyPolicyForUser,
                                 pContext))
              {
                 ceError = LwMapErrnoToLwError(errno);
                 goto thread_launch_error;
              }
              iCtx++;
              nActiveContexts++;
           }
       }

thread_launch_error:

       GPA_LOG_VERBOSE("Waiting for policies for [%d] users.", nActiveContexts);

       for (iCtx = 0; iCtx < nActiveContexts; iCtx++) {
           pthread_join(*(pUserThreads+iCtx), NULL);
       }

       GPA_LOG_VERBOSE("Completed applying policies for [%d] users.", nActiveContexts);

       /* If any threads are waiting for the processing, unblock them */
       for (iCtx = nActiveContexts; iCtx < nCtx; iCtx++) {
           PGPUSERCONTEXT pContext = *(ppContexts+iCtx);
           GPA_LOG_LOCK("Take pContext->lock at %s:%d",__FILE__,__LINE__);
           pthread_mutex_lock(&pContext->lock);
           GPA_LOG_LOCK("Got pContext->lock at %s:%d",__FILE__,__LINE__);
           pContext->policyStatus = GP_USER_POLICY_FAILED;
           pthread_cond_broadcast(&pContext->cond);
           GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
           pthread_mutex_unlock(&pContext->lock);
           GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);
       }
    }

    GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&gCurrentUsersLock);
    bReleaseLock = FALSE;
    GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    GPA_LOG_VERBOSE("Completed checking user policies...");

cleanup:
    
    if (bReleaseLock) {
        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    }

    if (pUserThreads)
       LwFreeMemory(pUserThreads);

    if (ppContexts)
       LwFreeMemory(ppContexts);

    return ceError;

error:

    GPA_LOG_ERROR("Error checking user policies");

    goto cleanup;
}

static
CENTERROR
InitCurrentUsersTable()
{
    GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&gCurrentUsersLock);
    GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    GPA_SAFE_FREE_USER_CONTEXT_LIST(gCurrentUsers);
    
    GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&gCurrentUsersLock);
    GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    return CENTERROR_SUCCESS;
}

static
void
FreeCurrentUsersTable()
{
    GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&gCurrentUsersLock);
    GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    GPA_SAFE_FREE_USER_CONTEXT_LIST(gCurrentUsers);

    GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&gCurrentUsersLock);
    GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
}

static
PVOID
GPAUserPollerLoop()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pCurrentUserContextList = NULL;
    PGPUSERCONTEXT pLoggedOutContextList = NULL;
    long uid = 0;

    PGPUSERCONTEXT pContext = NULL;
    struct timespec timeout = {0};
    BOOLEAN bReleaseLock = FALSE;
    BOOLEAN bChangesExist = FALSE;
    PSTR pszDistroName = NULL;
    PSTR pszDistroVersion = NULL;

    ceError = GetDistroName(&pszDistroName);
    if (ceError)
    {
        GPA_LOG_ERROR("Start of user poller loop failed to determine Operating System name, error: %d", ceError);
        ceError = CENTERROR_SUCCESS;
    }
    
    ceError = GetDistroVersion(&pszDistroVersion);
    if (ceError)
    {
        GPA_LOG_ERROR("Start of user poller loop failed to determine Operating System version, error: %d", ceError);
        ceError = CENTERROR_SUCCESS;
    }

    GPA_LOG_INFO("Group Policy (User) Poller started");

    pthread_mutex_lock(&gUserPollerInfo.lock);

    while (gUserPollerInfo.dwRefresh) {

        PGPUSERCONTEXT pPrevContext = NULL;
        PGPUSERCONTEXT pCandidate = NULL;

        gUserPollerInfo.dwRefresh = 0;
        
        GPA_SAFE_FREE_USER_CONTEXT_LIST(pCurrentUserContextList);
        GPA_SAFE_FREE_USER_CONTEXT_LIST(pLoggedOutContextList);
        
        ceError = GPAGetCurrentADUsers(pszDistroName, pszDistroVersion, &pCurrentUserContextList);
        if (ceError == CENTERROR_GP_NOT_AD_USER)
        {
            goto gpo_wait;
        }
        LOG_VERBOSE_AND_WAIT(FACILITY_USER_POLLER, ceError, "Error retrieving currently logged in user list");
        
        GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_lock(&gCurrentUsersLock);
        bReleaseLock = TRUE;
        GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        
        // This will comprise of the users who have completely logged off i.e.
        // not a single login session of this user must exist.
        pLoggedOutContextList = NULL;
        pContext = gCurrentUsers;
        pPrevContext = NULL;
        while (pContext)
        {
            GPA_LOG_LOCK("Take pContext->lock at %s:%d",__FILE__,__LINE__);
            pthread_mutex_lock(&pContext->lock);
            GPA_LOG_LOCK("Got pContext->lock at %s:%d",__FILE__,__LINE__);

            if (!GPALookupUserContext_Unsafe(pCurrentUserContextList,
                                             pContext->pUserInfo->uid,
                                             &pCandidate))
            {
               PGPUSERCONTEXT pTmpContext = NULL;

               //
               // User seems to have left, check to see if we are we are still in the middle of processing
               //
               if (pContext->policyStatus == GP_USER_POLICY_NEW ||
                   pContext->policyStatus == GP_USER_POLICY_PROCESSING)
               {
                   // Skip this context, we'll remove it next time when it is no longer active.
                   GPA_LOG_VERBOSE("The user [uid=%ld] logged out, but policy processing has not finished, context will be removed later",
                                   (long)pContext->pUserInfo->uid);
                   GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
                   pthread_mutex_unlock(&pContext->lock);
                   GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);
                   pPrevContext = pContext;
                   pContext = pContext->pNext;
               }
               else
               {
                   // This user has logged out?
                   uid = pContext->pUserInfo->uid;
                   GPA_LOG_VERBOSE("Adding the context [uid=%ld] to logged out list", (long)uid);
                   if (pPrevContext) {
                      pPrevContext->pNext = pContext->pNext;
                   }
                   else {
                      gCurrentUsers = pContext->pNext;
                   }
                   pTmpContext = pContext->pNext;

                   GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
                   pthread_mutex_unlock(&pContext->lock);
                   GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);

                   // Attach it to the logged out context list
                   pContext->pNext = NULL;
                   pContext->pNext = pLoggedOutContextList;
                   pLoggedOutContextList = pContext;

                   pContext = pTmpContext;
               }
            }
            else
            {
                GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
                pthread_mutex_unlock(&pContext->lock);
                GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);
                pPrevContext = pContext;
                pContext = pContext->pNext;
            }
        }
        
        // Find the users in the current list that are not yet in the global list
        bChangesExist = FALSE;
        pPrevContext = NULL;
        pContext = pCurrentUserContextList;
        while (pContext)
        {
            if (!GPALookupUserContext_Unsafe(gCurrentUsers,
                                             pContext->pUserInfo->uid,
                                             &pCandidate))
            {
               PGPUSERCONTEXT pTmpContext = NULL;

               // This user is new
               if (pPrevContext)  {
                  pPrevContext->pNext = pContext->pNext;
               } else {
                  pCurrentUserContextList = pContext->pNext;
               }
               pTmpContext = pContext->pNext;
               pContext->pNext = NULL;
               GPAAddUserContext_Unsafe(&gCurrentUsers, pContext);
               pContext = pTmpContext;

               bChangesExist = TRUE;
            }
            else
            {
               pPrevContext = pContext;
               pContext = pContext->pNext;
            }
        }
        
        GPA_SAFE_FREE_USER_CONTEXT_LIST(pCurrentUserContextList);
        GPA_SAFE_FREE_USER_CONTEXT_LIST(pLoggedOutContextList);

        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        bReleaseLock = FALSE;
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        
        if (bChangesExist)
        {
            GPARefreshUserPolicies();
        }
        
    gpo_wait:

        timeout.tv_sec = time(NULL) + gUserPollerInfo.dwPollingInterval;
        timeout.tv_nsec = 0;

        ceError = pthread_cond_timedwait(&gUserPollerInfo.pollCondition,
                                         &gUserPollerInfo.lock,
                                         &timeout);
        if (ProcessShouldExit())
           break;

        if (ceError == ETIMEDOUT)
           gUserPollerInfo.dwRefresh = 1;
    }
    
    pthread_mutex_unlock(&gUserPollerInfo.lock);
    
    if (bReleaseLock)
    {
        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    }
    
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pCurrentUserContextList);
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pLoggedOutContextList);
    LW_SAFE_FREE_STRING(pszDistroName);
    LW_SAFE_FREE_STRING(pszDistroVersion);

    GPA_LOG_INFO("Group Policy (User) Poller stopped [Error code:0x%8X (%s)]", ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
    
    return NULL;
}

static
PVOID
GPAUserPolicyLoop()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct timespec timeout;

    GPA_LOG_INFO("Group Policy (User) Driver started");

    ceError = InitCurrentUsersTable();
    BAIL_ON_CENTERIS_ERROR(ceError);

    pthread_mutex_lock(&gUserPolicyApplierInfo.lock);

    while (gUserPolicyApplierInfo.dwRefresh) {

        GPA_LOG_INFO("Started refreshing user group policies...");

        ceError = GetUserPollingInterval(&gdwUserPolicyRefreshInterval);
        LOG_VERBOSE_AND_WAIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error retrieving user policy refresh interval");

        GPA_LOG_VERBOSE("User policy refresh interval: [%d] seconds",
                    gdwUserPolicyRefreshInterval);

        ceError = GetLoopbackProcessingMode(&glpmUserPolicyMode);
        LOG_VERBOSE_AND_WAIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error retrieving loopback processing user policy mode");

        GPA_LOG_VERBOSE("Loopback processing user policy mode: [%s]",
            glpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_USER_ONLY ? 
                "User GPOs only" :
            glpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_REPLACE_USER ?
                "User GPOs are replaced with Computer GPOs" :
            glpmUserPolicyMode == LOOPBACK_PROCESSING_MODE_MERGED ?
                "User GPOs are merged with Computer GPOs" :
            "Invalid");

        gUserPolicyApplierInfo.dwRefresh = 0;

        ceError = ApplyUserPolicies();
        LOG_ERROR_AND_WAIT(FACILITY_USER_POLICY_APPLICATOR, ceError, "Error applying user policies");
        
        GPA_LOG_INFO("Successfully completed refreshing user group policies...");

      gpo_wait:

        GPA_LOG_VERBOSE("Waiting for next refresh of user group policies...");
        
        timeout.tv_sec = time(NULL) + gUserPolicyApplierInfo.dwPollingInterval;
        timeout.tv_nsec = 0;

        ceError = pthread_cond_timedwait(&gUserPolicyApplierInfo.pollCondition,
                                         &gUserPolicyApplierInfo.lock,
                                         &timeout);
        if (ProcessShouldExit())
            break;

        if (ceError == ETIMEDOUT)
            gUserPolicyApplierInfo.dwRefresh = 1;
    }

    pthread_mutex_unlock(&gUserPolicyApplierInfo.lock);

  error:

    FreeCurrentUsersTable();

    GPA_LOG_INFO("Group Policy (User) Driver stopped [Error code:0x%8X (%s)]", ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");

    return NULL;
}

static
CENTERROR
GPARefreshUserPoller()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    pthread_mutex_lock(&gUserPollerInfo.lock);

    gUserPollerInfo.dwRefresh = 1;

    ceError = pthread_cond_signal(&gUserPollerInfo.pollCondition);

    pthread_mutex_unlock(&gUserPollerInfo.lock);

    return ceError;
}

static
CENTERROR
GPAStartUserPollerThread()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pgUserPollerThread = NULL;

    ceError = pthread_create(&gUserPollerThread,
                             NULL,
                             GPAUserPollerLoop,
                             NULL
                             );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pgUserPollerThread = &gUserPollerThread;

error:

    return (ceError);
}

static
CENTERROR
GPAStopUserPollerThread()
{
    if (pgUserPollerThread) {
        GPARefreshUserPoller();
        pthread_join(gUserPollerThread, NULL);
    }

    return CENTERROR_SUCCESS;
}

CENTERROR
GPAStartUserPolicyThread()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pgUserPolicyThread = NULL;

    ceError = pthread_create(&gUserPolicyThread,
                             NULL,
                             GPAUserPolicyLoop,
                             NULL
                             );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pgUserPolicyThread = &gUserPolicyThread;
    
    ceError = GPAStartUserPollerThread();
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return (ceError);
}

CENTERROR
GPAStopUserPolicyThread()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    
    ceError = GPAStopUserPollerThread();
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    if (pgUserPolicyThread) {
        GPARefreshUserPolicies();
        pthread_join(gUserPolicyThread, NULL);
    }
    
error:

    return ceError;
}

CENTERROR
GPARefreshUserPolicies()
{

    CENTERROR ceError = CENTERROR_SUCCESS;

    pthread_mutex_lock(&gUserPolicyApplierInfo.lock);

    gUserPolicyApplierInfo.dwRefresh = 1;

    ceError = pthread_cond_signal(&gUserPolicyApplierInfo.pollCondition);

    pthread_mutex_unlock(&gUserPolicyApplierInfo.lock);

    return ceError;
}

static
CENTERROR
GPAWaitForUserPolicies(
        uid_t uid
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pContext = NULL;
    BOOLEAN bReleaseLock = FALSE;
    BOOLEAN bKeepPolling = TRUE;
    struct timespec timeout;
    
    GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&gCurrentUsersLock);
    bReleaseLock = TRUE;
    GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    if (!GPALookupUserContext_Unsafe(gCurrentUsers, uid, &pContext)) {
       GPA_LOG_WARNING("Cannot find user [uid=%ld] in list", (long)uid);
       goto done;
    }
    
    GPA_LOG_LOCK("Take pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&pContext->lock);
    GPA_LOG_LOCK("Got pContext->lock at %s:%d",__FILE__,__LINE__);

    GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&gCurrentUsersLock);
    bReleaseLock = FALSE;
    GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    while (!ProcessShouldExit() && bKeepPolling)
    {
        switch(pContext->policyStatus)
        {
            case GP_USER_POLICY_NEW:
            {
                GPA_LOG_VERBOSE("No policies have yet been applied for user [uid=%ld]", (long)uid);
                break;
            }
            case GP_USER_POLICY_PROCESSING:
            {
                GPA_LOG_VERBOSE("Applying policies for user [uid=%ld]", (long)uid);
                break;
            }
            case GP_USER_POLICY_APPLIED:
            {
                GPA_LOG_VERBOSE("Policies applied successfully for user [uid=%ld]", (long)uid);
                bKeepPolling = FALSE;
                break;
            }
            case GP_USER_POLICY_FAILED:
            {
                GPA_LOG_VERBOSE("Not able to apply policies for user [uid=%ld]", (long)uid);
                bKeepPolling = FALSE;
                break;
            }
        }
      
        if (bKeepPolling) {
           timeout.tv_sec = time(NULL) + 60; // wait a sec
           timeout.tv_nsec = 0;

           pthread_cond_timedwait(&pContext->cond,
                                  &pContext->lock,
                                  &timeout);
        }
    }
    
    GPA_LOG_LOCK("Release pContext->lock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&pContext->lock);
    GPA_LOG_LOCK("Released pContext->lock at %s:%d",__FILE__,__LINE__);
    
done:

    if (bReleaseLock)
    {
        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    }
    
    return ceError;
}

CENTERROR
GPAProcessLogin(
    PCSTR pszUsername
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pContext = NULL;
    PGPUSERCONTEXT pContext_ro= NULL;
    PGPUSER pUserInfo = NULL;
    BOOLEAN bReleaseLock = FALSE;
    uid_t uid = 0;
    
    if (IsNullOrEmptyString(pszUsername)) {
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPAFindUserByName(pszUsername, &pUserInfo);
    if (CENTERROR_EQUAL(ceError, CENTERROR_GP_NOT_AD_USER)) {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    uid = pUserInfo->uid;

    GPA_LOG_INFO("Processing Login Policies for User [login=%s,uid=%ld]", pszUsername, (long)uid);
    
    GPA_LOG_LOCK("Take gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_lock(&gCurrentUsersLock);
    bReleaseLock = TRUE;
    GPA_LOG_LOCK("Got gCurrentUsersLock at %s:%d",__FILE__,__LINE__);

    if (GPALookupUserContext_Unsafe(gCurrentUsers, uid, &pContext_ro))
    {
        GPA_LOG_VERBOSE("Found existing user context for [uid=%ld]", (long)uid);
        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        bReleaseLock = FALSE;
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        goto wait_for_policy;
    }
    
    // No user context found. We need to create one.
    if (!GPALookupUserContext_Unsafe(gCurrentUsers, uid, &pContext_ro)) {

        GPA_LOG_VERBOSE("Creating new user context for [uid=%ld]", (long)uid);

        ceError = GPAGetADUserInfoForLoginId(pszUsername, &pContext);
        BAIL_ON_CENTERIS_ERROR(ceError);

        GPAAddUserContext_Unsafe(&gCurrentUsers, pContext);
        pContext = NULL;
    }

    GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    pthread_mutex_unlock(&gCurrentUsersLock);
    bReleaseLock = FALSE;
    GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    
    ceError = GPARefreshUserPolicies();
    BAIL_ON_CENTERIS_ERROR(ceError);
    
wait_for_policy:

    GPA_LOG_VERBOSE("Waiting to apply policies for user [uid=%ld]", (long)uid);

    ceError = GPAWaitForUserPolicies(uid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPA_LOG_VERBOSE("Applied policies for user [uid=%ld]", (long)uid);

error:

    if (bReleaseLock)
    {
        GPA_LOG_LOCK("Release gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
        pthread_mutex_unlock(&gCurrentUsersLock);
        GPA_LOG_LOCK("Released gCurrentUsersLock at %s:%d",__FILE__,__LINE__);
    }

    if (pUserInfo)
    {
        GPAFreeUser(pUserInfo);
    }

    GPA_SAFE_FREE_USER_CONTEXT_LIST(pContext);

    if (!CENTERROR_IS_OK(ceError))
    {
       GPA_LOG_VERBOSE("Not able to apply policies for user [%s].", (IsNullOrEmptyString(pszUsername) ? "" : pszUsername));
    }

    return ceError;
}

CENTERROR
GPAProcessLogout(
    PCSTR pszUsername
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if (IsNullOrEmptyString(pszUsername)) {
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    GPA_LOG_INFO("Processing User [%s] upon logoff", pszUsername);

error:

    return ceError;
}
