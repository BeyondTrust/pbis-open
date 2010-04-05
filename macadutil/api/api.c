/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        adutils.c
 *
 * Abstract:
 *
 *       Mac Workgroup Manager
 *
 *       AD Utility API
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

static void *                gpLsaAccessLibHandle = (void*)NULL;

static PFNLSAACCESSGETDATA   gpfnLsaAccessGetData = NULL;
static PFNLSAACCESSCHECKDATA gpfnLsaAccessCheckData = NULL;
static PFNLSAACCESSFREEDATA  gpfnLsaAccessFreeData = NULL;

#define LSAACCESS_LIBPATH    LIBDIR "/" "liblsaaccess" MOD_EXT

static
DWORD
LWLoadLsaAccessLibrary(
    void
    )
{
    DWORD dwError = 0;
    PCSTR pszError = NULL;

    if ( gpLsaAccessLibHandle )
    {
        goto cleanup;
    }

    dlerror();

    gpLsaAccessLibHandle = dlopen(LSAACCESS_LIBPATH,
                                  RTLD_NOW | RTLD_GLOBAL);
    if ( gpLsaAccessLibHandle == NULL )
    {
        pszError = dlerror();
        dwError = MAC_AD_ERROR_LOAD_LIBRARY_FAILED;
        MAC_AD_LOG_ERROR(
            "Failed to load library [%s]. Error [%s]",
            LSAACCESS_LIBPATH,
            (IsNullOrEmptyString(pszError) ? "" : pszError));
        BAIL_ON_MAC_ERROR(dwError);
    }

    gpfnLsaAccessGetData = (PFNLSAACCESSGETDATA)dlsym(
                               gpLsaAccessLibHandle,
                               LSA_SYMBOL_NAME_ACCESS_GET_DATA);
    if ( gpfnLsaAccessGetData == NULL )
    {
        MAC_AD_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_GET_DATA);
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_MAC_ERROR(dwError);
    }

    gpfnLsaAccessCheckData = (PFNLSAACCESSCHECKDATA)dlsym(
                                 gpLsaAccessLibHandle,
                                 LSA_SYMBOL_NAME_ACCESS_CHECK_DATA);
    if ( gpfnLsaAccessCheckData == NULL )
    {
        MAC_AD_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_CHECK_DATA);
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_MAC_ERROR(dwError);
    }

    gpfnLsaAccessFreeData = (PFNLSAACCESSFREEDATA)dlsym(
                                gpLsaAccessLibHandle,
                                LSA_SYMBOL_NAME_ACCESS_FREE_DATA);
    if ( gpfnLsaAccessGetData == NULL )
    {
        MAC_AD_LOG_ERROR(
            "Unable to find LSA Access API - %s",
            LSA_SYMBOL_NAME_ACCESS_FREE_DATA);
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_MAC_ERROR(dwError);
    }

cleanup:

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

static
void
LWUnloadLsaAccessLibrary(
    )
{
    if ( gpLsaAccessLibHandle )
    {
        dlclose(gpLsaAccessLibHandle);
        gpLsaAccessLibHandle = NULL;
    }
}

LONG
LWMCInitialize(
    HANDLE                 hLog,
    PFN_MAC_AD_LOG_MESSAGE pfnLogHandler,
    MacADLogLevel          maxLogLevel,
    PMACADUTIL_FUNC_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    BOOLEAN bDirExists = FALSE;

    BAIL_ON_INVALID_POINTER(ppFnTable);

    dwError = LWLoadLsaAccessLibrary();
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCheckDirectoryExists(LWDS_ADMIN_CACHE_DIR, &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!bDirExists)
    {
        dwError = LWCreateDirectory(LWDS_ADMIN_CACHE_DIR, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppFnTable = &gpfnTable;

    LWSetLogHandler(
        hLog,
        pfnLogHandler,
        maxLogLevel);

cleanup:

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
LWMCShutdown(
    PMACADUTIL_FUNC_TABLE pFnTable
    )
{
    LWResetLogHandler();

    LWUnloadLsaAccessLibrary();

    return 0;
}

LONG
GetADDomain(
    PSTR* ppszDomain
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PSTR pszDomain_out = NULL;

    dwError = LWNetGetCurrentDomain(&pszDomain);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateString(pszDomain, &pszDomain_out);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszDomain = pszDomain_out;

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return LWGetMacError(dwError);

error:

    *ppszDomain = NULL;

    LW_SAFE_FREE_STRING(pszDomain_out);

    goto cleanup;
}

LONG
EnumWorkgroupManagerEnabledGPOs(
    PCSTR                 pszDomainName,
    PGROUP_POLICY_OBJECT* ppMCXGPOs
    )
{
    DWORD dwError = 0;
    PSTR  pszDomainDN = NULL;
    PSTR  pszSearchDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pGroupPolicyObjects = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;
    PSTR    pszOrigCachePath = NULL;

    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = EINVAL;
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* Set default credentials to the machine's */
    dwError = ADUInitKrb5(pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUOpenDirectory(pszDomainName, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomainName, &pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetAllMCXGPOList(
                    hDirectory,
                    pszSearchDN,
                    &pGroupPolicyObjects);
    BAIL_ON_MAC_ERROR(dwError);

    *ppMCXGPOs = pGroupPolicyObjects;
    pGroupPolicyObjects = NULL;

cleanup:

    if (hDirectory != (HANDLE)NULL)
    {
        ADUCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);

    ADU_SAFE_FREE_GPO_LIST(pGroupPolicyObjects);

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            MAC_AD_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    *ppMCXGPOs = NULL;

    goto cleanup;
}

LONG
GetSpecificGPO(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR    pszOrigCachePath = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;

    /* pszDomainName can be NULL sometimes, here we will default to our
       configured joined domain */
    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = GetADDomain(&pszDomain);
        BAIL_ON_MAC_ERROR(dwError);
    }
    else
    {
        dwError = LWAllocateString(pszDomainName, &pszDomain);
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* Set default credentials to the machine's */
    dwError = ADUInitKrb5(pszDomain);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomain,
                    NULL,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = GetSpecificGPO_authenticated(
                    pszDomain,
                    pszGPOName,
                    &pGPO);
    BAIL_ON_MAC_ERROR(dwError);

    *ppGPO = pGPO;

cleanup:

    LW_SAFE_FREE_STRING(pszDomain);

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            MAC_AD_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    *ppGPO = NULL;

    ADU_SAFE_FREE_GPO_LIST(pGPO);

    goto cleanup;
}

LONG
GetSpecificGPO_authenticated(
    PCSTR                 pszDomainName,
    PCSTR                 pszGPOName,
    PGROUP_POLICY_OBJECT* ppGPO
    )
{
    DWORD dwError = 0;
    PSTR pszDomainDN = NULL;
    PSTR pszSearchDN = NULL;
    PSTR  pszDomainLocal = NULL;
    PCSTR pszDomain = pszDomainName;
    HANDLE hDirectory = (HANDLE)NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;

    if (IsNullOrEmptyString(pszGPOName))
    {
        dwError = EINVAL;
        BAIL_ON_MAC_ERROR(dwError);
    }

    /* pszDomainName can be NULL sometimes, here we will default to our
       configured joined domain */
    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = GetADDomain(&pszDomainLocal);
        BAIL_ON_MAC_ERROR(dwError);

        pszDomain = pszDomainLocal;
    }

    dwError = ADUOpenDirectory(pszDomain, &hDirectory);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUConvertDomainToDN(pszDomain, &pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWAllocateStringPrintf(
                    &pszSearchDN,
                    "CN=Policies,CN=System,%s",
                    pszDomainDN);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUGetMCXGPO(
                    hDirectory,
                    pszSearchDN,
                    pszGPOName,
                    &pGPO);
    BAIL_ON_MAC_ERROR(dwError);

    if (!pGPO)
    {
        dwError = MAC_AD_ERROR_INVALID_NAME;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppGPO = pGPO;

cleanup:

    if (hDirectory != (HANDLE)NULL)
    {
        ADUCloseDirectory(hDirectory);
    }

    LW_SAFE_FREE_STRING(pszDomainDN);
    LW_SAFE_FREE_STRING(pszSearchDN);
    LW_SAFE_FREE_STRING(pszDomainLocal);

    return LWGetMacError(dwError);

error:

    *ppGPO = NULL;

    ADU_SAFE_FREE_GPO_LIST(pGPO);

    goto cleanup;
}

LONG
IsMCXSettingEnabledForGPO(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PBOOLEAN             pbEnabled
    )
{
    DWORD dwError = 0;
    PGROUP_POLICY_OBJECT pMatchedPolicy = NULL;

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            MACHINE_GROUP_POLICY,
                            COMPUTER_MCX_CSE_GUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_MAC_ERROR(dwError);
            break;

        case USER_GROUP_POLICY:
            dwError = ADUComputeCSEList(
                            USER_GROUP_POLICY,
                            USER_MCX_CSE_GUID,
                            pGPO,
                            &pMatchedPolicy);
            BAIL_ON_MAC_ERROR(dwError);
            break;

        default:

            dwError = MAC_AD_ERROR_INVALID_PARAMETER;
            BAIL_ON_MAC_ERROR(dwError);
    }

    *pbEnabled = (pMatchedPolicy != NULL);
    MAC_AD_LOG_INFO("ADUAdapter_IsMCXSettingEnabledForGPO(type %s): %s",
                    dwPolicyType == MACHINE_GROUP_POLICY ? "Computer" : "User",
                    pMatchedPolicy ? "yes" : "no");

cleanup:

    ADU_SAFE_FREE_GPO_LIST(pMatchedPolicy);

    return LWGetMacError(dwError);

error:

    *pbEnabled = FALSE;

    goto cleanup;
}

LONG
ConvertMCXSettingsToMCXValues(
    PCSTR pszPolicyPath,
    DWORD dwPolicyType,
    PMCXVALUE * ppMCXValueList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PMCXVALUE pMCXValues = NULL;
    PMCXVALUE pNew = NULL;
    PMCXVALUE pPrev = NULL;
    PSTR pszMCXFile = NULL;
    char szMCXValueFile[PATH_MAX];
    int iIter = 0;

    if (!ppMCXValueList)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = GetFileNameForMCXSettings(dwPolicyType, &pszMCXFile);
    BAIL_ON_MAC_ERROR(dwError);

    while (TRUE)
    {
        memset(szMCXValueFile, 0, sizeof(szMCXValueFile));
        sprintf(szMCXValueFile, "%s/%s-%d.mcx", pszPolicyPath, pszMCXFile, iIter+1);

        dwError = ReadMCXValueFromFile(szMCXValueFile, &pNew);
        if (dwError == MAC_AD_ERROR_INVALID_NAME)
        {
            dwError = MAC_AD_ERROR_SUCCESS;
            break;
        }
        BAIL_ON_MAC_ERROR(dwError);

        if (pPrev)
        {
            pPrev->pNext = pNew;
        }
        else
        {
            pMCXValues = pNew;
        }

        pPrev = pNew;
        pNew = NULL;

        iIter++;
    }

    if (iIter && pMCXValues)
    {
        *ppMCXValueList = pMCXValues;
        pMCXValues = NULL;
    }
    else
    {
        *ppMCXValueList = NULL;
    }

cleanup:

    if (pszMCXFile)
        LWFreeString(pszMCXFile);

    if (pMCXValues)
        FreeMCXValueList(pMCXValues);

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
ConvertDSAttributeValuesToMCXValues(
    PDSATTRIBUTEVALUE    pValues,
    PMCXVALUE *          ppMCXValues
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PDSATTRIBUTEVALUE pAttrValue = NULL;
    PMCXVALUE pNew = NULL;
    PMCXVALUE pPrev = NULL;
    PMCXVALUE pValueList = NULL;

    BAIL_ON_INVALID_POINTER(ppMCXValues);

    pAttrValue = pValues;
    while (pAttrValue)
    {
        dwError = LWAllocateMemory(sizeof(MCXVALUE), (PVOID*) &pNew);
        BAIL_ON_MAC_ERROR(dwError);

        pNew->iValLen = pAttrValue->valLen;

        dwError = LWAllocateMemory(pNew->iValLen, (PVOID*)&pNew->pValueData);
        BAIL_ON_MAC_ERROR(dwError);

        memcpy(pNew->pValueData, pAttrValue->pszValue, pNew->iValLen);

        if (pPrev)
        {
            pPrev->pNext = pNew;
        }
        else
        {
            pValueList = pNew;
        }

        pPrev = pNew;
        pNew = NULL;

        pAttrValue = pAttrValue->pNext;
    }

    *ppMCXValues = pValueList;

cleanup:

    return LWGetMacError(dwError);

error:

    if (ppMCXValues)
    {
        *ppMCXValues = NULL;
    }

    if (pValueList)
    {
        FreeMCXValueList(pValueList);
    }

    goto cleanup;
}

LONG
SaveMCXValuesForGPOSettingType(
    PMCXVALUE            pMCXValueList,
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PCSTR                pszUserUPN
    )
{
    DWORD   dwError = 0;
    BOOLEAN bPolicyExists = FALSE;
    PSTR    pszSourceFolder = NULL;
    PSTR    pszAllMCXFiles = NULL;
    DWORD   dwFileVersion = 0;
    DWORD   dwVersion = 0;
    WORD    wUserFileVersion = 0;
    WORD    wComputerFileVersion = 0;
    WORD    wUserVersion = 0;
    WORD    wComputerVersion = 0;
    char    szCseGuid[256];
    PSTR    pszDomainName = NULL;
    BOOLEAN bCurrent = FALSE;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR    pszOrigCachePath = NULL;
    BOOLEAN bDeactivateCredContext = FALSE;

    MAC_AD_LOG_INFO("Saving %s MCX Settings for GPO (%s)",
        dwPolicyType == MACHINE_GROUP_POLICY ? "machine" :
        dwPolicyType == USER_GROUP_POLICY ? "user" :
        "unknown",
        pGPO->pszDisplayName);

    BAIL_ON_INVALID_POINTER(pGPO);

    memset(szCseGuid, 0, sizeof(szCseGuid));

    /* Set default credentials to the machine's */
    dwError = LWNetGetCurrentDomain(&pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = IsCacheDataCurrentForGPO(
                    pGPO,
                    &dwVersion,
                    &dwFileVersion,
                    &bCurrent);
    BAIL_ON_MAC_ERROR(dwError);

    ADUGetComputerAndUserVersionNumbers(dwVersion, &wUserVersion, &wComputerVersion);
    ADUGetComputerAndUserVersionNumbers(dwFileVersion, &wUserFileVersion, &wComputerFileVersion);

    if (wUserVersion < wUserFileVersion)
    {
        wUserVersion = wUserFileVersion;
    }

    if (wComputerVersion < wComputerFileVersion)
    {
        wComputerVersion = wComputerFileVersion;
    }

    switch(dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:

            strcpy(szCseGuid, (PSTR) COMPUTER_MCX_CSE_GUID);

            dwError = GetCachedPolicyFiles(
                            MACHINE_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            NULL,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_MAC_ERROR(dwError);

            wComputerVersion++;

            break;

        case USER_GROUP_POLICY:

            strcpy(szCseGuid, (PSTR) USER_MCX_CSE_GUID);

            dwError = GetCachedPolicyFiles(
                            USER_GROUP_POLICY,
                            pGPO->pszgPCFileSysPath,
                            szCseGuid,
                            NULL,
                            &pszSourceFolder,
                            &bPolicyExists);
            BAIL_ON_MAC_ERROR(dwError);

            wUserVersion++;

            break;

        default:

            dwError = MAC_AD_ERROR_INVALID_PARAMETER;
            BAIL_ON_MAC_ERROR(dwError);
    }

    if (bPolicyExists)
    {
        dwError = LWAllocateStringPrintf(
                        &pszAllMCXFiles,
                        "%s/%s",
                        pszSourceFolder,
                        "*.mcx");
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LWRemoveFiles(pszAllMCXFiles, FALSE, FALSE);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = ConvertMCXValuesToMCXSettings(
                    pMCXValueList,
                    pszSourceFolder,
                    dwPolicyType);
    BAIL_ON_MAC_ERROR(dwError);

    dwVersion = ADUGetVersionFromUserAndComputer(wUserVersion, wComputerVersion);
    BAIL_ON_MAC_ERROR(dwError);

    /* Change thread to user's creds */
    dwError = ADUDeactivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = FALSE;

    ADUFreeCredContext(pCredContext);
    pCredContext = NULL;

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    pszUserUPN,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            MAC_AD_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
        pszOrigCachePath = NULL;
    }

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = ADUPutPolicyFiles(
                    NULL,
                    TRUE /* replace destination */,
                    dwPolicyType,
                    pGPO->pszgPCFileSysPath,
                    szCseGuid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUSetGPTVersionNumber(pGPO->pszgPCFileSysPath, dwVersion);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUSetPolicyVersionInAD(pGPO, dwVersion);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszSourceFolder);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszAllMCXFiles);

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            MAC_AD_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
GetMCXValuesForGPOSettingType(
    PGROUP_POLICY_OBJECT pGPO,
    DWORD                dwPolicyType,
    PMCXVALUE *          ppMCXValueList
    )
{
    DWORD     dwError = 0;
    PSTR      pszMachinePolicyPath = NULL;
    PSTR      pszUserPolicyPath = NULL;
    PSTR      pszDomainName = NULL;
    BOOLEAN   bMachinePolicyExists = FALSE;
    BOOLEAN   bUserPolicyExists = FALSE;
    PMCXVALUE pMCXValues = NULL;
    PADU_CRED_CONTEXT pCredContext = NULL;
    PSTR      pszOrigCachePath = NULL;
    BOOLEAN   bDeactivateCredContext = FALSE;

    BAIL_ON_INVALID_POINTER(ppMCXValueList);

    /* Set default credentials to the machine's */
    dwError = LWNetGetCurrentDomain(&pszDomainName);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUBuildCredContext(
                    pszDomainName,
                    NULL,
                    &pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUKrb5SetDefaultCachePath(
                    pCredContext->pszCachePath,
                    &pszOrigCachePath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = ADUActivateCredContext(pCredContext);
    BAIL_ON_MAC_ERROR(dwError);

    bDeactivateCredContext = TRUE;

    dwError = GetCurrentMCXSettingsForGPO(pGPO,
                                          &bMachinePolicyExists,
                                          &bUserPolicyExists,
                                          &pszMachinePolicyPath,
                                          &pszUserPolicyPath);
    BAIL_ON_MAC_ERROR(dwError);

    switch (dwPolicyType)
    {
        case MACHINE_GROUP_POLICY:

            if (bMachinePolicyExists && pszMachinePolicyPath)
            {
                dwError = ConvertMCXSettingsToMCXValues(
                                pszMachinePolicyPath,
                                MACHINE_GROUP_POLICY,
                                &pMCXValues);
                BAIL_ON_MAC_ERROR(dwError);
            }

            break;

       case USER_GROUP_POLICY:

           if (bUserPolicyExists && pszUserPolicyPath )
           {
               dwError = ConvertMCXSettingsToMCXValues(
                               pszUserPolicyPath,
                               USER_GROUP_POLICY,
                               &pMCXValues);
               BAIL_ON_MAC_ERROR(dwError);
           }

           break;

       default:

           dwError = MAC_AD_ERROR_INVALID_PARAMETER;
           BAIL_ON_MAC_ERROR(dwError);
    }

    *ppMCXValueList = pMCXValues;

cleanup:

    LW_SAFE_FREE_STRING(pszMachinePolicyPath);
    LW_SAFE_FREE_STRING(pszUserPolicyPath);
    LW_SAFE_FREE_STRING(pszDomainName);

    if (pCredContext)
    {
        if (bDeactivateCredContext)
        {
            ADUDeactivateCredContext(pCredContext);
        }

        ADUFreeCredContext(pCredContext);
    }

    if (pszOrigCachePath)
    {
        DWORD dwError2 = ADUKrb5SetDefaultCachePath(pszOrigCachePath, NULL);

        if (dwError2)
        {
            MAC_AD_LOG_ERROR("Failed to revert kerberos cache path [code:%d]",
                             dwError2);
        }

        LWFreeMemory(pszOrigCachePath);
    }

    return LWGetMacError(dwError);

error:

    if (ppMCXValueList)
    {
        *ppMCXValueList = NULL;
    }

    goto cleanup;
}

LONG
LookupComputerGroupGPO(
    PCSTR pszName,
    PSTR* ppszGPOGUID
    )
{
    DWORD dwError = 0;
    FILE * fp = NULL;
    PSTR pszGPOGUID = NULL;
    char szGPOName[256];
    char szGPOGUID[256];

    BAIL_ON_INVALID_POINTER(ppszGPOGUID);

    fp = fopen("/var/lib/likewise/grouppolicy/mcx/computer/.lwe-computer-mcx", "r");
    if (!fp)
    {
        MAC_AD_LOG_ERROR("LookupComputerGroupsGPO(%s) failed to find file with list of computer GPOs", pszName);
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (1)
    {
        if ( NULL == fgets( szGPOName,
                            sizeof(szGPOName),
                            fp) )
        {
            if (feof(fp))
            {
                break;
            }

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        LWStripWhitespace(szGPOName);

        if ( NULL == fgets( szGPOGUID,
                            sizeof(szGPOGUID),
                            fp) )
        {
            if (feof(fp)) {
                break;
            }

            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }
        LWStripWhitespace(szGPOGUID);

        if (!strcmp(szGPOName, pszName))
        {
            MAC_AD_LOG_INFO("LookupComputerGroupGPO(%s) found group (Name: %s GUID: %s)", pszName, szGPOName, szGPOGUID);

            dwError = LWAllocateString(szGPOGUID, &pszGPOGUID);
            BAIL_ON_MAC_ERROR(dwError);

            break;
        }
    }

    if (!pszGPOGUID)
    {
        MAC_AD_LOG_INFO("LookupComputerGroupGPO function did not find GPO, returning record not found");
        dwError = MAC_AD_ERROR_NO_SUCH_POLICY;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *ppszGPOGUID = pszGPOGUID;

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    return LWGetMacError(dwError);

error:

    if (ppszGPOGUID)
    {
        *ppszGPOGUID = NULL;
    }

    LW_SAFE_FREE_STRING(pszGPOGUID);

    goto cleanup;
}

LONG
LookupComputerListGPO(
    PCSTR  pszName,
    PSTR * ppszGPOGUID
    )
{
    DWORD dwError = 0;

    dwError = LookupComputerGroupGPO(
                    pszName,
                    ppszGPOGUID);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return LWGetMacError(dwError);

error:

    *ppszGPOGUID = NULL;

    goto cleanup;
}

LONG
AuthenticateUser(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    HANDLE hLsaServer = (HANDLE)NULL;

    dwError = LsaOpenServer(&hLsaServer);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaAuthenticateUser(
                  hLsaServer,
                  pszUsername,
                  pszPassword);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hLsaServer != (HANDLE)NULL) {
       LsaCloseServer(hLsaServer);
    }

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
GetUserPrincipalNames(
    PCSTR pszUserName,
    PSTR * ppszUserPrincipalName,
    PSTR * ppszUserSamAccount,
    PSTR * ppszUserDomainFQDN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    PSTR pszUPN = NULL;
    PLSA_SID_INFO pSIDInfoList = NULL;
    PLSASTATUS pLsaStatus = NULL;
    PSTR pszUserSamAccount = NULL;
    PSTR pszUserDomain = NULL;
    int i = 0, j = 0;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaFindUserByName(hLsaConnection,
                                pszUserName,
                                dwUserInfoLevel,
                                &pUserInfo);
    BAIL_ON_MAC_ERROR(dwError);

    if (((PLSA_USER_INFO_1)pUserInfo)->pszUPN)
    {
        MAC_AD_LOG_INFO("Got UPN (%s) for user: %s", ((PLSA_USER_INFO_1)pUserInfo)->pszUPN, pszUserName);
    }
    else
    {
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (((PLSA_USER_INFO_1)pUserInfo)->pszSid)
    {
        MAC_AD_LOG_INFO("Got SID (%s) for user: %s", ((PLSA_USER_INFO_1)pUserInfo)->pszSid, pszUserName);
    }
    else
    {
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LsaGetNamesBySidList(hLsaConnection,
                                   1,
                                   &((PLSA_USER_INFO_1)pUserInfo)->pszSid,
                                   &pSIDInfoList,
                                   NULL);
    BAIL_ON_MAC_ERROR(dwError);

    if (pSIDInfoList[0].accountType != AccountType_User)
    {
        MAC_AD_LOG_INFO("Could not get names for SID (%s) of user: %s, authentication subsystem maybe offline", ((PLSA_USER_INFO_1)pUserInfo)->pszSid, pszUserName);
        dwError = MAC_AD_ERROR_UPN_NOT_FOUND;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LsaGetStatus(hLsaConnection,
                           &pLsaStatus);
    BAIL_ON_MAC_ERROR(dwError);

    if (pSIDInfoList[0].pszDomainName != NULL)
    {
        for (i = 0; i < pLsaStatus->dwCount; i++)
        {
            if (!strcmp(pLsaStatus->pAuthProviderStatusList[i].pszId, "lsa-activedirectory-provider"))
            {
                for (j = 0; j < pLsaStatus->pAuthProviderStatusList[i].dwNumTrustedDomains; j++)
                {
                    if (pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszNetbiosDomain != NULL &&
                        !strcmp(pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszNetbiosDomain,
                                pSIDInfoList[0].pszDomainName))
                    {
                        LWAllocateString(pLsaStatus->pAuthProviderStatusList[i].pTrustedDomainInfoArray[j].pszDnsDomain,
                                         &pszUserDomain);
                        BAIL_ON_MAC_ERROR(dwError);
                        break;
                    }
                }
            }
        }
    }

    if (pszUserDomain)
    {
        MAC_AD_LOG_INFO("Got domain (%s) for user: %s", pszUserDomain, pszUserName);
    }

    LWAllocateString(pSIDInfoList->pszSamAccountName, &pszUserSamAccount);
    BAIL_ON_MAC_ERROR(dwError);

    LWAllocateString(((PLSA_USER_INFO_1)pUserInfo)->pszUPN, &pszUPN);
    BAIL_ON_MAC_ERROR(dwError);

    *ppszUserPrincipalName = pszUPN;
    pszUPN = NULL;
    *ppszUserSamAccount = pszUserSamAccount;
    pszUserSamAccount = NULL;
    *ppszUserDomainFQDN = pszUserDomain;
    pszUserDomain = NULL;

cleanup:

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (pSIDInfoList)
    {
        LsaFreeSIDInfoList(pSIDInfoList, 1);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pszUPN)
    {
        LWFreeString(pszUPN);
    }

    if (pszUserSamAccount)
    {
        LWFreeString(pszUserSamAccount);
    }

    if (pszUserDomain)
    {
        LWFreeString(pszUserDomain);
    }

    return LWGetMacError(dwError);

error:

    MAC_AD_LOG_INFO("Failed to get UPN for user (%s) with error: %d", pszUserName, dwError);

    goto cleanup;
}

void
GetLsaStatus(
    PBOOLEAN pbIsStarted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    BOOLEAN IsStarted = FALSE;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LsaGetStatus(hLsaConnection,
                           &pLsaStatus);
    BAIL_ON_MAC_ERROR(dwError);

    IsStarted = TRUE;

cleanup:

    *pbIsStarted = IsStarted;

    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return;

error:

    MAC_AD_LOG_INFO("Failed to get lsassd status with error: %d", dwError);

    goto cleanup;
}

LONG
NotifyUserLogon(
    PCSTR pszUserName
    )
{
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = GPOClientProcessLogin(hGPConnection, pszUserName);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hGPConnection != (HANDLE)NULL) {
        GPOClientCloseContext(hGPConnection);
    }

    return LWGetMacError(dwError);

error:

    MAC_AD_LOG_INFO("Failed to notify group policy about user logon (%s) with error: %d", pszUserName, dwError);

    goto cleanup;
}

LONG
NotifyUserLogoff(
    PCSTR pszUserName
    )
{
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = GPOClientProcessLogout(hGPConnection, pszUserName);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    if (hGPConnection != (HANDLE)NULL) {
        GPOClientCloseContext(hGPConnection);
    }

    return LWGetMacError(dwError);

error:

    MAC_AD_LOG_INFO("Failed to notify group policy about user logoff (%s) with error: %d", pszUserName, dwError);

    goto cleanup;
}

LONG
GetHomeDirectoryDockMCXValue(
    PMCXVALUE * ppMCXValueList
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PMCXVALUE pMCXValues = NULL;
    char szMCXValueFile[PATH_MAX] = { 0 };

    if (!ppMCXValueList)
    {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    sprintf(szMCXValueFile, "/etc/likewise/user-home-directory-in-dock.plist");

    dwError = ReadMCXValueFromFile(szMCXValueFile, &pMCXValues);
    BAIL_ON_MAC_ERROR(dwError);

    *ppMCXValueList = pMCXValues;
    pMCXValues = NULL;

cleanup:

    if (pMCXValues)
        FreeMCXValueList(pMCXValues);

    return LWGetMacError(dwError);

error:

    if (ppMCXValueList)
    {
        *ppMCXValueList = NULL;
    }

    goto cleanup;
}

static
void
FreeADUserInfo(
    PAD_USER_ATTRIBUTES pUserADAttrs
    )
{
    if (pUserADAttrs)
    {
        if (pUserADAttrs->pszDisplayName)
            LWFreeString(pUserADAttrs->pszDisplayName);

        if (pUserADAttrs->pszFirstName)
            LWFreeString(pUserADAttrs->pszFirstName);

        if (pUserADAttrs->pszLastName)
            LWFreeString(pUserADAttrs->pszLastName);

        if (pUserADAttrs->pszADDomain)
            LWFreeString(pUserADAttrs->pszADDomain);

        if (pUserADAttrs->pszKerberosPrincipal)
            LWFreeString(pUserADAttrs->pszKerberosPrincipal);

        if (pUserADAttrs->pszEMailAddress)
            LWFreeString(pUserADAttrs->pszEMailAddress);

        if (pUserADAttrs->pszMSExchHomeServerName)
            LWFreeString(pUserADAttrs->pszMSExchHomeServerName);

        if (pUserADAttrs->pszMSExchHomeMDB)
            LWFreeString(pUserADAttrs->pszMSExchHomeMDB);

        if (pUserADAttrs->pszTelephoneNumber)
            LWFreeString(pUserADAttrs->pszTelephoneNumber);

        if (pUserADAttrs->pszFaxTelephoneNumber)
            LWFreeString(pUserADAttrs->pszFaxTelephoneNumber);

        if (pUserADAttrs->pszMobileTelephoneNumber)
            LWFreeString(pUserADAttrs->pszMobileTelephoneNumber);

        if (pUserADAttrs->pszStreetAddress)
            LWFreeString(pUserADAttrs->pszStreetAddress);

        if (pUserADAttrs->pszPostOfficeBox)
            LWFreeString(pUserADAttrs->pszPostOfficeBox);

        if (pUserADAttrs->pszCity)
            LWFreeString(pUserADAttrs->pszCity);

        if (pUserADAttrs->pszState)
            LWFreeString(pUserADAttrs->pszState);

        if (pUserADAttrs->pszPostalCode)
            LWFreeString(pUserADAttrs->pszPostalCode);

        if (pUserADAttrs->pszCountry)
            LWFreeString(pUserADAttrs->pszCountry);

        if (pUserADAttrs->pszTitle)
            LWFreeString(pUserADAttrs->pszTitle);

        if (pUserADAttrs->pszCompany)
            LWFreeString(pUserADAttrs->pszCompany);

        if (pUserADAttrs->pszDepartment)
            LWFreeString(pUserADAttrs->pszDepartment);

        if (pUserADAttrs->pszHomeDirectory)
            LWFreeString(pUserADAttrs->pszHomeDirectory);

        if (pUserADAttrs->pszHomeDrive)
            LWFreeString(pUserADAttrs->pszHomeDrive);

        if (pUserADAttrs->pszPasswordLastSet)
            LWFreeString(pUserADAttrs->pszPasswordLastSet);

        if (pUserADAttrs->pszUserAccountControl)
            LWFreeString(pUserADAttrs->pszUserAccountControl);

        if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
            LWFreeString(pUserADAttrs->pszMaxMinutesUntilChangePassword);

        if (pUserADAttrs->pszMinMinutesUntilChangePassword)
            LWFreeString(pUserADAttrs->pszMinMinutesUntilChangePassword);

        if (pUserADAttrs->pszMaxFailedLoginAttempts)
            LWFreeString(pUserADAttrs->pszMaxFailedLoginAttempts);

        if (pUserADAttrs->pszAllowedPasswordHistory)
            LWFreeString(pUserADAttrs->pszAllowedPasswordHistory);

        if (pUserADAttrs->pszMinCharsAllowedInPassword)
            LWFreeString(pUserADAttrs->pszMinCharsAllowedInPassword);

        LWFreeMemory(pUserADAttrs);
    }
}

LONG
GetADUserInfo(
    uid_t uid,
    PAD_USER_ATTRIBUTES * ppadUserInfo
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PAD_USER_ATTRIBUTES padUserInfo = NULL;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;
    char      szUserAttrCacheFile[PATH_MAX] = { 0 };

    if (!uid)
    {
        MAC_AD_LOG_ERROR("Called with invalid parameter");
        goto cleanup;
    }

    dwError = LWAllocateMemory(sizeof(AD_USER_ATTRIBUTES), (PVOID *) &padUserInfo);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(szUserAttrCacheFile, "/var/lib/likewise/grouppolicy/user-cache/%ld/ad-user-attrs", (long) uid);

    /* Get user attributes that apply to user by parsing ad-user-attrs for specific user*/
    dwError = LWParseConfigFile(szUserAttrCacheFile,
                                &pSectionList,
                                FALSE);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "displayName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDisplayName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "givenName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFirstName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "sn",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszLastName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userDomain",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszADDomain = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Name Attributes",
                                            "userPrincipalName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszKerberosPrincipal = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "mail",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszEMailAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "msExchHomeServerName",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeServerName = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Email Attributes",
                                            "homeMDB",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMSExchHomeMDB = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "telephoneNumber",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "facsimileTelephoneNumber",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszFaxTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Phone Attributes",
                                            "mobile",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMobileTelephoneNumber = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "streetAddress",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszStreetAddress = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postOfficeBox",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostOfficeBox = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "l",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCity = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "st",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszState = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "postalCode",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPostalCode = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Address Attributes",
                                            "co",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCountry = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "title",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszTitle = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "company",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszCompany = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Work Attributes",
                                            "department",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszDepartment = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDirectory",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDirectory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "homeDrive",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszHomeDrive = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdLastSet",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszPasswordLastSet = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "userAccountControl",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszUserAccountControl = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "maxPwdAge",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdAge",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinMinutesUntilChangePassword = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "lockoutThreshhold",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMaxFailedLoginAttempts = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "pwdHistoryLength",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszAllowedPasswordHistory = pszValue;
        pszValue = NULL;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "User AD Network Settings Attributes",
                                            "minPwdLength",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS && pszValue)
    {
        padUserInfo->pszMinCharsAllowedInPassword = pszValue;
        pszValue = NULL;
    }

    dwError = MAC_AD_ERROR_SUCCESS;
    *ppadUserInfo = padUserInfo;
    padUserInfo = NULL;

cleanup:

    if (padUserInfo)
    {
        FreeADUserInfo(padUserInfo);
    }

    if (pSectionList)
    {
        LWFreeConfigSectionList(pSectionList);
    }

    if (pszValue)
    {
        LWFreeString(pszValue);
    }

    return LWGetMacError(dwError);

error:

    *ppadUserInfo = NULL;
    dwError = MAC_AD_ERROR_SUCCESS;

    goto cleanup;
}

LONG
GetConfigurationSettings(
    BOOLEAN * pbMergeModeMCX,
    BOOLEAN * pbEnableForceHomedirOnStartupDisk,
    BOOLEAN * pbUseADUNCForHomeLocation,
    PSTR *    ppszUNCProtocolForHomeLocation,
    PSTR *    ppszAllowAdministrationBy,
    BOOLEAN * pbMergeAdmins
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszValue = NULL;
    PCFGSECTION pSectionList = NULL;
    PSTR pszUNCProtocolForHomeLocation = NULL;
    PSTR pszAllowAdministrationBy = NULL;

    dwError = LWParseConfigFile("/etc/likewise/lwedsplugin.conf",
                                &pSectionList,
                                FALSE);
    if (dwError)
    {
        goto error;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "enable-mcx-merge-mode",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            if (!strcmp(pszValue, "yes") || !strcmp(pszValue, "true"))
            {
                *pbMergeModeMCX = TRUE;
            }
            else
            {
                *pbMergeModeMCX = FALSE;
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
        else
        {
            *pbMergeModeMCX = FALSE;
        }
    }
    else
    {
        *pbMergeModeMCX = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "enable-force-homedir-on-startup-disk",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            if (!strcmp(pszValue, "yes") || !strcmp(pszValue, "true"))
            {
                *pbEnableForceHomedirOnStartupDisk = TRUE;
            }
            else
            {
                *pbEnableForceHomedirOnStartupDisk = FALSE;
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
        else
        {
            *pbEnableForceHomedirOnStartupDisk = FALSE;
        }
    }
    else
    {
        *pbEnableForceHomedirOnStartupDisk = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "use-ad-unc-for-home-location",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            if (!strcmp(pszValue, "yes") || !strcmp(pszValue, "true"))
            {
                *pbUseADUNCForHomeLocation = TRUE;
            }
            else
            {
                *pbUseADUNCForHomeLocation = FALSE;
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
        else
        {
            *pbUseADUNCForHomeLocation = FALSE;
        }
    }
    else
    {
        *pbUseADUNCForHomeLocation = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "unc-protocol-for-home-location",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            if (!strcmp(pszValue, "afp") || !strcmp(pszValue, "AFP") ||
                !strcmp(pszValue, "smb") || !strcmp(pszValue, "SMB"))
            {
                dwError = LWAllocateString(pszValue, &pszUNCProtocolForHomeLocation);
                if (dwError)
                {
                    goto error;
                }
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
        else
        {
            dwError = LWAllocateString("smb", &pszUNCProtocolForHomeLocation);
            if (dwError)
            {
                goto error;
            }
        }
    }
    else
    {
        dwError = LWAllocateString("smb", &pszUNCProtocolForHomeLocation);
        if (dwError)
        {
            goto error;
        }
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "allow-administration-by",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            dwError = LWAllocateString(pszValue, &pszAllowAdministrationBy);
            if (dwError)
            {
                goto error;
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
    }
    else
    {
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    dwError = LWGetConfigValueBySectionName(pSectionList,
                                            "global",
                                            "enable-merge-admins",
                                            &pszValue);
    if (dwError == MAC_AD_ERROR_SUCCESS)
    {
        if (pszValue)
        {
            if (!strcmp(pszValue, "yes") || !strcmp(pszValue, "true"))
            {
                *pbMergeAdmins = TRUE;
            }
            else
            {
                *pbMergeAdmins = FALSE;
            }

            LWFreeString(pszValue);
            pszValue = NULL;
        }
        else
        {
            *pbMergeAdmins = FALSE;
        }
    }
    else
    {
        *pbMergeAdmins = FALSE;
        dwError = MAC_AD_ERROR_SUCCESS;
    }

    *ppszUNCProtocolForHomeLocation = pszUNCProtocolForHomeLocation;
    pszUNCProtocolForHomeLocation = NULL;
    *ppszAllowAdministrationBy = pszAllowAdministrationBy;
    pszAllowAdministrationBy = NULL;

cleanup:

    if (pSectionList) {
        LWFreeConfigSectionList(pSectionList);
    }

    if (pszValue) {
        LWFreeString(pszValue);
    }

    return LWGetMacError(dwError);

error:

    *pbMergeModeMCX = FALSE;
    *pbEnableForceHomedirOnStartupDisk = FALSE;
    *pbUseADUNCForHomeLocation = FALSE;
    *ppszUNCProtocolForHomeLocation = NULL;
    *ppszAllowAdministrationBy = NULL;
    *pbMergeAdmins = FALSE;

    dwError = 0;

    goto cleanup;
}

LONG
GetAccessCheckData(
    PSTR    pszAllowList,
    PVOID * ppAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PVOID pAccessData = NULL;
    DWORD  dwCount = 0;
    DWORD  dwIndex = 0;
    PCSTR  cp = NULL;
    PCSTR  cp2 = NULL;
    PSTR   cp3 = NULL;
    PSTR * ppczStrArray = NULL;

    for (cp = pszAllowList; *cp !=  0; cp++)
    {
        if (*cp == ',') dwCount++;
    }

    dwCount++;

    dwError = LWAllocateMemory((dwCount+1)*sizeof(PCSTR), (PVOID *)&ppczStrArray);
    BAIL_ON_MAC_ERROR(dwError);

    cp = pszAllowList;
    for ( ;; )
    {
         cp2 = strchr(cp, ',');
         if (cp2)
         {
             dwError = LWStrndup( cp, cp2 - cp, &cp3 );
             BAIL_ON_MAC_ERROR(dwError);
         }
         else
         {
             dwError = LWStrndup( cp, strlen(cp), &cp3 );
             BAIL_ON_MAC_ERROR(dwError);
         }

         LWStripWhitespace(cp3);

         if ( strlen(cp3) > 0 )
         {
             ppczStrArray[dwIndex++] = cp3;
         }
         else
         {
             LWFreeMemory(cp3);
         }

         if (!cp2) break;

         cp = ++cp2;
    }

    if ( dwIndex == 0 )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    if (gpfnLsaAccessGetData)
    {
        dwError = gpfnLsaAccessGetData((PCSTR *)ppczStrArray, &pAccessData);
    }
    else
    {
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_MAC_ERROR(dwError);

    *ppAccessData = pAccessData;
    pAccessData = NULL;

cleanup:

    if ( ppczStrArray )
    {
        for ( dwIndex = 0 ; ppczStrArray[dwIndex] != NULL ; dwIndex++ )
        {
            LWFreeString(ppczStrArray[dwIndex]);
        }

        LWFreeMemory(ppczStrArray);
    }

    return LWGetMacError(dwError);

error:

    *ppAccessData = NULL;

    goto cleanup;
}

LONG
CheckUserForAccess(
    PCSTR  pszUsername,
    PCVOID pAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;

    if (gpfnLsaAccessCheckData)
    {
        dwError = gpfnLsaAccessCheckData(pszUsername, pAccessData);
    }
    else
    {
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return LWGetMacError(dwError);

error:

    goto cleanup;
}

LONG
FreeAccessCheckData(
    PVOID pAccessData
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;

    if (gpfnLsaAccessFreeData)
    {
        dwError = gpfnLsaAccessFreeData(pAccessData);
    }
    else
    {
        dwError = MAC_AD_ERROR_LOOKUP_SYMBOL_FAILED;
    }
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return LWGetMacError(dwError);

error:

    goto cleanup;
}


