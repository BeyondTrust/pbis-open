/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lpmarshal.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Defines)
 *
 *        Marshal from DIRECTORY structures to lsass info levels
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LocalFindAttribute(
    PDIRECTORY_ENTRY      pEntry,
    PWSTR                 pwszAttrName,
    PDIRECTORY_ATTRIBUTE* ppAttr
    );

static
DWORD
LocalMarshalAccountFlagsToSecurityObject(
    PLSA_SECURITY_OBJECT pObject,
    DWORD dwAccountFlags,
    LONG64 llPwdLastSet,
    LONG64 llAcctExpiry
    )
{
    DWORD dwError = 0;
    BOOLEAN bPasswordNeverExpires = FALSE;
    BOOLEAN bAccountDisabled = FALSE;
    BOOLEAN bUserCanChangePassword = FALSE;
    BOOLEAN bAccountLocked = FALSE;
    BOOLEAN bAccountExpired = FALSE;
    BOOLEAN bPasswordExpired = FALSE;
    BOOLEAN bPromptPasswordChange = FALSE;
    LONG64 llMinPwdAge = 0;
    LONG64 llMaxPwdAge = 0;
    LONG64 llTimeToExpiry = 0;
    LONG64 llCurrentTime = 0;
    LONG64 llPwdChangeTime = 0;

    dwError = LocalCfgGetMinPasswordAge(&llMinPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgGetMaxPasswordAge(&llMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwGetNtTime((PULONG64)&llCurrentTime);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwAccountFlags & LOCAL_ACB_PWNOEXP)
    {
        bPasswordNeverExpires = TRUE;
    }

    if (dwAccountFlags & LOCAL_ACB_DISABLED)
    {
        bAccountDisabled = TRUE;
    }

    if (!bPasswordNeverExpires)
    {
        if (llCurrentTime > llPwdLastSet + llMaxPwdAge)
        {
            // Password has expired

            bPasswordExpired      = TRUE;
            bPromptPasswordChange = TRUE;
        }
        else
        {
            // Password has not expired yet but check if
            // we can prompt for password change

            dwError = LocalCfgGetPasswordChangeWarningTime(&llPwdChangeTime);
            BAIL_ON_LSA_ERROR(dwError);

            bPasswordExpired = FALSE;

            llTimeToExpiry = llMaxPwdAge - (llCurrentTime - llPwdLastSet);

            if (llTimeToExpiry <= llPwdChangeTime)
            {
                bPromptPasswordChange = TRUE;
            }
            else
            {
                bPromptPasswordChange = FALSE;
            }
        }
    }
    else
    {
        // Password never expires

        bPasswordExpired      = FALSE;
        bPromptPasswordChange = FALSE;
    }

    if (llAcctExpiry)
    {
        bAccountExpired = (llCurrentTime > llAcctExpiry) ? TRUE : FALSE;
    }

    if (llCurrentTime - llPwdLastSet < llMinPwdAge)
    {
        bUserCanChangePassword = FALSE;
    }
    else
    {
        bUserCanChangePassword = TRUE;
    }

    pObject->userInfo.qwPwdLastSet           = llPwdLastSet;
    pObject->userInfo.qwMaxPwdAge            = llMaxPwdAge;
    pObject->userInfo.qwPwdExpires           = llCurrentTime + llTimeToExpiry;
    pObject->userInfo.qwAccountExpires       = llAcctExpiry;
    pObject->userInfo.bIsAccountInfoKnown    = TRUE;
    pObject->userInfo.bAccountDisabled       = bAccountDisabled;
    pObject->userInfo.bAccountExpired        = bAccountExpired;
    pObject->userInfo.bAccountLocked         = bAccountLocked;
    pObject->userInfo.bPasswordExpired       = bPasswordExpired;
    pObject->userInfo.bPasswordNeverExpires  = bPasswordNeverExpires;
    pObject->userInfo.bPromptPasswordChange  = bPromptPasswordChange;
    pObject->userInfo.bUserCanChangePassword = bUserCanChangePassword;

error:

    return dwError;
}

DWORD
LocalMarshalAttrToInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PDWORD           pdwValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pdwValue = pAttrValue->data.ulValue;

cleanup:

    return dwError;

error:

    *pdwValue = 0;

    goto cleanup;
}

DWORD
LocalMarshalAttrToLargeInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PLONG64          pllValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pllValue = pAttrValue->data.llValue;

cleanup:

    return dwError;

error:

    *pllValue = 0;

    goto cleanup;
}

DWORD
LocalMarshalAttrToANSIString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PSTR             pszValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_ANSI_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pAttrValue->data.pszStringValue,
                    &pszValue);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToUnicodeString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PWSTR*           ppwszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PWSTR            pwszValue = NULL;
    DWORD            dwLen = 0;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pwszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwLen = wc16slen(pAttrValue->data.pwszStringValue);

    dwError = LwAllocateMemory(
                    (dwLen + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszValue);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy((PBYTE)pwszValue,
           (PBYTE)pAttrValue->data.pwszStringValue,
           dwLen * sizeof(wchar16_t));

    *ppwszValue = pwszValue;

cleanup:

    return dwError;

error:

    *ppwszValue = NULL;

    LW_SAFE_FREE_MEMORY(pwszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToANSIFromUnicodeString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PSTR             pszValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pwszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pAttrValue->data.pwszStringValue,
                    &pszValue);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToOctetStream(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PBYTE*           ppData,
    PDWORD           pdwDataLen
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PBYTE            pData = NULL;
    DWORD            dwDataLen = 0;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_OCTET_STREAM)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pOctetString ||
                 !pAttrValue->data.pOctetString->pBytes ||
                 !pAttrValue->data.pOctetString->ulNumBytes)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    pAttrValue->data.pOctetString->ulNumBytes,
                    (PVOID*)&pData);
    BAIL_ON_LSA_ERROR(dwError);

    dwDataLen = pAttrValue->data.pOctetString->ulNumBytes;

    memcpy(pData, pAttrValue->data.pOctetString->pBytes, dwDataLen);

    *ppData = pData;
    *pdwDataLen = dwDataLen;

cleanup:

    return dwError;

error:

    *ppData = NULL;
    *pdwDataLen = 0;

    LW_SAFE_FREE_MEMORY(pData);

    goto cleanup;
}

DWORD
LocalMarshalAttrToBOOLEAN(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PBOOLEAN         pbValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_BOOLEAN)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pbValue = pAttrValue->data.bBooleanValue;

cleanup:

    return dwError;

error:

    *pbValue = FALSE;

    goto cleanup;
}

DWORD
LocalMarshalAttrToSid(
    PDIRECTORY_ENTRY  pEntry,
    PWSTR             pwszAttrName,
    PSID             *ppSid
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PSID pSid = NULL;
    DWORD dwSidSize = 0;
    PSID pRetSid = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            ntStatus = RtlAllocateSidFromWC16String(
                                  &pSid,
                                  pAttrValue->data.pwszStringValue);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (pAttrValue->Type == DIRECTORY_ATTR_TYPE_ANSI_STRING)
        {
            ntStatus = RtlAllocateSidFromCString(
                                  &pSid,
                                  pAttrValue->data.pszStringValue);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwSidSize = RtlLengthSid(pSid);
    dwError = LwAllocateMemory(
                    dwSidSize,
                    OUT_PPVOID(&pRetSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCopySid(
                    dwSidSize,
                    pRetSid,
                    pSid);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSid = pRetSid;

cleanup:
    RTL_FREE(&pSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pRetSid);
    *ppSid = NULL;

    goto cleanup;
}

static
DWORD
LocalFindAttribute(
    PDIRECTORY_ENTRY      pEntry,
    PWSTR                 pwszAttrName,
    PDIRECTORY_ATTRIBUTE* ppAttr
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    DWORD iAttr = 0;

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, pwszAttrName))
        {
            break;
        }

        pAttr = NULL;
    }

    if (!pAttr)
    {
        dwError = LW_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppAttr = pAttr;

cleanup:

    return dwError;

error:

    *ppAttr = NULL;

    goto cleanup;
}

DWORD
LocalMarshalEntryToSecurityObject(
    PDIRECTORY_ENTRY pEntry,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    static WCHAR wszAttrNameObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    static WCHAR wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    static WCHAR wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    static WCHAR wszAttrNamePrimaryGroup[]   = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    static WCHAR wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    static WCHAR wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    static WCHAR wszAttrNameNTHash[]         = LOCAL_DIR_ATTR_NT_HASH;
    static WCHAR wszAttrNameLMHash[]         = LOCAL_DIR_ATTR_LM_HASH;
    static WCHAR wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    static WCHAR wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    static WCHAR wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    static WCHAR wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    static WCHAR wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    static WCHAR wszAttrNameAccountFlags[]   = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    static WCHAR wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    static WCHAR wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    static WCHAR wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    static WCHAR wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PLSA_SECURITY_OBJECT pObject = NULL;
    DWORD  dwObjectClass = 0;
    DWORD  dwAccountFlags = 0;
    LONG64 llAccountExpiry = 0;
    LONG64 llPasswordLastSet = 0;
    DWORD  dwUid = 0;
    DWORD  dwGid = 0;
    BOOLEAN enableUnixIds = TRUE;

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    wszAttrNameObjectClass,
                    &dwObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
        sizeof(*pObject),
        OUT_PPVOID(&pObject));
    BAIL_ON_LSA_ERROR(dwError);

    pObject->bIsLocal = TRUE;

    dwError = LocalCfgGetEnableUnixIds(&enableUnixIds);
    BAIL_ON_LSA_ERROR(dwError);

    pObject->enabled = enableUnixIds;

    switch (dwObjectClass)
    {
    case LOCAL_OBJECT_CLASS_USER:
        pObject->type = LSA_OBJECT_TYPE_USER;

        dwError = LocalMarshalAttrToInteger(
            pEntry,
            wszAttrNameUID,
            &dwUid);
        BAIL_ON_LSA_ERROR(dwError);
        
        pObject->userInfo.uid = (uid_t) dwUid;
        
        dwError = LocalMarshalAttrToInteger(
            pEntry,
            wszAttrNamePrimaryGroup,
            &dwGid);
        BAIL_ON_LSA_ERROR(dwError);
        
        pObject->userInfo.gid = (gid_t) dwGid;
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameSamAccountName,
            &pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNamePassword,
            &pObject->userInfo.pszPasswd);
        if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameGecos,
            &pObject->userInfo.pszGecos);
        if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameShell,
            &pObject->userInfo.pszShell);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    wszAttrNameHomedir,
                    &pObject->userInfo.pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameObjectSID,
            &pObject->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameUPN,
            &pObject->userInfo.pszUPN);
        if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
		    wszAttrNameDN,
		    &pObject->pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameNetBIOSDomain,
            &pObject->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToInteger(
            pEntry,
            wszAttrNameAccountFlags,
            &dwAccountFlags);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToLargeInteger(
            pEntry,
            wszAttrNameAccountExpiry,
            &llAccountExpiry);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToLargeInteger(
            pEntry,
            wszAttrNamePasswdLastSet,
            &llPasswordLastSet);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalMarshalAccountFlagsToSecurityObject(
            pObject,
            dwAccountFlags,
            llPasswordLastSet,
            llAccountExpiry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
            &pObject->userInfo.pszUnixName,
            "%s%c%s",
            pObject->pszNetbiosDomainName,
            LsaSrvDomainSeparator(),
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        if (LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszUPN))
        {
            dwError = LwAllocateStringPrintf(
                &pObject->userInfo.pszUPN,
                "%s@%s",
                pObject->pszSamAccountName,
                pObject->pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
            LwStrToUpper(pObject->userInfo.pszUPN + strlen(pObject->pszSamAccountName) + 1);
            
            pObject->userInfo.bIsGeneratedUPN = TRUE;
        }

        dwError = LocalMarshalAttrToOctetStream(
            pEntry,
            wszAttrNameNTHash,
            &pObject->userInfo.pNtHash,
            &pObject->userInfo.dwNtHashLen);
        if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToOctetStream(
            pEntry,
            wszAttrNameLMHash,
            &pObject->userInfo.pLmHash,
            &pObject->userInfo.dwLmHashLen);
        if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LOCAL_OBJECT_CLASS_GROUP:
        pObject->type = LSA_OBJECT_TYPE_GROUP;

        dwError = LocalMarshalAttrToInteger(
            pEntry,
            wszAttrNameGID,
            &dwGid);
        BAIL_ON_LSA_ERROR(dwError);
        
        pObject->groupInfo.gid = dwGid;
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameSamAccountName,
            &pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
		    wszAttrNameDN,
		    &pObject->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameObjectSID,
            &pObject->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LocalMarshalAttrToANSIFromUnicodeString(
            pEntry,
            wszAttrNameNetBIOSDomain,
            &pObject->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
            &pObject->groupInfo.pszUnixName,
            "%s%c%s",
            pObject->pszNetbiosDomainName,
            LsaSrvDomainSeparator(),
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:


    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}
