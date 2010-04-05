#include "config.h"
#include "macadsys.h"
#include "macadutil.h"
#include "lwutils.h"
#include "utmpx.h"

#define GOTO_CLEANUP_ON_MACERROR(err) \
        if (err) goto cleanup;

typedef struct __MACADUTIL_INTERFACE
{
    PVOID pLibHandle;
    PMACADUTIL_FUNC_TABLE  pFuncTable;
    PFN_MACADUTIL_SHUTDOWN pfnShutdownMacADUtil;
} MACADUTIL_INTERFACE, *PMACADUTIL_INTERFACE;

static PMACADUTIL_INTERFACE gpMacADUtilItf = NULL;

static
long
ValidateMacADUtilInterface(
    PMACADUTIL_FUNC_TABLE pFuncTable
    )
{
    long macError = 0;

    if (!pFuncTable ||
        !pFuncTable->pfnGetADDomain ||
        !pFuncTable->pfnEnumGPOsEnabledForWorkgroupManager ||
        !pFuncTable->pfnGetSpecificGPO ||
        !pFuncTable->pfnIsMCXSettingEnabledForGPO ||
        !pFuncTable->pfnConvertDSAttrValsToMCX ||
        !pFuncTable->pfnSaveMCXValuesForGPOSettingType ||
        !pFuncTable->pfnConvertMCXSettingsToValues ||
        !pFuncTable->pfnGetMCXValuesForGPOSettingType ||
        !pFuncTable->pfnLookupComputerGroupGPO ||
        !pFuncTable->pfnLookupComputerListGPO ||
        !pFuncTable->pfnAuthenticateUser ||
        !pFuncTable->pfnGetUserPrincipalNames ||
        !pFuncTable->pfnNotifyUserLogon ||
        !pFuncTable->pfnNotifyUserLogoff)
    {
       macError = 1;
    }

    return macError;
}

#ifdef __cplusplus
extern "C"
#endif
VOID
LoggerCallback(
    HANDLE      hLog,
    MacADLogLevel logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    )
{
    vprintf(pszFormat, msgList);
    printf("\n");
}

VOID
InitMacADUtilInterface(
    VOID
    )
{
    long macError = 0;
    PMACADUTIL_INTERFACE pMacADUtilItf = NULL;
    PVOID pLibHandle = NULL;
    PCSTR pszError = NULL;
    CHAR szMacADUtilLibPath[PATH_MAX+1];
    PMACADUTIL_FUNC_TABLE  pFuncTable = NULL;
    PFN_MACADUTIL_INITIALIZE pfnInitMacADUtil = NULL;
    PFN_MACADUTIL_SHUTDOWN   pfnShutdownMacADUtil = NULL;

    /* Are we already initialized */
    if (gpMacADUtilItf) {
	    printf("libmacadutil library already initialized\n");
	    return;
	}
		
    sprintf(szMacADUtilLibPath, "/opt/likewise/lib/libmacadutil.so");

    dlerror();
    pLibHandle = dlopen(szMacADUtilLibPath, RTLD_NOW | RTLD_GLOBAL);
    if (pLibHandle == NULL) {
        pszError = dlerror();
        printf("Error: Failed to load Likewise libmacadutil Module [%s]\n",
                      IsNullOrEmptyString(pszError) ? "" : pszError);
		macError = 1;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    dlerror();
    pfnInitMacADUtil = (PFN_MACADUTIL_INITIALIZE)dlsym(
                                pLibHandle,
                                MACADUTIL_INITIALIZE);
    if (pfnInitMacADUtil == NULL) {
        pszError = dlerror();
        printf("Error: Failed to lookup symbol %s in libmacadutil Module [%s]\n",
		              MACADUTIL_INITIALIZE,
                      IsNullOrEmptyString(pszError) ? "" : pszError);
        macError = 1;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    dlerror();
    pfnShutdownMacADUtil = (PFN_MACADUTIL_SHUTDOWN)dlsym(
                                pLibHandle,
                                MACADUTIL_SHUTDOWN);
    if (pfnShutdownMacADUtil == NULL) {
        pszError = dlerror();
        printf("Error: Failed to lookup symbol %s in libmacadutil Module [%s]\n",
		                MACADUTIL_SHUTDOWN,
                        IsNullOrEmptyString(pszError) ? "" : pszError);
        macError = 1;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = pfnInitMacADUtil((HANDLE)NULL, &LoggerCallback, MAC_AD_LOG_LEVEL_VERBOSE, &pFuncTable);
    GOTO_CLEANUP_ON_MACERROR(macError);
	
	printf("Init call for libmacadutil completed\n");

    macError = ValidateMacADUtilInterface(pFuncTable);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWAllocateMemory(sizeof(MACADUTIL_INTERFACE), (PVOID*)&pMacADUtilItf);
    GOTO_CLEANUP_ON_MACERROR(macError);
	
	printf("validate function table completed\n");

    pMacADUtilItf->pFuncTable = pFuncTable;
    pMacADUtilItf->pLibHandle = pLibHandle;
    pMacADUtilItf->pfnShutdownMacADUtil = pfnShutdownMacADUtil;

    gpMacADUtilItf = pMacADUtilItf;

    printf("libmacadutil library loaded successfully\n");
	return;

cleanup:

    if (pfnShutdownMacADUtil) {
        pfnShutdownMacADUtil(pFuncTable);
    }

    if (pLibHandle) {
        dlclose(pLibHandle);
    }
}

VOID
ShutdownMacADUtilInterface(
    VOID
    )
{
    PMACADUTIL_INTERFACE pMacADUtilItf = gpMacADUtilItf;
    if (pMacADUtilItf) {
       if (pMacADUtilItf->pfnShutdownMacADUtil) {
          pMacADUtilItf->pfnShutdownMacADUtil(pMacADUtilItf->pFuncTable);
       }
       if (pMacADUtilItf->pLibHandle) {
           dlclose(pMacADUtilItf->pLibHandle);
       }
       LWFreeMemory(pMacADUtilItf);
    }
}

long GetADDomain(
    PSTR* ppszDomain
    )
{
    if (gpMacADUtilItf)
	    return gpMacADUtilItf->pFuncTable->pfnGetADDomain(ppszDomain);
	else
	{
	    *ppszDomain = NULL;
        return 2;
	}
}

long EnumWorkgroupManagerEnabledGPOs(
    PSTR                  pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    )
{
    if (gpMacADUtilItf)
	    return gpMacADUtilItf->pFuncTable->pfnEnumGPOsEnabledForWorkgroupManager(pszDomainName, ppMCXGPOs);
	else
        return 2;
}

void
FreeMCXValueList(
    PMCXVALUE pValueList
        )
{
    while (pValueList)
    {
        PMCXVALUE pTemp = pValueList;

        pValueList = pValueList->pNext;

        if (pTemp->pValueData)
            LWFreeMemory(pTemp->pValueData);

        LWFreeMemory(pTemp);
    }
}

int main()
{
    long lError = 0;
    PSTR pszDomain = NULL;
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bMergeModeMCX = FALSE;
    BOOLEAN bEnableForceHomedirOnStartupDisk = FALSE;
    BOOLEAN bUseADUNCForHomeLocation = FALSE;
    PSTR szUNCProtocolForHomeLocation = NULL;
    PSTR szAllowAdministrationBy = NULL;
    BOOLEAN bMergeAdmins = FALSE;
    
    InitMacADUtilInterface();

    lError = gpMacADUtilItf->pFuncTable->pfnGetConfigurationSettings(&bMergeModeMCX,
                                                                     &bEnableForceHomedirOnStartupDisk,
                                                                     &bUseADUNCForHomeLocation,
                                                                     &szUNCProtocolForHomeLocation,
                                                                     &szAllowAdministrationBy,
                                                                     &bMergeAdmins);

    printf("Configuration: \nMerge mode = %s\nForce homedir = %s\nUse AD UNC = %s\nUNC Protocol = %s\nAdmin by = %s\nMerge Admins = %s\n\n",
           bMergeModeMCX ? "true" : "false",
           bEnableForceHomedirOnStartupDisk ? "true" : "false",
           bUseADUNCForHomeLocation ? "true" : "false",
           szUNCProtocolForHomeLocation ? szUNCProtocolForHomeLocation : "null",
           szAllowAdministrationBy ? szAllowAdministrationBy : "null",
           bMergeAdmins ? "true" : "false");

    lError = GetADDomain(&pszDomain);
    if (lError)
       goto error;

    lError = EnumWorkgroupManagerEnabledGPOs(
                pszDomain,
                &pGPOList);
    if (lError)
       goto error;

    for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
    {
        PMCXVALUE pMCXValues = NULL, pMCX = NULL;;

        printf("Display name: %s\n", pGPO->pszDisplayName);
	lError = gpMacADUtilItf->pFuncTable->pfnIsMCXSettingEnabledForGPO(pGPO, USER_GROUP_POLICY, &bEnabled);
        if (lError)
            goto error;
        printf("GPO %s User MCX settings enabled\n", bEnabled ? "has" : "does not have");
        if (bEnabled) {
	    lError = gpMacADUtilItf->pFuncTable->pfnGetMCXValuesForGPOSettingType(pGPO, USER_GROUP_POLICY, &pMCXValues);
            if (lError) {
                printf("GPO does not have User MCX values\n");
                goto error;
            }
            printf("Found User MCX values for GPO\n");
            pMCX = pMCXValues;
            while (pMCX) {
                printf("MCX Value part iValLen: %d\n", pMCX->iValLen);
                pMCX = pMCX->pNext;
            }
            FreeMCXValueList(pMCXValues);
        }

	lError = gpMacADUtilItf->pFuncTable->pfnIsMCXSettingEnabledForGPO(pGPO, MACHINE_GROUP_POLICY, &bEnabled);
        if (lError)
            goto error;
        printf("GPO %s Computer MCX settings enabled\n", bEnabled ? "has" : "does not have");
        if (bEnabled) {
	    lError = gpMacADUtilItf->pFuncTable->pfnGetMCXValuesForGPOSettingType(pGPO, MACHINE_GROUP_POLICY, &pMCXValues);
            if (lError) {
                printf("GPO does not have  Computer MCX values\n");
                goto error;
            }
            printf("Found Computer MCX values for GPO\n");
            pMCX = pMCXValues;
            while (pMCX) {
                printf("MCX Value part iValLen: %d\n", pMCX->iValLen);
                pMCX = pMCX->pNext;
            }
            FreeMCXValueList(pMCXValues);
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(szUNCProtocolForHomeLocation);
    LW_SAFE_FREE_STRING(szAllowAdministrationBy);

    if (pszDomain)
    {
        free(pszDomain);
        pszDomain = NULL;
    }

    return lError;

error:

    goto cleanup;
}
