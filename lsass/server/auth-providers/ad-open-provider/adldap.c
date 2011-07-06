/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "adnetapi.h"

DWORD
ADGetDefaultDomainPrefixedName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pAlias,
    OUT PLSA_LOGIN_NAME_INFO* ppPrefixedName
    )
{
    DWORD dwError = 0;
    PSTR pDefaultPrefix = NULL;
    PSTR pNameCopy = NULL;
    PLSA_LOGIN_NAME_INFO pPrefixedName = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pPrefixedName),
                    (PVOID*)&pPrefixedName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetUserDomainPrefix(
                    pState,
                    &pDefaultPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pAlias, &pNameCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pPrefixedName->nameType = NameType_NT4;
    pPrefixedName->pszDomain = pDefaultPrefix;
    pPrefixedName->pszName = pNameCopy;

    *ppPrefixedName = pPrefixedName;

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pDefaultPrefix);
        LW_SAFE_FREE_STRING(pNameCopy);
        LW_SAFE_FREE_MEMORY(pPrefixedName);
        *ppPrefixedName = NULL;
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
ADGetDomainQualifiedString(
    PCSTR pszNetBIOSDomainName,
    PCSTR pszName,
    PSTR* ppszQualifiedName
    )
{
    DWORD dwError = 0;
    PSTR  pszQualifiedName = NULL;

    dwError = LwAllocateStringPrintf(
                    &pszQualifiedName,
                    "%s%c%s",
                    pszNetBIOSDomainName,
                    LsaSrvDomainSeparator(),
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrnToUpper(pszQualifiedName, strlen(pszNetBIOSDomainName));

    LwStrToLower(pszQualifiedName + strlen(pszNetBIOSDomainName) + 1);

    *ppszQualifiedName = pszQualifiedName;

cleanup:

    return dwError;

error:

    *ppszQualifiedName = NULL;

    LW_SAFE_FREE_STRING(pszQualifiedName);

    goto cleanup;
}

DWORD
ADGetLDAPUPNString(
    IN OPTIONAL HANDLE hDirectory,
    IN OPTIONAL LDAPMessage* pMessage,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszSamaccountName,
    OUT PSTR* ppszUPN,
    OUT PBOOLEAN pbIsGeneratedUPN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszUPN = NULL;
    BOOLEAN bIsGeneratedUPN = FALSE;

    if (hDirectory && pMessage)
    {
        pLd = LwLdapGetSession(hDirectory);

        ppszValues = (PSTR*)ldap_get_values(pLd, pMessage, AD_LDAP_UPN_TAG);
        if (ppszValues && ppszValues[0])
        {
            dwError = LwAllocateString(ppszValues[0], &pszUPN);
            BAIL_ON_LSA_ERROR(dwError);

            if (!index(pszUPN, '@'))
            {
                // Some domain users might have invalid UPNs in AD.
                // Fix it for them.
                LW_SAFE_FREE_STRING(pszUPN);
                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                // Do not touch the non-realm part, just the realm part
                // to make sure the realm conforms to spec.
                LsaPrincipalRealmToUpper(pszUPN);
            }
        }
    }

    if (!pszUPN)
    {
        dwError = LwAllocateStringPrintf(
                        &pszUPN,
                        "%s@%s",
                        pszSamaccountName,
                        pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        bIsGeneratedUPN = TRUE;

        // If we genereate, we do lower@UPPER regardless of whatever
        // SAM account name case was provided.  (Note: It may be that
        // we should preseve case from the SAM account name, but we
        // would need to make sure that the SAM account name provided
        // to this function is matches the case in AD and is not derived
        // from something the user typed in locally.
        LsaPrincipalNonRealmToLower(pszUPN);
        LsaPrincipalRealmToUpper(pszUPN);
    }

    *ppszUPN = pszUPN;
    *pbIsGeneratedUPN = bIsGeneratedUPN;

cleanup:
    if (ppszValues)
    {
        ldap_value_free(ppszValues);
    }
    return dwError;

error:
    *ppszUPN = NULL;

    LW_SAFE_FREE_STRING(pszUPN);

    goto cleanup;
}

DWORD
ADGetUserPrimaryGroupSid(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDomainDnsName,
    IN PCSTR pszUserDN,
    IN PCSTR pszUserObjectsid,
    OUT PSTR* ppszPrimaryGroupSID
    )
{
    DWORD dwError = 0;
    LDAP* pLd = NULL;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    PSTR pszPrimaryGroupSID = NULL;
    PSTR pszQuery = NULL;
    DWORD dwCount = 0;
    DWORD dwUserPrimaryGroupID = 0;
    LDAPMessage *pMessage = NULL;
    PSTR szAttributeListUserPrimeGID[] =
    {
        AD_LDAP_PRIMEGID_TAG,
        NULL
    };
    PSTR pszDirectoryRoot = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaAllocSecurityIdentifierFromString(
                            pszUserObjectsid,
                            &pUserSID);
    BAIL_ON_LSA_ERROR(dwError);

    // Find the user's primary group ID.
    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszUserDN,
                    LDAP_SCOPE_BASE,
                    "objectClass=*",
                    szAttributeListUserPrimeGID,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                      pLd,
                      pMessage);
    if (dwCount != 1)
    {
        dwError = LW_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwLdapGetUInt32(
                hDirectory,
                pMessage,
                AD_LDAP_PRIMEGID_TAG,
                &dwUserPrimaryGroupID);
    BAIL_ON_LSA_ERROR(dwError);

     // Find the primary group's SID.
    dwError = LsaSetSecurityIdentifierRid(
                pUserSID,
                dwUserPrimaryGroupID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierString(
                pUserSID,
                &pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPrimaryGroupSID = pszPrimaryGroupSID;

cleanup:
    LW_SAFE_FREE_STRING(pszQuery);
    LW_SAFE_FREE_STRING(pszDirectoryRoot);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    if (pUserSID)
    {
        LsaFreeSecurityIdentifier(pUserSID);
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPrimaryGroupSID);
    *ppszPrimaryGroupSID = NULL;

    goto cleanup;
}

DWORD
ADFindComputerDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR pszSamAccountName,
    PCSTR pszDomainName,
    PSTR* ppszComputerDN
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    PSTR pszQuery = NULL;
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszComputerDN = NULL;
    PSTR pszEscapedUpperSamAccountName = NULL;
    HANDLE hDirectory = NULL;

    dwError = LwLdapConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapEscapeString(
                &pszEscapedUpperSamAccountName,
                pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszEscapedUpperSamAccountName);

    dwError = LwAllocateStringPrintf(&pszQuery,
                                      "(sAMAccountName=%s)",
                                      pszEscapedUpperSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        dwError = LW_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszComputerDN))
    {
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszComputerDN = pszComputerDN;

cleanup:
    LW_SAFE_FREE_STRING(pszEscapedUpperSamAccountName);
    LW_SAFE_FREE_STRING(pszDirectoryRoot);
    LW_SAFE_FREE_STRING(pszQuery);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    *ppszComputerDN = NULL;
    LW_SAFE_FREE_STRING(pszComputerDN);

    goto cleanup;
}

DWORD
ADGetCellInformation(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    PSTR*  ppszCellDN
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszCellDN = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_ONELEVEL,
                    "(name=$LikewiseIdentityCell)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LW_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
        dwError = LW_ERROR_INTERNAL;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszCellDN))
    {
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszCellDN = pszCellDN;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    *ppszCellDN = NULL;

    LW_SAFE_FREE_STRING(pszCellDN);

    goto cleanup;
}

DWORD
ADGetDomainMaxPwdAge(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDomainName,
    PUINT64 pMaxPwdAge)
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {
            AD_LDAP_MAX_PWDAGE_TAG,
            NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszDirectoryRoot = NULL;
    int64_t int64MaxPwdAge = 0;
    HANDLE hDirectory = NULL;

    dwError = LwLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDirectoryRoot,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LW_ERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        dwError = LW_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //process "maxPwdAge"
    dwError = LwLdapGetInt64(
                hDirectory,
                pMessage,
                AD_LDAP_MAX_PWDAGE_TAG,
                &int64MaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    if (int64MaxPwdAge == 0x8000000000000000LL)
    {
        *pMaxPwdAge = 0;
    }
    else
    {
        *pMaxPwdAge = (UINT64) (int64MaxPwdAge < 0 ? -int64MaxPwdAge : int64MaxPwdAge);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszDirectoryRoot);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADGetConfigurationMode(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    ADConfigurationMode* pADConfMode
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {AD_LDAP_DESCRIPTION_TAG, NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    ADConfigurationMode adConfMode = NonSchemaMode;
    HANDLE hDirectory = NULL;

    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pConn);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT){
        dwError = LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LW_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
        dwError = LW_ERROR_INTERNAL;
    }

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DESCRIPTION_TAG,
                    &ppszValues,
                    &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNumValues; i++)
    {
        if (!strncasecmp(ppszValues[i], "use2307Attrs=", sizeof("use2307Attrs=")-1))
        {
           PSTR pszValue = ppszValues[i] + sizeof("use2307Attrs=") - 1;
           if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) && !strcasecmp(pszValue, "true")) {
              adConfMode = SchemaMode;
              break;
           }
        }
    }

    *pADConfMode = adConfMode;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    if (ppszValues) {
        LwFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *pADConfMode = UnknownMode;

    goto cleanup;
}

#define AD_GUID_SIZE 16

DWORD
ADGuidStrToHex(
        PCSTR pszStr,
        PSTR* ppszHexStr)
{
   DWORD dwError = 0;
   PSTR pszHexStr = NULL;
   PSTR pszUUIDStr = NULL;
   int iValue = 0, jValue = 0;
   uuid_t uuid= {0};
   unsigned char temp;

   BAIL_ON_INVALID_STRING(pszStr);

   dwError = LwAllocateString(
                 pszStr,
                 &pszUUIDStr);
   BAIL_ON_LSA_ERROR(dwError);

   if (uuid_parse(pszUUIDStr, uuid) < 0) {
       dwError = LW_ERROR_INVALID_OBJECTGUID;
       BAIL_ON_LSA_ERROR(dwError);
   }

   for(iValue = 0; iValue < 2; iValue++){
       temp = uuid[iValue];
       uuid[iValue] = uuid[3-iValue];
       uuid[3-iValue] = temp;
   }
   temp = uuid[4];
   uuid[4] = uuid[5];
   uuid[5] = temp;

   temp = uuid[6];
   uuid[6] = uuid[7];
   uuid[7] = temp;

   dwError = LwAllocateMemory(
                sizeof(CHAR)*(AD_GUID_SIZE*3+1),
               (PVOID*)&pszHexStr);
   BAIL_ON_LSA_ERROR(dwError);

   for (iValue = 0, jValue = 0; jValue < AD_GUID_SIZE; ){
       if (iValue % 3 == 0){
           *((char*)(pszHexStr+iValue++)) = '\\';
       }
       else{
           sprintf((char*)pszHexStr+iValue, "%.2X", uuid[jValue]);
           iValue += 2;
           jValue++;
       }
   }

   *ppszHexStr = pszHexStr;

cleanup:

    LW_SAFE_FREE_STRING(pszUUIDStr);

    return dwError;

error:

    *ppszHexStr = NULL;

    LW_SAFE_FREE_STRING(pszHexStr);

    goto cleanup;
}

DWORD
ADCopyAttributeList(
    PSTR   szAttributeList[],
    PSTR** pppOutputAttributeList)
{
    DWORD dwError = 0;
    size_t sAttrListSize = 0, iValue = 0;
    PSTR* ppOutputAttributeList = NULL;

    if (!szAttributeList){
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (szAttributeList[sAttrListSize]){
        sAttrListSize++;
    }
    sAttrListSize++;

    dwError = LwAllocateMemory(
                sAttrListSize * sizeof(PSTR),
                (PVOID*)&ppOutputAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < sAttrListSize - 1; iValue++){
        dwError = LwAllocateString(
                      szAttributeList[iValue],
                      &ppOutputAttributeList[iValue]);
        BAIL_ON_LSA_ERROR(dwError);
    }
    ppOutputAttributeList[iValue] = NULL;

    *pppOutputAttributeList = ppOutputAttributeList;

cleanup:

    return dwError;

error:
    LwFreeNullTerminatedStringArray(ppOutputAttributeList);

    *pppOutputAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserOrGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTCLASS_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_HOMEDIR_TAG,
            AD_LDAP_SHELL_TAG,
            AD_LDAP_GECOS_TAG,
            AD_LDAP_SEC_DESC_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            AD_LDAP_ALIAS_TAG,
            AD_LDAP_DISPLAY_NAME_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTCLASS_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTCLASS_TAG,
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_PRIMEGID_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_USER_CTRL_TAG,
             AD_LDAP_PWD_LASTSET_TAG,
             AD_LDAP_ACCOUT_EXP_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LwFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_HOMEDIR_TAG,
            AD_LDAP_SHELL_TAG,
            AD_LDAP_GECOS_TAG,
            AD_LDAP_SEC_DESC_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_ALIAS_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_PRIMEGID_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_USER_CTRL_TAG,
             AD_LDAP_PWD_LASTSET_TAG,
             AD_LDAP_ACCOUT_EXP_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LwFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppPseudoAttributeList = NULL;

    PSTR szPseudoAttributeListSchema[] =
        {
                AD_LDAP_UID_TAG,
                AD_LDAP_GID_TAG,
                AD_LDAP_NAME_TAG,
                AD_LDAP_PASSWD_TAG,
                AD_LDAP_HOMEDIR_TAG,
                AD_LDAP_SHELL_TAG,
                AD_LDAP_GECOS_TAG,
                AD_LDAP_SEC_DESC_TAG,
                AD_LDAP_KEYWORDS_TAG,
                AD_LDAP_ALIAS_TAG,
                NULL
        };

    PSTR szPseudoAttributeListNonSchema[] =
         {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
         };

    switch (adConfMode)
    {
        case SchemaMode:
            dwError = ADCopyAttributeList(
                            szPseudoAttributeListSchema,
                            &ppPseudoAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case NonSchemaMode:
            dwError = ADCopyAttributeList(
                             szPseudoAttributeListNonSchema,
                             &ppPseudoAttributeList);
             BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppPseudoAttributeList = ppPseudoAttributeList;

cleanup:
    return dwError;

error:
     LwFreeNullTerminatedStringArray(ppPseudoAttributeList);
    *pppPseudoAttributeList = NULL;
    goto cleanup;
}

//
// DWORD dwID - this is assumed to be a hashed UID or GID.
//
DWORD
UnprovisionedModeMakeLocalSID(
    PCSTR pszDomainSID,
    DWORD dwID,
    PSTR* ppszLocalSID
    )
{
    DWORD dwError = 0;
    PSTR pszUnhashedLocalSID = NULL;
    DWORD dwUnhashedLocalRID = 0;
    DWORD dwHashedLocalRID = 0;
    PLSA_SECURITY_IDENTIFIER pUnhashedLocalSID = NULL;

    dwUnhashedLocalRID = dwID & 0x0007FFFF;

    dwError = LwAllocateStringPrintf(&pszUnhashedLocalSID,
                    "%s-%u",
                    pszDomainSID,
                    dwUnhashedLocalRID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromString(
                    pszUnhashedLocalSID,
                    &pUnhashedLocalSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierHashedRid(
                    pUnhashedLocalSID,
                    &dwHashedLocalRID);
    BAIL_ON_LSA_ERROR(dwError);

    //The user of this function is expected to provide
    //a hashed ID; applying the hash algorithm against
    //it and the root domain SID should not alter its value.
    //If the ID is below 1000, however, it likely represents
    //a builtin object like "Administrator" or Guests, and therefore
    //will use a domain like S-1-5-32, not the root domain
    //The check attempted below would have no meaning, since the domain
    //is not known.
    //TODO: use logic from list of well-known SID's to check SID validity
    //see: http://support.microsoft.com/kb/243330
    if (dwHashedLocalRID != dwID)
    {
        if (dwID >= 1000)
        {
            dwError = LW_ERROR_NO_SUCH_OBJECT;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else  //dwID < 1000.  Try again using domain for builtin SIDs
        {
            PCSTR pszBuiltinDomainSID = "S-1-5-32";
            LW_SAFE_FREE_STRING(pszUnhashedLocalSID);

            if (pUnhashedLocalSID)
            {
                LsaFreeSecurityIdentifier(pUnhashedLocalSID);
                pUnhashedLocalSID = NULL;
            }

            dwError = LwAllocateStringPrintf(&pszUnhashedLocalSID,
                            "%s-%u",
                            pszBuiltinDomainSID,
                            dwUnhashedLocalRID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocSecurityIdentifierFromString(
                            pszUnhashedLocalSID,
                            &pUnhashedLocalSID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaGetSecurityIdentifierHashedRid(
                            pUnhashedLocalSID,
                            &dwHashedLocalRID);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwHashedLocalRID != dwID)
            {
                dwError = LW_ERROR_NO_SUCH_OBJECT;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *ppszLocalSID = pszUnhashedLocalSID;

cleanup:

    if (pUnhashedLocalSID != NULL)
    {
        LsaFreeSecurityIdentifier(pUnhashedLocalSID);
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszUnhashedLocalSID);
    *ppszLocalSID = NULL;

    goto cleanup;

}

DWORD
ADGetGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_MEMBER_TAG,
            AD_LDAP_DISPLAY_NAME_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_MEMBER_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_MEMBER_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LwFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetGroupPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppPseudoAttributeList = NULL;

    PSTR szPseudoAttributeListSchema[] =
        {
                AD_LDAP_GID_TAG,
                AD_LDAP_NAME_TAG,
                AD_LDAP_PASSWD_TAG,
                AD_LDAP_KEYWORDS_TAG,
                AD_LDAP_MEMBER_TAG,
                AD_LDAP_SAM_NAME_TAG,
                AD_LDAP_DISPLAY_NAME_TAG,
                NULL
        };

    PSTR szPseudoAttributeListNonSchema[] =
         {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
         };

    switch (adConfMode)
    {
        case SchemaMode:
            dwError = ADCopyAttributeList(
                            szPseudoAttributeListSchema,
                            &ppPseudoAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case NonSchemaMode:
            dwError = ADCopyAttributeList(
                             szPseudoAttributeListNonSchema,
                             &ppPseudoAttributeList);
             BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppPseudoAttributeList = ppPseudoAttributeList;

cleanup:
    return dwError;

error:
     LwFreeNullTerminatedStringArray(ppPseudoAttributeList);
    *pppPseudoAttributeList = NULL;
    goto cleanup;
}

static
VOID
DestroyQueryListEntry(
    IN OUT PLSA_AD_QUERY_LISTS_ENTRY* ppEntry
    )
{
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = *ppEntry;
    if (pEntry)
    {
        LwFreeStringArray(pEntry->ppszQueryValues, pEntry->dwQueryCount);
        LwFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
CreateQueryListEntry(
    OUT PLSA_AD_QUERY_LISTS_ENTRY* ppEntry,
    IN DWORD dwQueryCount,
    IN PSTR* ppszQueryValues
    )
{
    DWORD dwError = 0;
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = NULL;

    dwError = LwAllocateMemory(sizeof(*pEntry), (PVOID*)&pEntry);
    BAIL_ON_LSA_ERROR(dwError);

    pEntry->dwQueryCount = dwQueryCount;
    pEntry->ppszQueryValues = ppszQueryValues;

    *ppEntry = pEntry;

cleanup:
    return dwError;

error:
    *ppEntry = NULL;
    DestroyQueryListEntry(&pEntry);
    goto cleanup;
}

// Give an attribute name "pszAttributeName"
// Return all the values of the attribute for a given DN "pszDN"
// The number of values can be more than 1500 since "ranging" is handled correctly in the routine
// Now accept whether to do an extended DN search and whether to parse result to get Sids
DWORD
ADLdap_GetAttributeValuesList(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDN,
    IN PCSTR pszAttributeName,
    IN BOOLEAN bDoExtDnSearch,
    IN BOOLEAN bDoSidParsing,
    OUT PDWORD pdwTotalCount,
    OUT PSTR** pppszValues
    )
{
    DWORD dwError = 0;
    // Do not free "szAttributeList"
    PSTR szAttributeList[2] = {NULL,NULL};
    PSTR* ppszValuesTotal = NULL;
    PSTR* ppszValues = NULL;
    LDAPMessage* pMessage = NULL;
    DWORD dwCount = 0;
    DWORD dwTotalCount = 0;
    PDLINKEDLIST pList = NULL;
    PDLINKEDLIST pNode = NULL;
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = NULL;
    PSTR pszRangeAttr = NULL;
    LDAP* pLd = NULL;
    BerElement* pBer = NULL;
    PSTR pszRetrievedAttr = NULL;
    //Do Not free "pszRetrievedRangeAttr"
    PSTR pszRetrievedRangeAttr = NULL;
    BOOLEAN bIsEnd = FALSE;
    DWORD iValues = 0;
    DWORD iValuesTotal = 0;
    PSTR pszAttributeRangedName = NULL;
    HANDLE hDirectory = NULL;

    BAIL_ON_INVALID_STRING(pszAttributeName);
    szAttributeList[0] = (PSTR)pszAttributeName;

    for (;;)
    {
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        if (!bDoExtDnSearch && bDoSidParsing)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (bDoExtDnSearch)
        {
            dwError = LsaDmLdapDirectoryExtendedDNSearch(
                            pConn,
                            pszDN,
                            "(objectClass=*)",
                            szAttributeList,
                            LDAP_SCOPE_BASE,
                            &hDirectory,
                            &pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaDmLdapDirectorySearch(
                            pConn,
                            pszDN,
                            LDAP_SCOPE_BASE,
                            "(objectClass=*)",
                            szAttributeList,
                            &hDirectory,
                            &pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pLd = LwLdapGetSession(hDirectory);

        dwError = LwLdapGetStringsWithExtDnResult(
                        hDirectory,
                        pMessage,
                        pszAttributeName,
                        bDoSidParsing,
                        &ppszValues,
                        &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppszValues && dwCount)
        {
            if (pList)
            {
                // This is the case where we started out getting
                // ranged info but the info became non-ranged.
                // We might actually want to allow this to handle
                // a case where the membership list is trimmed
                // while we are enumerating.
                dwError = LW_ERROR_LDAP_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwTotalCount = dwCount;
            ppszValuesTotal = ppszValues;

            dwCount = 0;
            ppszValues = NULL;

            break;
        }

        if (pszRetrievedAttr)
        {
            ldap_memfree(pszRetrievedAttr);
            pszRetrievedAttr = NULL;
        }

        if (pBer)
        {
             ber_free(pBer, 0);
        }

        LW_SAFE_FREE_STRING(pszAttributeRangedName);

        dwError = LwAllocateStringPrintf(
                      &pszAttributeRangedName,
                      "%s;Range=",
                      pszAttributeName);
        BAIL_ON_LSA_ERROR(dwError);

        pszRetrievedAttr = ldap_first_attribute(pLd, pMessage, &pBer);
        while (pszRetrievedAttr)
        {
            if (!strncasecmp(pszRetrievedAttr, pszAttributeRangedName, strlen(pszAttributeRangedName)))
            {
                pszRetrievedRangeAttr = pszRetrievedAttr;
                break;
            }
            ldap_memfree(pszRetrievedAttr);
            pszRetrievedAttr = NULL;

            pszRetrievedAttr = ldap_next_attribute(pLd, pMessage, pBer);
        }

        if (!pszRetrievedRangeAttr)
        {
            // This happens when we have an group with no members,
            break;
        }

        if ('*' == pszRetrievedRangeAttr[strlen(pszRetrievedRangeAttr)-1])
        {
            bIsEnd = TRUE;
        }

        dwError = LwLdapGetStringsWithExtDnResult(
                        hDirectory,
                        pMessage,
                        pszRetrievedRangeAttr,
                        bDoSidParsing,
                        &ppszValues,
                        &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalCount += dwCount;

        dwError = CreateQueryListEntry(
                        &pEntry,
                        dwCount,
                        ppszValues);
        BAIL_ON_LSA_ERROR(dwError);
        ppszValues = NULL;
        dwCount = 0;

        dwError = LsaDLinkedListPrepend(&pList, pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;

        if (bIsEnd)
        {
            break;
        }

        LW_SAFE_FREE_STRING(pszRangeAttr);

        dwError = LwAllocateStringPrintf(
                        &pszRangeAttr,
                        "%s%u-*",
                        pszAttributeRangedName,
                        dwTotalCount);
        BAIL_ON_LSA_ERROR(dwError);

        szAttributeList[0] = pszRangeAttr;
    }

    if (pList && !ppszValuesTotal)
    {
        dwError = LwAllocateMemory(
                        sizeof(*ppszValuesTotal) * dwTotalCount,
                        (PVOID*)&ppszValuesTotal);
        BAIL_ON_LSA_ERROR(dwError);

        for (pNode = pList; pNode; pNode = pNode->pNext)
        {
            PLSA_AD_QUERY_LISTS_ENTRY pEntry = (PLSA_AD_QUERY_LISTS_ENTRY)pNode->pItem;

            for (iValues = 0; iValues < pEntry->dwQueryCount; iValues++)
            {
                ppszValuesTotal[iValuesTotal] = pEntry->ppszQueryValues[iValues];
                pEntry->ppszQueryValues[iValues] = NULL;
                iValuesTotal++;
            }
        }
    }

    *pdwTotalCount = dwTotalCount;
    *pppszValues = ppszValuesTotal;

cleanup:
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (pszRetrievedAttr)
    {
        ldap_memfree(pszRetrievedAttr);
    }

    if (pBer)
    {
        ber_free(pBer, 0);
    }

    LwFreeStringArray(ppszValues, dwCount);
    DestroyQueryListEntry(&pEntry);
    LW_SAFE_FREE_STRING(pszAttributeRangedName);
    LW_SAFE_FREE_STRING(pszRangeAttr);

    for (pNode = pList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_QUERY_LISTS_ENTRY pEntry = (PLSA_AD_QUERY_LISTS_ENTRY)pNode->pItem;
        DestroyQueryListEntry(&pEntry);
    }
    LsaDLinkedListFree(pList);

    return dwError;

error:
    LwFreeStringArray(ppszValuesTotal, iValuesTotal);

    *pdwTotalCount = 0;
    *pppszValues = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetGroupMembers(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomainName,
    IN PCSTR pszSid,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSidCount = 0;
    PSTR pszDnsDomainName = NULL;
    PLSA_SECURITY_OBJECT pGroupObj = NULL;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    PSTR *ppszLDAPValues = NULL;
    size_t sFoundCount = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    dwError = AD_FindObjectBySid(
                    pContext,
                    pszSid,
                    &pGroupObj);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupObj->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapGetDomainName(
                  pContext->pState->hDmState,
                  pszDomainName,
                  &pszDnsDomainName,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapOpenDc(
                  pContext,
                  pszDnsDomainName,
                  &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_GetAttributeValuesList(
                    pConn,
                    pGroupObj->pszDN,
                    AD_LDAP_MEMBER_TAG,
                    TRUE,
                    TRUE,
                    &dwSidCount,
                    &ppszLDAPValues);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectsBySidList(
                 pContext,
                 dwSidCount,
                 ppszLDAPValues,
                 &sFoundCount,
                 &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    *psCount = sFoundCount;
    *pppResults = ppResults;

cleanup:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    ADCacheSafeFreeObject(&pGroupObj);
    LwFreeStringArray(ppszLDAPValues, dwSidCount);
    LsaDmLdapClose(pConn);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    LSA_LOG_ERROR("Failed to find group's members of objectSid=%s. [error code:%u]",
                  LSA_SAFE_LOG_STRING(pszSid), dwError);

    ADCacheSafeFreeObjectList((DWORD)sFoundCount, &ppResults);
    goto cleanup;
}

DWORD
ADLdap_GetObjectGroupMembership(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT int* piPrimaryGroupIndex,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError =  0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    PSTR pszFullDomainName = NULL;
    INT i = 0;
    PLSA_SECURITY_OBJECT* ppGroupInfoList = NULL;
    size_t sNumGroupsFound = 0;
    int    iPrimaryGroupIndex = -1;
    DWORD gcMembershipCount = 0;
    PSTR* ppGcMembershipList = NULL;
    DWORD dcMembershipCount = 0;
    PSTR* ppDcMembershipList = NULL;
    PLW_HASH_TABLE pGroupHash = NULL;
    LSA_TRUST_DIRECTION trustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE trustMode = LSA_TRUST_MODE_UNKNOWN;
    DWORD index = 0;
    DWORD totalSidCount = 0;
    PSTR* ppTotalSidList = NULL;

    // If we cannot get dn, then we cannot get DN information for this objects, hence BAIL
    if (LW_IS_NULL_OR_EMPTY_STR(pObject->pszDN))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwLdapConvertDNToDomain(
                 pObject->pszDN,
                 &pszFullDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    // Note that this function is called only for 2-way trusts.  However,
    // the trust could be an external trust or a forest trust.  We can only
    // query the GC if there is a forest trust.

    dwError = AD_DetermineTrustModeandDomainName(
                    pContext->pState,
                    pszFullDomainName,
                    &trustDirection,
                    &trustMode,
                    NULL,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_ASSERT(LSA_TRUST_DIRECTION_TWO_WAY == trustDirection ||
            LSA_TRUST_DIRECTION_SELF == trustDirection);

    if (trustMode != LSA_TRUST_MODE_EXTERNAL)
    {
        // Get forest info from domain's GC since there is a forest trust.
        // This will only include universal group information.  (The domain
        // global groups will not include membership info in the GC.)
        dwError = LsaDmLdapOpenGc(
                      pContext,
                      pszFullDomainName,
                      &pConn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_GetAttributeValuesList(
                        pConn,
                        pObject->pszDN,
                        AD_LDAP_MEMBEROF_TAG,
                        TRUE,
                        TRUE,
                        &gcMembershipCount,
                        &ppGcMembershipList);
        BAIL_ON_LSA_ERROR(dwError);

        LsaDmLdapClose(pConn);
        pConn = NULL;
    }

    dwError = LsaDmLdapOpenDc(
                  pContext,
                  pszFullDomainName,
                  &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_GetAttributeValuesList(
                    pConn,
                    pObject->pszDN,
                    AD_LDAP_MEMBEROF_TAG,
                    TRUE,
                    TRUE,
                    &dcMembershipCount,
                    &ppDcMembershipList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    (dcMembershipCount + gcMembershipCount + 1) * 2,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pGroupHash);
    BAIL_ON_LSA_ERROR(dwError);

    for (index = 0; index < gcMembershipCount; index++)
    {
        PSTR pSid = ppGcMembershipList[index];
        if (!LwHashExists(pGroupHash, pSid))
        {
            dwError = LwHashSetValue(pGroupHash, pSid, pSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    for (index = 0; index < dcMembershipCount; index++)
    {
        PSTR pSid = ppDcMembershipList[index];
        if (!LwHashExists(pGroupHash, pSid))
        {
            dwError = LwHashSetValue(pGroupHash, pSid, pSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pObject->type == LSA_OBJECT_TYPE_USER && pObject->userInfo.pszPrimaryGroupSid)
    {
        // Add the pszPrimaryGroupSID entry to the hash
        PSTR pSid = pObject->userInfo.pszPrimaryGroupSid;
        if (!LwHashExists(pGroupHash, pSid))
        {
            dwError = LwHashSetValue(pGroupHash, pSid, pSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = AD_MoveHashValuesToArray(
                    pGroupHash,
                    &totalSidCount,
                    (PVOID**)(PVOID)&ppTotalSidList);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = AD_FindObjectsBySidList(
                    pContext,
                    totalSidCount,
                    ppTotalSidList,
                    &sNumGroupsFound,
                    &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    AD_FilterNullEntries(
            ppGroupInfoList,
            &sNumGroupsFound);

    // Determine primary group index
    if (pObject->type == LSA_OBJECT_TYPE_USER &&
        pObject->userInfo.pszPrimaryGroupSid &&
        ppGroupInfoList &&
        sNumGroupsFound)
    {
        for (i = (INT)sNumGroupsFound - 1; i >= 0; i--)
        {
            if (!strcmp(ppGroupInfoList[i]->pszObjectSid, pObject->userInfo.pszPrimaryGroupSid))
            {
                iPrimaryGroupIndex = i;
                break;
            }
        }
    }

    *psNumGroupsFound = sNumGroupsFound;
    *pppGroupInfoList = ppGroupInfoList;
    *piPrimaryGroupIndex = iPrimaryGroupIndex;

cleanup:
    LwHashSafeFree(&pGroupHash);
    LW_SAFE_FREE_STRING(pszFullDomainName);
    LwFreeStringArray(ppGcMembershipList, gcMembershipCount);
    LwFreeStringArray(ppDcMembershipList, dcMembershipCount);
    // Do not free the string pointers inside. They are borrowed from the
    // hashes.
    LW_SAFE_FREE_MEMORY(ppTotalSidList);

    LsaDmLdapClose(pConn);

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *psNumGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    if ( dwError != LW_ERROR_DOMAIN_IS_OFFLINE )
    {
        LSA_LOG_ERROR("Failed to group memberships of SID=%s. [error code:%u]",
                      pObject->pszObjectSid, dwError);
    }

    ADCacheSafeFreeObjectList((DWORD)sNumGroupsFound, &ppGroupInfoList);

    goto cleanup;
}

DWORD
ADLdap_IsValidDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR    pszDN,
    PBOOLEAN pbValidDN
    )
{
    DWORD dwError = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_DN_TAG,
        NULL
    };
    LDAPMessage* pMessage = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_ONELEVEL,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    *pbValidDN = TRUE;

cleanup:

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT)
    {
        dwError = 0;
    }

    *pbValidDN = FALSE;

    goto cleanup;
}

DWORD
ADLdap_GetObjectSid(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PSTR pszSid = NULL;

    BAIL_ON_INVALID_POINTER(pMessage);

    if (hDirectory == (HANDLE)NULL)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pucSIDBytes,
                &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_POINTER(pucSIDBytes);

    dwError = LsaSidBytesToString(
                pucSIDBytes,
                dwSIDByteLength,
                &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:
    LW_SAFE_FREE_MEMORY(pucSIDBytes);

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszSid);
    *ppszSid = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetAccountType(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    OUT LSA_OBJECT_TYPE* pAccountType
    )
{
    DWORD dwError = 0;
    LSA_OBJECT_TYPE accountType = LSA_OBJECT_TYPE_UNDEFINED;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;

    // Determine whether this object is user or group.

    dwError = LwLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_OBJECTCLASS_TAG,
                    &ppszValues,
                    &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "user", sizeof("user")-1))
        {
            accountType = LSA_OBJECT_TYPE_USER;
            break;
        }
        else if (!strncasecmp(ppszValues[iValue], "group", sizeof("group")-1))
        {
            accountType = LSA_OBJECT_TYPE_GROUP;
            break;
        }
    }

cleanup:
    LwFreeStringArray(ppszValues, dwNumValues);

    *pAccountType = accountType;

    return dwError;

error:
    goto cleanup;
}
