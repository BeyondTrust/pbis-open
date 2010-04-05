#include "includes.h"

#define MAX_USERS 100

CENTERROR
GPMultiStrsToByteArrayA(
    PSTR*    ppszInMultiSz,
    PBYTE*   ppOutBuf,
    SSIZE_T* pOutBufLen
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    SSIZE_T idx       = 0;
    SSIZE_T OutBufLen = 0;
    PBYTE   pOutBuf   = NULL;
    PBYTE   pCursor   = NULL;

    BAIL_ON_NULL_POINTER(ppszInMultiSz);
    BAIL_ON_NULL_POINTER(ppOutBuf);
    BAIL_ON_NULL_POINTER(pOutBufLen);

    // Determine total length of all strings in bytes
    for (; ppszInMultiSz[idx]; idx++)
    {
        OutBufLen += strlen(ppszInMultiSz[idx]) + 1;
    }

    OutBufLen++; // double null at end

    ceError = LW_RTL_ALLOCATE((PVOID*)&pOutBuf, BYTE,
    		                  sizeof(*pOutBuf) * OutBufLen);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (idx=0, pCursor = pOutBuf; ppszInMultiSz[idx]; idx++)
    {
        size_t len = strlen(ppszInMultiSz[idx]) + 1;

        memcpy(pCursor, ppszInMultiSz[idx], len);

        pCursor += len;
    }

    *pCursor = '\0';

   *ppOutBuf   = pOutBuf;
   *pOutBufLen = OutBufLen;

cleanup:

    return ceError;

error:

    if (pOutBuf)
    {
        LwRtlMemoryFree(pOutBuf);
    }

    if (ppOutBuf)
    {
        *ppOutBuf = NULL;
    }
    if (pOutBufLen)
    {
        *pOutBufLen = 0;
    }

    goto cleanup;
}

CENTERROR
GPMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PWSTR pszOutput = NULL;

    if (!pszInput) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *ppszOutput = pszOutput;

cleanup:

    return ceError;

error:

    *ppszOutput = NULL;

    goto cleanup;
}

CENTERROR
GPSetMultiStringToRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName, 
    PSTR pszValue
    )
{

    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDelim = ",";
    PSTR pszToken = NULL;
    PSTR pszValName = NULL;
    PSTR pszStrtokState = NULL;
    PCHAR *pszNames = NULL;
    PBYTE pData = NULL;
    SSIZE_T dwDataLen = 0;
    int i = 0,j=0;

    ceError = LwAllocateString(pszValue, &pszValName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RTL_ALLOCATE((PVOID*)&pszNames,VOID,sizeof(pszNames)* (MAX_USERS+1));
    BAIL_ON_CENTERIS_ERROR(ceError);

    pszToken = strtok_r(pszValName, pszDelim, &pszStrtokState);

    while (pszToken)
    {
        ceError = LwRtlCStringDuplicate((PSTR*)&pszNames[i++], pszToken);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if(i == MAX_USERS)
        {
            //Process the users within the range
            GPA_LOG_VERBOSE("Exceeding maximum user limit");
            break;
        }

        pszToken = strtok_r (NULL, pszDelim, &pszStrtokState);
    }

    pszNames[i] = NULL;

    ceError = GPMultiStrsToByteArrayA(
                                pszNames,
                                &pData,
                                &dwDataLen);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = RegSetValueExA(
                 hConnection,
                 hKey,
                (PCSTR)pszValueName,
                 0,
                 REG_MULTI_SZ,
                (const BYTE*)pData,
                 dwDataLen 
                );
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to set multi string value name %s in registry",pszValueName);
        ceError = 0;
    }

error:
    if (pData)
    {
        LwRtlMemoryFree(pData);
    }

    for(j=0;j<MAX_USERS+1;j++)
        LW_SAFE_FREE_STRING(pszNames[j]);

    RTL_FREE(&pszNames);

    LW_SAFE_FREE_STRING(pszValName);

    return ceError;
}

CENTERROR
GPSetStringToRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName, 
    PSTR pszValue
    )
{

    CENTERROR ceError = CENTERROR_SUCCESS;
    ceError = RegSetValueExA(
                 hConnection,
                 hKey,
                (PCSTR)pszValueName,
                (DWORD) 0,
                (DWORD) REG_SZ,
                (const BYTE*) pszValue,
                (DWORD) strlen(pszValue)+1
                );
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to set string value name %s in registry",pszValueName);
        ceError = 0;
    }

    return ceError;
}

CENTERROR
GPSetBoolToRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName, 
    BOOL bValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwValue = 0;

    if (bValue)
         dwValue = 1;

    ceError = RegSetValueExA(
                 hConnection,
                 hKey,
                (PCSTR)pszValueName,
                (DWORD) 0,
                (DWORD) REG_DWORD,
                (const BYTE*) &dwValue,
                sizeof(dwValue)
                );

    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to set bool value name %s in registry",pszValueName);
        ceError = 0;
    }

    return ceError;
}

CENTERROR
GPSetDwordToRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName, 
    DWORD dwValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = RegSetValueExA(
                 hConnection,
                 hKey,
                (PCSTR)pszValueName,
                (DWORD) 0,
                (DWORD) REG_DWORD,
                (const BYTE*) &dwValue,
                (DWORD) sizeof(dwValue)
                );
    if (ceError)
    {
        GPA_LOG_ERROR( "Unable to set dword value name %s in registry",pszValueName);
        ceError = 0;
    }

    return ceError;
}

CENTERROR
GPDeleteValueFromRegistry(
    HANDLE hConnection, 
    HKEY hKey, 
    PCSTR pszValueName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = RegDeleteKeyValueA(
                 hConnection,
                 hKey,
                 NULL,
                 pszValueName);
    if (ceError)
    {
        if(ceError != LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
            GPA_LOG_ERROR( "Unable to delete valuename %s from registry,Error Code %d",pszValueName,ceError);

        ceError = 0;
    }

    return ceError;
}

CENTERROR
GPGetRegOpenKey(
    PSTR pszKeyPath,
    PHANDLE phConnection,
    PHKEY   phKey
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey = NULL;
    PSTR pszDelim = "\\";
    PSTR pszToken = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszFullPath = NULL;

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
        //If key is not present then try to create it
        GPA_LOG_VERBOSE( "Trying to create registry key %s",pszKeyPath);

        ceError = LwAllocateString(pszKeyPath, &pszFullPath);
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszToken = strtok_r(pszFullPath, pszDelim, &pszStrtokState);
        while (pszToken)
        {
            ceError = RegCreateKeyExA(
                hReg,
                pRootKey,
                pszToken,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                NULL,
                &pNewKey,
                NULL);
            if(ceError == LWREG_ERROR_KEYNAME_EXIST)
            {
                ceError = RegOpenKeyExA(
                            hReg,
                            pRootKey,
                            pszToken,
                            0,
                            KEY_ALL_ACCESS,
                            &pNewKey);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (pRootKey)
            {
              ceError = RegCloseKey(hReg, pRootKey);
              BAIL_ON_CENTERIS_ERROR(ceError);
              pRootKey = NULL;
            }

            pRootKey = pNewKey;

            pszToken = strtok_r (NULL, pszDelim, &pszStrtokState);
        }
    }

    *phConnection = (HANDLE)hReg;
    *phKey = pNewKey;

cleanup:

    LW_SAFE_FREE_STRING(pszFullPath);
    return ceError;

error:

    RegCloseServer(hReg);

    *phConnection = NULL;
    *phKey = NULL;

    goto cleanup;
}

CENTERROR
UpdateBoolValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    BOOL bValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(bUseDefault) 
    {

        ceError = GPDeleteValueFromRegistry(
                     hReg,
                     hKey,
                     pszValueName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = GPSetBoolToRegistry(
                     hReg,
                     hKey,
                     pszValueName,
                     bValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
   return ceError;
}

CENTERROR
UpdateMultiStringValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(bUseDefault) 
    {

        ceError = GPDeleteValueFromRegistry(
                     hReg,
                     hKey,
                     pszValueName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = GPSetMultiStringToRegistry(
                     hReg,
                     hKey,
                     pszValueName,
                     pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
   return ceError;
}

CENTERROR
UpdateStringValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(bUseDefault) 
    {

        ceError = GPDeleteValueFromRegistry(
                     hReg,
                     hKey,
                     pszValueName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = GPSetStringToRegistry(
                     hReg,
                     hKey,
                     pszValueName,
                     pszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
   return ceError;
}

CENTERROR
UpdateDwordValueInRegistry( 
    HANDLE hReg,
    HKEY hKey, 
    BOOLEAN bUseDefault, 
    PCSTR pszValueName,
    DWORD dwValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(bUseDefault) 
    {

        ceError = GPDeleteValueFromRegistry(
                     hReg,
                     hKey,
                     pszValueName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = GPSetDwordToRegistry(
                     hReg,
                     hKey,
                     pszValueName,
                     dwValue);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
   return ceError;
}

BOOL
GetBoolValue( 
    PSTR pszValue
    )
{
    if( !strcmp((const char*)pszValue, (const char*)"true"))
        return (TRUE);
    else
        return (FALSE);
}
