#include "includes.h"


#define DWORD_SWAP(dwVal) \
        (dwVal = ((((DWORD)(dwVal) & 0xFF000000L) >> 24) | \
                  (((DWORD)(dwVal) & 0x00FF0000L) >>  8) | \
                  (((DWORD)(dwVal) & 0x0000FF00L) <<  8) | \
                  (((DWORD)(dwVal) & 0x000000FFL) << 24)))

static const CHAR LDAP_PREFIX[]                   = "[LDAP://";

static
CENTERROR
ParseGPLinks(
    PSTR pszBuf,
    BOOLEAN bAddEnforcedOnly,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObject
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFirst = NULL;
    PSTR pszLast = NULL;
    PSTR pszEnd = NULL;
    PSTR pszPolicyDN = NULL;
    char szOptions[PATH_MAX + 1];
    PGROUP_POLICY_OBJECT pGPObjectList = NULL;
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    DWORD dwBytes = 0;

    pszFirst = pszBuf;
    while (pszFirst != NULL && *pszFirst != '\0') {

        pszFirst = strstr(pszFirst, LDAP_PREFIX);
        if (pszFirst == NULL) {
            break;
        }

        pszFirst += strlen(LDAP_PREFIX);

        pszLast = strchr(pszFirst, ';');
        if (pszLast == NULL) {
            ceError = CENTERROR_GP_INVALID_GPLINK;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszEnd = strchr(pszLast, ']');
        if (pszEnd == NULL) {
            ceError = CENTERROR_GP_INVALID_GPLINK;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        dwBytes = pszEnd - (pszLast+1);
        strncpy(szOptions, pszLast+1, dwBytes);
        *(szOptions+dwBytes) = '\0';

        if (bAddEnforcedOnly == TRUE && atoi(szOptions) != GPO_FLAG_FORCE) {
            /* Skip adding GPO since we are trying to block inheritance and the
               current item is not Enforced. */
            pszFirst = strchr(pszLast, ']');
            continue;
        }

        dwBytes = pszLast - pszFirst;

        ceError = LwAllocateMemory(sizeof(GROUP_POLICY_OBJECT), (PVOID*)&pGPObject);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateMemory(dwBytes+1, (PVOID*)&pszPolicyDN);
        BAIL_ON_CENTERIS_ERROR(ceError);

        strncpy(pszPolicyDN, pszFirst, dwBytes);
        *(pszPolicyDN+dwBytes) = '\0';
        pGPObject->pszPolicyDN = pszPolicyDN;
        pGPObject->dwOptions = atoi(szOptions);

        pszPolicyDN = NULL;

        pszFirst = strchr(pszLast, ']');

        pGPObject->pNext = pGPObjectList;
        pGPObjectList = pGPObject;
        pGPObject = NULL;
    }

    ceError = GPAReverseGPOList( pGPObjectList, ppGroupPolicyObject );
    BAIL_ON_CENTERIS_ERROR(ceError);

    pGPObjectList = NULL;

    return (ceError);

error:

    GPA_SAFE_FREE_GPO_LIST (pGPObject);
    GPA_SAFE_FREE_GPO_LIST (pGPObjectList);

    LW_SAFE_FREE_STRING(pszPolicyDN);
    
    return (ceError);
}

CENTERROR
GPAGetPolicyLinkInfo(
    HANDLE hDirectory,
    PSTR pszComputerDN,
    BOOLEAN bAddEnforcedOnly,
    PGROUP_POLICY_OBJECT * ppGroupPolicyObjects,
    DWORD * pdwOptions
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR szAttributeList[] = { GPA_GPLINK_ATTR, GPA_GPOPTIONS_ATTR, NULL };
    PGROUP_POLICY_OBJECT pGPObjectList = NULL;
    PGROUP_POLICY_OBJECT pGPObject = NULL;
    LDAPMessage* pMessage = NULL;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwOptions = 0;
    DWORD dwCount = 0;
    PSTR pszValue = NULL;

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = GPODirectorySearch(
        hDirectory,
        pszComputerDN,
        LDAP_SCOPE_BASE,
        "(objectclass=*)",
        szAttributeList,
        &pMessage);
    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );

    if (dwCount < 0) {

        ceError = CENTERROR_GP_LDAP_ERROR;

    } else if (dwCount == 0) {

        ceError = CENTERROR_GP_LDAP_NO_SUCH_POLICY;

    } else if (dwCount > 1) {

        ceError = CENTERROR_GP_LDAP_MULTIPLE_POLICIES;

    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pMessage,
                               GPA_GPLINK_ATTR,
                               &pszValue);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND) {
        pszValue = NULL;
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszValue) {
        ceError = ParseGPLinks(pszValue, bAddEnforcedOnly, &pGPObjectList);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GPAGetLDAPUInt32(hDirectory,
                                   pMessage,
                                   GPA_GPOPTIONS_ATTR,
                                   &dwOptions);
        if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND) {
            dwOptions = 0;
            ceError = CENTERROR_SUCCESS;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (pdwOptions != NULL) {
            *pdwOptions = dwOptions;
        }

        if (*ppGroupPolicyObjects != NULL) {
            (*ppGroupPolicyObjects)->pNext = pGPObjectList;
        } else {
            *ppGroupPolicyObjects = pGPObjectList;
        }

        LwFreeString(pszValue);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return ceError;

error:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    LW_SAFE_FREE_STRING(pszValue);

    GPA_SAFE_FREE_GPO_LIST (pGPObject);
    GPA_SAFE_FREE_GPO_LIST (pGPObjectList);

    return ceError;
}

static
CENTERROR
GetGPTINIFile(
    PGPUSER pUser,
    PSTR pszgPCFileSysPath,
    PSTR *ppszGPTINIFilePath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szSourceFilePath[PATH_MAX + 1];
    PSTR pszDestFilePath = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszPolicyIdentifier = NULL;
    PSTR pszDC = NULL;
    PSTR pszShortDomainName = NULL;

    ceError = LwAllocateMemory( PATH_MAX + 1,
                                (PVOID*)&pszDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError =  GPACrackFileSysPath( pszgPCFileSysPath,
                                    &pszDomainName,
                                    &pszSourcePath,
                                    &pszPolicyIdentifier);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetShortDomainName( pszDomainName,
                                    &pszShortDomainName );
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetPreferredDomainController(pszDomainName, &pszDC);
    if (!CENTERROR_IS_OK(ceError)) {
        if (CENTERROR_EQUAL( ceError,
                             CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO)   ||
            CENTERROR_EQUAL( ceError,
                             CENTERROR_GP_NO_SMB_KRB5_SITE_INFO)) {
            GPA_LOG_ALWAYS( "GPAgent unable to obtain preferred server for AD site: FQDN(%s) Short Name (%s)",
                            pszDomainName,
                            pszShortDomainName );
            ceError = CENTERROR_SUCCESS;
        } else {
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    sprintf( szSourceFilePath,
             "%s\\%s",
             pszSourcePath,
             "GPT.INI");

    sprintf( pszDestFilePath,
             "%s/%s_GPT.INI",
             CACHEDIR,
             pszPolicyIdentifier);

    ceError = GPOLwioCopyFile(pUser,
                             pszDomainName,
                             pszDC,
                             szSourceFilePath,
                             pszDestFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszGPTINIFilePath = pszDestFilePath;
    pszDestFilePath = NULL;

error:

    if (pszDomainName) {
        LwFreeString(pszDomainName);
    }

    if (pszShortDomainName) {
        LwFreeString(pszShortDomainName);
    }

    if (pszSourcePath) {
        LwFreeString(pszSourcePath);
    }

    if (pszPolicyIdentifier) {
        LwFreeString(pszPolicyIdentifier);
    }

    if (pszDC) {
        LwFreeString(pszDC);
    }

    if (pszDestFilePath) {
        LwFreeString(pszDestFilePath);
    }

    return ceError;
}

static
CENTERROR
ParseAndGetGPTVersion(
    PSTR pszFilePath,
    PDWORD pdwGPTVersion
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszValue = NULL;
    PGPACFGSECTION pSectionList = NULL;

    ceError = GPAParseConfigFile(pszFilePath, 
                                &pSectionList, 
                                FALSE);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetConfigValueBySectionName(pSectionList,
                                            "General",
                                            "Version",
                                            &pszValue);
    if (ceError == CENTERROR_SUCCESS) {
        if (!pszValue) {
            ceError = CENTERROR_CFG_VALUE_NOT_FOUND;
        } else {
            *pdwGPTVersion = atoi(pszValue);
        }
    }

error:

    if (pSectionList) {
        GPAFreeConfigSectionList(pSectionList);
    }

    if (pszValue) {
        LwFreeString(pszValue);
    }

    return ceError;
}

static
CENTERROR
GetGPTVersionNumber(
    PGPUSER pUser,
    PSTR pszgPCFileSysPath,
    PDWORD pdwGPTVersion
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszGPTINIFilePath = NULL;
    BOOLEAN bFileExists = FALSE;

    ceError = GetGPTINIFile(pUser,
                            pszgPCFileSysPath,
                            &pszGPTINIFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACheckFileExists(pszGPTINIFilePath, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(bFileExists)
    {
        ceError = ParseAndGetGPTVersion(pszGPTINIFilePath,
                                        pdwGPTVersion);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwRemoveFile(pszGPTINIFilePath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pszGPTINIFilePath) {
        LwFreeString(pszGPTINIFilePath);
    }

    return ceError;
}

static
void GetComputerAndUserVersionNumbers(
    DWORD dwVersion,
    PWORD pwUserVersion,
    PWORD pwComputerVersion
    )
{
#if defined(WORDS_BIGENDIAN)
    DWORD_SWAP(dwVersion);
#endif

    *pwUserVersion = (WORD)((dwVersion & 0xFFFF0000)>>16);
    *pwComputerVersion = (WORD)(dwVersion & 0x0000FFFF);
}

static
DWORD GetVersionFromUserAndComputer(
    WORD   wUser,
    WORD   wComputer
    )
{
    DWORD dwResult = 
     ((((DWORD)(wUser) & 0xFFFF) << 16) | ((DWORD)(wComputer) & 0xFFFF));

#if defined(WORDS_BIGENDIAN)
    DWORD_SWAP(dwResult);
#endif

    return dwResult;
}

CENTERROR
GPAGetPolicyInformation(
    PGPUSER pUser,
    HANDLE hDirectory,
    PSTR pszPolicyDN,
    PGROUP_POLICY_OBJECT pGroupPolicyObject
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR szAttributeList[] =
        {GPA_DISPLAY_NAME_ATTR,
         GPA_FLAGS_ATTR,
         GPA_FILESYS_PATH_ATTR,
         GPA_FUNCTIONALITY_VERSION_ATTR,
         GPA_MACHINE_EXTENSION_NAMES_ATTR,
         GPA_USER_EXTENSION_NAMES_ATTR,
         GPA_WQL_FILTER_ATTR,
         GPA_VERSION_NUMBER_ATTR,
         NULL
        };
    LDAPMessage* pMessage = NULL;
    DWORD dwCount = 0;
    PSTR  pszValue = NULL;
    DWORD dwValue = 0;
    DWORD dwGPTVersion = 0;
    WORD  wADUserVer = 0;
    WORD  wADComputerVer = 0;
    WORD  wGPTUserVer = 0;
    WORD  wGPTComputerVer = 0;
    PGPO_DIRECTORY_CONTEXT pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;

    ceError = GPODirectorySearch(
        hDirectory,
        pszPolicyDN,
        LDAP_SCOPE_BASE,
        "(objectclass=*)",
        szAttributeList,
        &pMessage);

    BAIL_ON_CENTERIS_ERROR(ceError);

    dwCount = ldap_count_entries(
        pDirectory->ld,
        pMessage
        );

    if (dwCount < 0) {

        ceError = CENTERROR_GP_LDAP_ERROR;

    } else if (dwCount == 0) {

        ceError = CENTERROR_GP_LDAP_NO_SUCH_POLICY;

    } else if (dwCount > 1) {

        ceError = CENTERROR_GP_LDAP_MULTIPLE_POLICIES;

    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLDAPString(hDirectory,
                               pMessage,
                               GPA_DISPLAY_NAME_ATTR,
                               &pszValue);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND)
    {
        /* It is possible to find a GPO, but not be able to read from it when the
           access is filtered by permissions.  Return with error... */
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->pszDisplayName = pszValue;
    pszValue = NULL;

    ceError = GPAGetLDAPUInt32(hDirectory,
                               pMessage,
                               GPA_FLAGS_ATTR,
                               &dwValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->dwFlags = dwValue;

    ceError = GPAGetLDAPString(hDirectory,
                               pMessage,
                               GPA_FILESYS_PATH_ATTR,
                               &pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->pszgPCFileSysPath = pszValue;
    pszValue = NULL;

    ceError = GPAGetLDAPUInt32(hDirectory,
                               pMessage,
                               GPA_FUNCTIONALITY_VERSION_ATTR,
                               &dwValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->gPCFunctionalityVersion = dwValue;

    ceError = GPAGetLDAPString(hDirectory,
                               pMessage,
                               GPA_MACHINE_EXTENSION_NAMES_ATTR,
                               &pszValue);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND) {
        /* Okay to not read this attribute, as it is optional */
        pszValue = NULL;
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->pszgPCMachineExtensionNames = pszValue;
    pszValue = NULL;

    ceError = GPAGetLDAPString(hDirectory,
                               pMessage,
                               GPA_USER_EXTENSION_NAMES_ATTR,
                               &pszValue);
    if (ceError == CENTERROR_GP_LDAP_NO_VALUE_FOUND) {
        /* Okay to not read this attribute, as it is optional */
        pszValue = NULL;
        ceError = CENTERROR_SUCCESS;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->pszgPCUserExtensionNames = pszValue;
    pszValue = NULL;

    ceError = GPAGetLDAPUInt32(hDirectory,
                               pMessage,
                               GPA_VERSION_NUMBER_ATTR,
                               &dwValue);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pGroupPolicyObject->dwVersion = dwValue;

    GetComputerAndUserVersionNumbers(pGroupPolicyObject->dwVersion,
                                     &wADUserVer,
                                     &wADComputerVer);

    GPA_LOG_VERBOSE("Found GPO (%s) with settings located (%s) is at version %d (User:%d Computer:%d)",
                    pGroupPolicyObject->pszDisplayName,
                    pGroupPolicyObject->pszgPCFileSysPath,
                    pGroupPolicyObject->dwVersion,
                    wADUserVer,
                    wADComputerVer);

    /* Here we use the pszgPCFileSysPath to retrieve the GPT.ini file
       from the given GPO's sysvol location and compare the version number in
       it with that read from dwVersion above. If the GPT.ini file is different,
       we save the number from it in place of dwVersion. */
    ceError = GetGPTVersionNumber(pUser,
                                  pGroupPolicyObject->pszgPCFileSysPath,
                                  &dwGPTVersion);
    if (ceError) {
        GPA_LOG_ERROR("The GPT.ini version number could not be determined for GPO (%s) located at (%s), going to use AD GPO version number information instead.",
                        pGroupPolicyObject->pszDisplayName,
                        pGroupPolicyObject->pszgPCFileSysPath);
        ceError = CENTERROR_SUCCESS;
    } else {
        GetComputerAndUserVersionNumbers(dwGPTVersion,
                                         &wGPTUserVer,
                                         &wGPTComputerVer);

        GPA_LOG_VERBOSE("GPO (%s) version number from GPT.INI file is at %d (User:%d Computer:%d)",
                        pGroupPolicyObject->pszDisplayName,
                        dwGPTVersion,
                        wGPTUserVer,
                        wGPTComputerVer);

        if (dwGPTVersion != pGroupPolicyObject->dwVersion){
            WORD wUser = wADUserVer;
            WORD wComputer = wADComputerVer;

            if (wADUserVer > wGPTUserVer) {
                wUser = wGPTUserVer;
            }

            if (wADComputerVer > wGPTComputerVer) {
                wComputer = wGPTComputerVer;
            }

            pGroupPolicyObject->dwVersion =
                GetVersionFromUserAndComputer(wUser, wComputer);

            GPA_LOG_VERBOSE("GPO (%s) version information in AD is not the same as GPT.INI version, going use version %d.",
                            pGroupPolicyObject->pszDisplayName,
                            pGroupPolicyObject->dwVersion);
        }
    }

error:

    if (pszValue) {
        LwFreeString(pszValue);
    }

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return ceError;
}
