/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

#define BAIL_ON_NON_LWREG_ERROR(dwError) \
        if (!(40700 <= dwError && dwError <= 41200)) {  \
           BAIL_ON_LWUTIL_ERROR(dwError);            \
        }
typedef enum
{
    LWGPTypeString,
    LWGPTypeDword,
    LWGPTypeBoolean
} LWGP_CONFIG_TYPE;

typedef struct __LWGP_CONFIG_TABLE
{
    PCSTR   pszName;
    LWGP_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    PVOID pValue;
} LWGP_CONFIG_TABLE, *PLWGP_CONFIG_TABLE;

typedef struct __LWGP_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    PSTR pszConfigKey;
} LWGP_CONFIG_REG, *PLWGP_CONFIG_REG;

DWORD
LWGPOpenConfig(
    PCSTR pszConfigKey,
    PLWGP_CONFIG_REG *ppReg
    );

DWORD
GetValuesFromRegistry( 
    PSTR pszGUIDKey,
    PGROUP_POLICY_CLIENT_EXTENSION *ppGPClientExtension
    );

DWORD
LWGPProcessConfig(
    PCSTR pszConfigKey,
    PLWGP_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    );

DWORD
LWGPReadConfigDword(
    PLWGP_CONFIG_REG pReg,
    PCSTR pszName,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    );

DWORD
LWGPReadConfigString(
    PLWGP_CONFIG_REG pReg,
    PCSTR   pszName,
    PSTR    *ppszValue
    );

DWORD
LWGPReadConfigBoolean(
    PLWGP_CONFIG_REG pReg,
    PCSTR pszName,
    PBOOLEAN pbValue
    );

VOID
LWGPCloseConfig(
    PLWGP_CONFIG_REG pReg
    );

void
FreeClientExtension(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension
    );

DWORD
GetGUIDString( 
    PSTR pszGUIDKey,
    PSTR *ppszGUIDString
    );

DWORD
GetCSEListFromRegistry(
    PSTR pszKeyPath,
    PGROUP_POLICY_CLIENT_EXTENSION * ppGroupPolicyClientExtensions
    )
{
    DWORD ceError = 0;
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
    BAIL_ON_LWUTIL_ERROR(ceError);

    ceError = RegOpenKeyExA(
                (HANDLE) hReg,
                NULL,
                HKEY_THIS_MACHINE,
                (DWORD) 0,
                (REGSAM) KEY_READ,
                (PHKEY) &pRootKey
                );
    if (ceError)
    {
        LWUTIL_LOG_ERROR( "Unable to open registry root key %s",HKEY_THIS_MACHINE);
        goto error;
    }

    ceError = RegOpenKeyExA(
                (HANDLE) hReg,
                (HKEY) pRootKey,
                pszKeyPath,
                (DWORD) 0,
                (REGSAM) KEY_READ,
                (PHKEY) &pNewKey
                );

    if (ceError)
    {
        LWUTIL_LOG_ERROR( "Unable to open registry key %s",pszKeyPath);
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
    BAIL_ON_LWUTIL_ERROR(ceError);

    ceError = LWAllocateStringPrintf(
                    &pszParentKeyPath,
                    "%s\\%s",
                    HKEY_THIS_MACHINE,
                    pszKeyPath);
    BAIL_ON_LWUTIL_ERROR(ceError);

    for(i=0; i<dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        ceError = LWAllocateMemory(
                      sizeof(GROUP_POLICY_CLIENT_EXTENSION),
                      (PVOID*) &pGPClientExtension);
        BAIL_ON_LWUTIL_ERROR(ceError);

        ceError = LWAllocateMemory(dwSubKeyLen*sizeof(*pszKeyName),
                                   (PVOID)&pszKeyName);
        BAIL_ON_LWUTIL_ERROR(ceError);

        ceError = RegEnumKeyExA((HANDLE)hReg,
                                pNewKey,
                                i,
                                pszKeyName,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_LWUTIL_ERROR(ceError);

        ceError = LWAllocateStringPrintf(
                        &pszFullKeyName,
                        "%s\\%s",
                        pszParentKeyPath,
                        pszKeyName);
        BAIL_ON_LWUTIL_ERROR(ceError);

        ceError = GetValuesFromRegistry(pszFullKeyName,&pGPClientExtension);
        BAIL_ON_LWUTIL_ERROR(ceError);

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

void
FreeClientExtension(
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension
    )
{
    LW_SAFE_FREE_STRING(pGPClientExtension->pszName);
    LW_SAFE_FREE_STRING(pGPClientExtension->pszGUID);

    LWFreeMemory(pGPClientExtension);
}


DWORD
GetValuesFromRegistry( 
    PSTR pszGUIDKey,
    PGROUP_POLICY_CLIENT_EXTENSION *ppGPClientExtension
    )
{
    DWORD ceError = 0;
    PGROUP_POLICY_CLIENT_EXTENSION pGPClientExtension = *ppGPClientExtension;
    PSTR pszToken = NULL;
    PSTR pszGUIDPath = NULL;
    PSTR pszAbsoluteKey = NULL;
    PSTR pszGUID = NULL;

    //get the guid by stripping the string
    ceError = GetGUIDString(pszGUIDKey,&pszGUID);
    BAIL_ON_LWUTIL_ERROR(ceError);

    ceError = LWAllocateMemory(strlen(pszGUID)+1, (PVOID*)&pGPClientExtension->pszGUID);
    BAIL_ON_LWUTIL_ERROR(ceError);
    sprintf(pGPClientExtension->pszGUID, "%s", pszGUID);

    //Now read all the parameters and store it accordingly
    LWGP_CONFIG_TABLE ConfigDescription[] =
    {
        {
            "Name",
            LWGPTypeString,
            0,
            -1,
            &(pGPClientExtension->pszName)
        },
        {
            "NoMachinePolicy",
            LWGPTypeBoolean,
            0,
            -1,
            &(pGPClientExtension->dwNoMachinePolicy)
        },
    };

    //Read from registry
    ceError = LWAllocateStringPrintf(&pszAbsoluteKey, "%s", pszGUIDKey);
    BAIL_ON_LWUTIL_ERROR(ceError);

    pszToken = (char*)strtok_r(pszAbsoluteKey,"\\",&pszGUIDPath);

    ceError = LWGPProcessConfig(
                pszGUIDPath,
                ConfigDescription,
                sizeof(ConfigDescription)/sizeof(ConfigDescription[0]));

    BAIL_ON_LWUTIL_ERROR(ceError);

    pGPClientExtension->pNext = NULL;

cleanup:
    LW_SAFE_FREE_STRING(pszAbsoluteKey);
    LW_SAFE_FREE_STRING(pszGUID);
    return ceError;
error:
    goto cleanup;
}

DWORD
GetGUIDString( 
    PSTR pszGUIDKey,
    PSTR *ppszGUIDString
    )
{
    DWORD ceError = 0;
    PSTR pszToken = NULL;
    PSTR pszLasts = NULL;
    PSTR pszKey = NULL;
    PSTR pszGUID = NULL;

    ceError = LWAllocateStringPrintf(&pszKey, "%s", pszGUIDKey);
    BAIL_ON_LWUTIL_ERROR(ceError);

    ceError = LWAllocateMemory(strlen(pszKey),(PVOID)&pszGUID);
    BAIL_ON_LWUTIL_ERROR(ceError);

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

DWORD
LWGPProcessConfig(
    PCSTR pszConfigKey,
    PLWGP_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    )
{
    DWORD dwError = 0;
    DWORD dwEntry;

    PLWGP_CONFIG_REG pReg = NULL;

    dwError = LWGPOpenConfig(pszConfigKey, &pReg);
    BAIL_ON_LWUTIL_ERROR(dwError);

    if ( pReg == NULL )
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        dwError = 0;
        switch (pConfig[dwEntry].Type)
        {
            case LWGPTypeString:
                dwError = LWGPReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].pValue);
                break;

            case LWGPTypeDword:
                dwError = LWGPReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case LWGPTypeBoolean:
                dwError = LWGPReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].pValue);
                break;

            default:
                break;
        }
        BAIL_ON_NON_LWREG_ERROR(dwError);
        dwError = 0;
    }

cleanup:
    LWGPCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:
    goto cleanup;    
}

DWORD
LWGPOpenConfig(
    PCSTR pszConfigKey,
    PLWGP_CONFIG_REG *ppReg
    )
{
    DWORD dwError = 0;
    PLWGP_CONFIG_REG pReg = NULL;

    dwError = LWAllocateMemory(sizeof(LWGP_CONFIG_REG), (PVOID*)&pReg);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LWAllocateString(pszConfigKey, &(pReg->pszConfigKey));
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = RegOpenServer(&(pReg->hConnection));
    if ( dwError )
    {
        dwError = 0;
        goto error;
    }

    dwError = RegOpenKeyExA(
                pReg->hConnection,
                NULL,
                HKEY_THIS_MACHINE,
                (DWORD) 0,
                (REGSAM) KEY_READ,
                (PHKEY) &(pReg->hKey)
                );

    if (dwError)
    {
        dwError = 0;
        goto error;
    }

cleanup:

    *ppReg = pReg;

    return dwError;

error:

    LWGPCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
LWGPCloseConfig(
    PLWGP_CONFIG_REG pReg
    )
{
    if ( pReg )
    {
        LW_SAFE_FREE_STRING(pReg->pszConfigKey);

        if ( pReg->hConnection )
        {
            if ( pReg->hKey )
            {
                RegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            RegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        LWFreeMemory(pReg);
    }
}

DWORD
LWGPReadConfigDword(
    PLWGP_CONFIG_REG pReg,
    PCSTR pszName,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;

    DWORD dwValue;
    DWORD dwSize;
    DWORD dwType;

    dwSize = sizeof(dwValue);
    dwError = RegGetValueA(
                pReg->hConnection,
                pReg->hKey,
                pReg->pszConfigKey,
                pszName,
                RRF_RT_REG_DWORD,
                &dwType,
                (PBYTE)&dwValue,
                &dwSize);
    if (!dwError)
    {
        if ( dwMin <= dwValue && dwValue <= dwMax)
            *pdwValue = dwValue;
    }

    dwError = 0;

    return dwError;
}

DWORD
LWGPReadConfigString(
    PLWGP_CONFIG_REG pReg,
    PCSTR   pszName,
    PSTR    *ppszValue
    )
{
    DWORD dwError = 0;
    
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType;
    DWORD dwSize;

    dwSize = sizeof(szValue);
    memset(szValue, 0, dwSize);
    dwError = RegGetValueA(
                pReg->hConnection,
                pReg->hKey,
                pReg->pszConfigKey,
                pszName,
                RRF_RT_REG_SZ,
                &dwType,
                szValue,
                &dwSize);
    if (!dwError)
    {
        dwError = LWAllocateString(szValue, &pszValue);
        BAIL_ON_LWUTIL_ERROR(dwError);

        LW_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;
    }

    dwError = 0;
    
cleanup: 

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:
    goto cleanup;    
}

DWORD
LWGPReadConfigBoolean(
    PLWGP_CONFIG_REG pReg,
    PCSTR pszName,
    PBOOLEAN pbValue
    )
{

    DWORD dwError = 0;

    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    dwError = LWGPReadConfigDword(
                pReg,
                pszName,
                0,
                -1,
                &dwValue);
    BAIL_ON_LWUTIL_ERROR(dwError);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return dwError;

error:
    goto cleanup;
}
