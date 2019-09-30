/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "../includes.h"




VOID
FreeUserAttributes(
    PGPUSER_AD_ATTRS pUserADAttrs
    )
{
    if (pUserADAttrs)
    {
        if (pUserADAttrs->pszDisplayName)
            LwFreeString(pUserADAttrs->pszDisplayName);

        if (pUserADAttrs->pszFirstName)
            LwFreeString(pUserADAttrs->pszFirstName);

        if (pUserADAttrs->pszLastName)
            LwFreeString(pUserADAttrs->pszLastName);

        if (pUserADAttrs->pszADDomain)
            LwFreeString(pUserADAttrs->pszADDomain);

        if (pUserADAttrs->pszKerberosPrincipal)
            LwFreeString(pUserADAttrs->pszKerberosPrincipal);

        if (pUserADAttrs->pszEMailAddress)
            LwFreeString(pUserADAttrs->pszEMailAddress);

        if (pUserADAttrs->pszMSExchHomeServerName)
            LwFreeString(pUserADAttrs->pszMSExchHomeServerName);

        if (pUserADAttrs->pszMSExchHomeMDB)
            LwFreeString(pUserADAttrs->pszMSExchHomeMDB);

        if (pUserADAttrs->pszTelephoneNumber)
            LwFreeString(pUserADAttrs->pszTelephoneNumber);

        if (pUserADAttrs->pszFaxTelephoneNumber)
            LwFreeString(pUserADAttrs->pszFaxTelephoneNumber);

        if (pUserADAttrs->pszMobileTelephoneNumber)
            LwFreeString(pUserADAttrs->pszMobileTelephoneNumber);

        if (pUserADAttrs->pszStreetAddress)
            LwFreeString(pUserADAttrs->pszStreetAddress);

        if (pUserADAttrs->pszPostOfficeBox)
            LwFreeString(pUserADAttrs->pszPostOfficeBox);

        if (pUserADAttrs->pszCity)
            LwFreeString(pUserADAttrs->pszCity);

        if (pUserADAttrs->pszState)
            LwFreeString(pUserADAttrs->pszState);

        if (pUserADAttrs->pszPostalCode)
            LwFreeString(pUserADAttrs->pszPostalCode);

        if (pUserADAttrs->pszCountry)
            LwFreeString(pUserADAttrs->pszCountry);

        if (pUserADAttrs->pszTitle)
            LwFreeString(pUserADAttrs->pszTitle);

        if (pUserADAttrs->pszCompany)
            LwFreeString(pUserADAttrs->pszCompany);

        if (pUserADAttrs->pszDepartment)
            LwFreeString(pUserADAttrs->pszDepartment);

        if (pUserADAttrs->pszHomeDirectory)
            LwFreeString(pUserADAttrs->pszHomeDirectory);

        if (pUserADAttrs->pszHomeDrive)
            LwFreeString(pUserADAttrs->pszHomeDrive);

        if (pUserADAttrs->pszPasswordLastSet)
            LwFreeString(pUserADAttrs->pszPasswordLastSet);

        if (pUserADAttrs->pszUserAccountControl)
            LwFreeString(pUserADAttrs->pszUserAccountControl);

        if (pUserADAttrs->pszMaxMinutesUntilChangePassword)
            LwFreeString(pUserADAttrs->pszMaxMinutesUntilChangePassword);

        if (pUserADAttrs->pszMinMinutesUntilChangePassword)
            LwFreeString(pUserADAttrs->pszMinMinutesUntilChangePassword);

        if (pUserADAttrs->pszMaxFailedLoginAttempts)
            LwFreeString(pUserADAttrs->pszMaxFailedLoginAttempts);

        if (pUserADAttrs->pszAllowedPasswordHistory)
            LwFreeString(pUserADAttrs->pszAllowedPasswordHistory);

        if (pUserADAttrs->pszMinCharsAllowedInPassword)
            LwFreeString(pUserADAttrs->pszMinCharsAllowedInPassword);

        LwFreeMemory(pUserADAttrs);
    }
}

DWORD
CacheUserLoginMessage(
    PSTR pszHomeDirPath,
    PSTR pszMessage
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszFileDir = NULL;
    PSTR pszFilePath = NULL;
    BOOLEAN bDirExists = FALSE;
    FILE * fp = NULL;
    int len = 0;

    if (!pszMessage || strlen(pszMessage) == 0)
    {
        goto error;
    }

    if (!pszHomeDirPath || strlen(pszHomeDirPath) == 0)
    {
        goto error;
    }

    LOG("Saving user login message to [homedir: %s]", pszHomeDirPath);

    dwError = LwAllocateStringPrintf(&pszFileDir, "%s/Library/Preferences", pszHomeDirPath);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszFilePath, "%s/login-message", pszFileDir);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCheckFileTypeExists(pszFileDir, LWFILE_DIRECTORY, &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (bDirExists == FALSE)
    {
        dwError = LwCreateDirectory(pszFileDir, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_MAC_ERROR(dwError);
    }

    fp = fopen(pszFilePath, "w");
    if (fp)
    {
        len = fwrite(pszMessage, sizeof(char), strlen(pszMessage), fp);
        if (len < strlen(pszMessage))
        {
            dwError = ENOMEM;
            BAIL_ON_MAC_ERROR(dwError);
        }
    }

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    LW_SAFE_FREE_STRING(pszFileDir);

    if (fp)
    {
        fclose(fp);
    }

    return dwError;
}

DWORD
CollectCurrentADAttributesForUser(
    PSTR pszDomainUserName,    
    PSTR pszMessage,
    BOOLEAN bOnlineLogon
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT* ppUserObjects = NULL;
    
    if (bOnlineLogon)
    {
        LOG("Connecting to lsass");
                
        dwError = GetUserObjectFromName(pszDomainUserName, &ppUserObjects);
        
        if (dwError)
        {
            LOG("Error (%d) while reading user attributes from lsass", dwError);
            BAIL_ON_MAC_ERROR(dwError);
        }

        dwError = CacheUserAttributes(ppUserObjects[0]->userInfo.uid, *ppUserObjects);
        if (dwError)
        {
            LOG("Error (%d) while saving user  attributes to cache", dwError);
            BAIL_ON_MAC_ERROR(dwError);
        }
   
        dwError = FlushDirectoryServiceCache();
        if (dwError)
        {
            LOG("Failed to flush the Mac DirectoryService cache. Error: %d", dwError);
            BAIL_ON_MAC_ERROR(dwError);
        }
    }
    else
    {
        LOG("Offline logon, can't refresh user attributes for: user: %s", pszDomainUserName);
    }
    
    
cleanup:

    if (ppUserObjects) {
       FreeObjectList(1, ppUserObjects);
    }    

    return LWGetMacError(dwError);

error:

    goto cleanup;

}


DWORD
CacheUserAttributes(
    uid_t uid,
    PLSA_SECURITY_OBJECT pUserADAttrs
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    PSTR pszFileDir = NULL;
    PSTR pszFilePath = NULL;
    PCFGSECTION pUserSettingsList = NULL;
    PCFGSECTION pADSection_Name = NULL;
    PCFGSECTION pADSection_EMail = NULL;
    PCFGSECTION pADSection_Phone = NULL;
    PCFGSECTION pADSection_Address = NULL;
    PCFGSECTION pADSection_Work = NULL;
    PCFGSECTION pADSection_Network = NULL;
    BOOLEAN bDirExists = FALSE;

    dwError = LwAllocateStringPrintf(&pszFileDir, "/var/lib/pbis/lwedsplugin/user-cache/%ld", (long) uid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszFilePath, "/var/lib/pbis/lwedsplugin/user-cache/%ld/ad-user-attrs", (long) uid);
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LwCheckFileTypeExists(pszFileDir, LWFILE_DIRECTORY, &bDirExists);
    BAIL_ON_MAC_ERROR(dwError);

    if (bDirExists == FALSE)
    {
        dwError = LwCreateDirectory(pszFileDir, S_IRUSR|S_IRGRP|S_IROTH);
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWCreateConfigSection(&pUserSettingsList,
                                    &pADSection_Name,
                                    "User AD Name Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_EMail,
                                    &pADSection_EMail,
                                    "User AD EMail Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Phone,
                                    &pADSection_Phone,
                                    "User AD Phone Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Address,
                                    &pADSection_Address,
                                    "User AD Address Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Work,
                                    &pADSection_Work,
                                    "User AD Work Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    dwError = LWCreateConfigSection(&pADSection_Network,
                                    &pADSection_Network,
                                    "User AD Network Settings Attributes");
    BAIL_ON_MAC_ERROR(dwError);

    pADSection_Name->pNext = pADSection_EMail;
    pADSection_EMail->pNext = pADSection_Phone;
    pADSection_Phone->pNext = pADSection_Address;
    pADSection_Address->pNext = pADSection_Work;
    pADSection_Work->pNext = pADSection_Network;
       
    if (pUserADAttrs->userInfo.pszDisplayName)
    {
        dwError = LWSetConfigValueBySection(pUserSettingsList,
                                            "displayName",
                                            pUserADAttrs->userInfo.pszDisplayName);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->userInfo.pszUPN)
    {
        dwError = LWSetConfigValueBySection(pADSection_Name,
                                            "userPrincipalName",
                                            pUserADAttrs->userInfo.pszUPN);
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (pUserADAttrs->userInfo.pszWindowsHomeFolder)
    {
        dwError = LWSetConfigValueBySection(pADSection_Network,
                                            "homeDirectory",
                                            pUserADAttrs->userInfo.pszWindowsHomeFolder);
        BAIL_ON_MAC_ERROR(dwError);
    }
    dwError = LWSaveConfigSectionList(pszFilePath, pUserSettingsList);
    BAIL_ON_MAC_ERROR(dwError);

error:

    LW_SAFE_FREE_STRING(pszFilePath);
    LW_SAFE_FREE_STRING(pszFileDir);

    LWFreeConfigSectionList(pUserSettingsList);
    pUserSettingsList = NULL;

    return dwError;
}


DWORD
FlushDirectoryServiceCache(
    )
{
    DWORD dwError = MAC_AD_ERROR_SUCCESS;
    BOOLEAN exists = FALSE;

    dwError = LwCheckFileTypeExists("/usr/sbin/lookupd", LWFILE_REGULAR, &exists);
    BAIL_ON_MAC_ERROR(dwError);

    if (!exists)
    {
        system("/usr/sbin/lookupd -flushcache");
    }

    dwError = LwCheckFileTypeExists("/usr/bin/dscacheutil", LWFILE_REGULAR, &exists);
    BAIL_ON_MAC_ERROR(dwError);

    if (exists)
    {
        system("/usr/bin/dscacheutil -flushcache");
    }

error:

    return dwError;
}


