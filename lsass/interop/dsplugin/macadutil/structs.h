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

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#ifndef GPO_DEFINED
#define GPO_DEFINED 1
typedef struct _GROUP_POLICY_OBJECT {
    DWORD dwOptions;    // GPLink options - do we care?
    DWORD dwVersion;    // Version - extract this from the gpt.ini
    PSTR pszPolicyDN;
    PSTR pszDSPath;
    PSTR pszDisplayName;
    PSTR pszgPCFileSysPath;
    PSTR pszgPCMachineExtensionNames;
    PSTR pszgPCUserExtensionNames;
    DWORD gPCFunctionalityVersion;
    DWORD dwFlags;
    BOOLEAN bNewVersion;
    struct _GROUP_POLICY_OBJECT *pNext;
} GROUP_POLICY_OBJECT, *PGROUP_POLICY_OBJECT;
#endif

typedef struct _ADU_DIRECTORY_CONTEXT
{
    LDAP *ld;

} ADU_DIRECTORY_CONTEXT, *PADU_DIRECTORY_CONTEXT;

typedef struct _ADU_CRED_CONTEXT
{
    PSTR                pszPrincipalName;
    PSTR                pszCachePath;
    PSTR                pszHomeDirPath;
    PSTR                pszSID;
    uid_t               uid;
    BOOLEAN             bDestroyCachePath;

    LW_PIO_CREDS pAccessToken;

} ADU_CRED_CONTEXT, *PADU_CRED_CONTEXT;

#ifndef DSATTRIBUTEVALUE_DEFINED
#define DSATTRIBUTEVALUE_DEFINED 1

typedef struct __DSATTRIBUTEVALUE
{
    uint32_t valLen;
    char *   pszValue;
    struct __DSATTRIBUTEVALUE * pNext;
} DSATTRIBUTEVALUE, *PDSATTRIBUTEVALUE;

#endif /* DSATTRIBUTEVALUE_DEFINED */

#ifndef MCXVALUE_DEFINED
#define MCXVALUE_DEFINED 1

typedef struct __MCXVALUE
{
    char *   pValueData;
    int      iValLen;
    struct __MCXVALUE * pNext;
} MCXVALUE, *PMCXVALUE;

#endif /* MCXVALUE_DEFINED */

#ifndef AD_USER_ATTRIBUTES_DEFINED
#define AD_USER_ATTRIBUTES_DEFINED 1

typedef struct __AD_USER_ATTRIBUTES
{
    /* Name attributes */
    PSTR  pszDisplayName;
    PSTR  pszFirstName;
    PSTR  pszLastName;
    PSTR  pszADDomain;
    PSTR  pszKerberosPrincipal;

    /* Email attributes */
    PSTR  pszEMailAddress;
    PSTR  pszMSExchHomeServerName;
    PSTR  pszMSExchHomeMDB;

    /* Phone attributes */
    PSTR  pszTelephoneNumber;
    PSTR  pszFaxTelephoneNumber;
    PSTR  pszMobileTelephoneNumber;

    /* Address attributes */
    PSTR  pszStreetAddress;
    PSTR  pszPostOfficeBox;
    PSTR  pszCity;
    PSTR  pszState;
    PSTR  pszPostalCode;
    PSTR  pszCountry;

    /* Work attributes */
    PSTR  pszTitle;
    PSTR  pszCompany;
    PSTR  pszDepartment;

    /* Network setting attributes */
    PSTR  pszHomeDirectory;
    PSTR  pszHomeDrive;
    PSTR  pszPasswordLastSet;
    PSTR  pszUserAccountControl;
    PSTR  pszMaxMinutesUntilChangePassword;
    PSTR  pszMinMinutesUntilChangePassword;
    PSTR  pszMaxFailedLoginAttempts;
    PSTR  pszAllowedPasswordHistory;
    PSTR  pszMinCharsAllowedInPassword;

} AD_USER_ATTRIBUTES, *PAD_USER_ATTRIBUTES;

#endif /* AD_USER_ATTRIBUTES_DEFINED */


#endif /* __STRUCTS_H__ */
