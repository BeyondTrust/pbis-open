#include "includes.h"

void
FreeClientExtension(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension
    )
{
    if (pGPClientExtension->pszName) {
        LwFreeString(pGPClientExtension->pszName);
    }

    if (pGPClientExtension->pszGUID) {
        LwFreeString(pGPClientExtension->pszGUID);
    }

    if (pGPClientExtension->pszDllName) {
        LwFreeString(pGPClientExtension->pszDllName);
    }

    if (pGPClientExtension->pszProcessGroupPolicyFunction) {
        LwFreeString(pGPClientExtension->pszProcessGroupPolicyFunction);
    }

    if (pGPClientExtension->pszResetGroupPolicyFunction) {
        LwFreeString(pGPClientExtension->pszResetGroupPolicyFunction);
    }

    LwFreeMemory(pGPClientExtension);
}

void
FreeClientExtensionList(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtensionList
    )
{
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension = pGPClientExtensionList;
    PGROUP_POLICY_CLIENT_EXTENSION pTmpExtension = NULL;

    while (pGPClientExtension != NULL) {
        pTmpExtension = pGPClientExtension;
        pGPClientExtension = pGPClientExtension->pNext;

        FreeClientExtension(pTmpExtension);
    }
}

CENTERROR
GetGUIDString( 
    PSTR pszGUIDKey,
    PSTR *ppszGUIDString
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszToken = NULL;
    PSTR pszLasts = NULL;
    PSTR pszKey = NULL;
    PSTR pszGUID = NULL;

    ceError = LwAllocateStringPrintf(&pszKey, "%s", pszGUIDKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateMemory(strlen(pszKey),(PVOID)&pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszToken = (char*)strtok_r(pszKey,"{",&pszLasts);

    strncpy(pszGUID,pszLasts,(strlen(pszLasts)-1));

    *ppszGUIDString = pszGUID;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    pszGUID = NULL;
    return ceError;
error:
    goto cleanup;
}

CENTERROR
GetValuesFromRegistry( 
    PSTR pszGUIDKey,
    PGROUP_POLICY_CLIENT_EXTENSION *ppGPClientExtension
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension = *ppGPClientExtension;
    PSTR pszToken = NULL;
    PSTR pszGUIDPath = NULL;
    PSTR pszAbsoluteKey = NULL;
    PSTR pszGUID = NULL;
    //uuid_t UUID;

    //get the guid by stripping the string
    ceError = GetGUIDString(pszGUIDKey,&pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = (uuid_parse(pszGUID, pGPClientExtension->UUID) < 0 ? CENTERROR_BAD_GUID : CENTERROR_SUCCESS);
    BAIL_ON_CENTERIS_ERROR(ceError);

    //Store UUID in uuid grouppolicy list struct
    //uuid_copy(pGPClientExtension->UUID, UUID);
    //Store guidstring with {} in guid grouppolicy list struct
    ceError = LwAllocateMemory(strlen(pszGUID)+3, (PVOID*)&pGPClientExtension->pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);
    sprintf(pGPClientExtension->pszGUID, "{%s}", pszGUID);

    //Now read all the parameters and store it accordingly
    GPA_CONFIG_TABLE ConfigDescription[] =
    {
        {
            "Name",
            FALSE,
            GPATypeString,
            0,
            -1,
            NULL,
            &(pGPClientExtension->pszName)
        },
        {
            "DllName",
            FALSE,
            GPATypeString,
            0,
            -1,
            NULL,
            &(pGPClientExtension->pszDllName)
        },
        {
            "EnableAsynchronousProcessing",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwEnableAsynchronousProcessing)
        },
        {
            "NoBackgroundPolicy",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwNoBackgroundPolicy)
        },
        {
            "NoGPOListChanges",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwNoGPOListChanges)
        },
        {
            "NoMachinePolicy",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwNoMachinePolicy)
        },
        {
            "NoSlowLink",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwNoSlowLink)
        },
        {
            "NoUserPolicy",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwNoUserPolicy)
        },
        {
            "PerUserLocalSettings",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwPerUserSettings)
        },
        {
            "RequireSuccessfulRegistry",
            FALSE,
            GPATypeBoolean,
            0,
            -1,
            NULL,
            &(pGPClientExtension->dwRequireSuccessfulRegistry)
        },
        {
            "ProcessGroupPolicy",
            FALSE,
            GPATypeString,
            0,
            -1,
            NULL,
            &(pGPClientExtension->pszProcessGroupPolicyFunction)
        },
        {
            "ResetGroupPolicy",
            FALSE,
            GPATypeString,
            0,
            -1,
            NULL,
            &(pGPClientExtension->pszResetGroupPolicyFunction)
        },
    };

    //Read from registry
    ceError = LwAllocateStringPrintf(&pszAbsoluteKey, "%s", pszGUIDKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszToken = (char*)strtok_r(pszAbsoluteKey,"\\",&pszGUIDPath);

    ceError = GPAProcessConfig(
                pszGUIDPath,
                pszGUIDPath,
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));

    BAIL_ON_CENTERIS_ERROR(ceError);

    pGPClientExtension->pNext = NULL;

cleanup:
    LW_SAFE_FREE_STRING(pszAbsoluteKey);
    LW_SAFE_FREE_STRING(pszGUID);
    return ceError;
error:
    goto cleanup;
}

CENTERROR
GetCSEListFromRegistry(
    PSTR pszKeyPath,
    PGROUP_POLICY_CLIENT_EXTENSION * ppGroupPolicyClientExtensions
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD dwSubKeyLen = 0;
    DWORD dwValuesCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    PSTR pszKeyName = NULL;
    PSTR pszFullKeyName = NULL;
    int i =0;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtensionList = NULL;
    PGROUP_POLICY_CLIENT_EXTENSION pGPCseTemp = NULL;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension = NULL;
    PSTR pszParentKeyPath = NULL;

    ceError = RegOpenServer(&hReg);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegOpenKeyExA(
                (HANDLE) hReg,
                NULL,
                HKEY_THIS_MACHINE,
                (DWORD) 0,
                (REGSAM) KEY_ALL_ACCESS,
                (PHKEY) &pRootKey
                );
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegOpenKeyExA(
                (HANDLE) hReg,
                (HKEY) pRootKey,
                pszKeyPath,
                (DWORD) 0,
                (REGSAM) KEY_ALL_ACCESS,
                (PHKEY) &pNewKey
                );

    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to open registry key %s",pszKeyPath);
        goto error;
    }

    ceError = RegQueryInfoKeyA(
                hReg,
                pNewKey,
                NULL,
                NULL,
                NULL,
                &dwSubKeyCount,
                &dwMaxSubKeyLen,
                NULL,
                &dwValuesCount,
                NULL,
                NULL,
                NULL,
                NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateStringPrintf(
                    &pszParentKeyPath,
                    "%s\\%s",
                    HKEY_THIS_MACHINE,
                    pszKeyPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for(i=0; i<dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        ceError = LwAllocateMemory(
                      sizeof(GROUP_POLICY_CLIENT_EXTENSION),
                      (PVOID*) &pGPClientExtension);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateMemory(dwSubKeyLen*sizeof(*pszKeyName),
                                   (PVOID)&pszKeyName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = RegEnumKeyExA((HANDLE)hReg,
                                pNewKey,
                                i,
                                pszKeyName,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = LwAllocateStringPrintf(
                        &pszFullKeyName,
                        "%s\\%s",
                        pszParentKeyPath,
                        pszKeyName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = GetValuesFromRegistry(pszFullKeyName,&pGPClientExtension);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(!pGPClientExtensionList)
        {
            pGPClientExtensionList = pGPClientExtension;
            pGPCseTemp = pGPClientExtension;
        }
        else
        {
            pGPCseTemp->pNext = pGPClientExtension;
            pGPCseTemp = pGPCseTemp->pNext;
        }
  
        LW_SAFE_FREE_STRING(pszKeyName);
        pszKeyName = NULL;
        LW_SAFE_FREE_STRING(pszFullKeyName);
        pszFullKeyName = NULL;
        pGPClientExtension = NULL;
    }

    *ppGroupPolicyClientExtensions = pGPClientExtensionList;

cleanup:

    RegCloseKey(hReg, pNewKey);
    pNewKey = NULL;

    RegCloseKey(hReg, pRootKey);
    pRootKey = NULL;

    RegCloseServer(hReg);
    hReg = NULL;

    LW_SAFE_FREE_STRING(pszKeyName);
    LW_SAFE_FREE_STRING(pszFullKeyName);
    LW_SAFE_FREE_STRING(pszParentKeyPath);

    return(ceError);

error:

    if (pGPClientExtensionList) {
        FreeClientExtensionList(pGPClientExtensionList);
    }

    goto cleanup;
}
