/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        join.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

#define LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS 60

#define MACHPASS_LEN  (16)


static
DWORD
LsaJoinDomainInternal(
    PWSTR  pwszHostname,
    PWSTR  pwszDnsDomain,
    PWSTR  pwszDomain,
    PWSTR  pwszAccountOu,
    PWSTR  pwszAccount,
    PWSTR  pwszPassword,
    DWORD  dwJoinFlags,
    DWORD  dwUserAccountAttributes,
    PWSTR  pwszOsName,
    PWSTR  pwszOsVersion,
    PWSTR  pwszOsServicePack,
    PSTR   pszServicePrincipalNameList
    );

static
DWORD
LsaRandBytes(
    PBYTE pBuffer,
    DWORD dwCount
    );

static
DWORD
LsaGenerateMachinePassword(
    PWSTR  pwszPassword,
    size_t sPasswordLen
    );


DWORD
LsaGenerateRandomString(
    PSTR    pszBuffer,
    size_t  sBufferLen
    );

static
DWORD
LsaGetAccountName(
    const wchar16_t *machname,
    const wchar16_t *domain_controller_name,
    const wchar16_t *dns_domain_name,
    wchar16_t       **account_name,
    BOOLEAN         *exists
    );


static
NTSTATUS
LsaCreateMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    PWSTR          pwszMachinePassword,
    DWORD          dwJoinFlags,
    PWSTR         *ppwszDomainName,
    PSID          *ppDomainSid
    );


static
NTSTATUS
LsaEncryptPasswordBufferEx(
    PBYTE  pPasswordBuffer,
    DWORD  dwPasswordBufferSize,
    PWSTR  pwszPassword,
    DWORD  dwPasswordLen,
    PBYTE  pSessionKey,
    DWORD  dwSessionKeyLen
    );


static
NTSTATUS
LsaEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    );


static
DWORD
LsaSaveMachinePassword(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachineAccountName,
    PCWSTR  pwszMachineDnsDomain,
    PCWSTR  pwszDomainName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszDCName,
    PCWSTR  pwszSidStr,
    PCWSTR  pwszPassword,
    PSTR    pszServicePrincipalNameList
    );

static
DWORD
LsaBuildPrincipalName(
    OUT PWSTR *ppPrincipalName,
    IN PCWSTR InstanceName,
    IN PCWSTR HostName,
    IN BOOLEAN UpperCaseHostName,
    IN OPTIONAL PCWSTR RealmName,
    IN OPTIONAL BOOLEAN UpperCaseRealmName
    );

static
DWORD
LsaSavePrincipalKey(
    PCWSTR  pwszName,
    PCWSTR  pwszPassword,
    DWORD   dwPasswordLen,
    PCWSTR  pwszRealm,
    PCWSTR  pwszSalt,
    PCWSTR  pwszDCName,
    DWORD   dwKvno
    );


static
DWORD
LsaDirectoryConnect(
    PCWSTR pDomain,
    LDAP** ppLdConn,
    PWSTR* ppDefaultContext
    );


static
DWORD
LsaDirectoryDisconnect(
    LDAP *ldconn
    );


static
DWORD
LsaMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    BOOLEAN move,
    BOOLEAN exists
    );


static
DWORD
LsaMachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *fqdn,
    const wchar16_t *machname,
    const wchar16_t *dn_context,
    wchar16_t **samacct
    );


static
DWORD
LsaMachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **pDn,
    wchar16_t **pDnsName
    );


static
DWORD
LsaMachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    );

static
DWORD
LsaGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pNtHash,
    IN  DWORD   dwNtHashSize
    );


static
DWORD
LsaEncryptNtHashVerifier(
    IN  PBYTE    pNewNtHash,
    IN  DWORD    dwNewNtHashLen,
    IN  PBYTE    pOldNtHash,
    IN  DWORD    dwOldNtHashLen,
    OUT PBYTE    pNtVerifier,
    IN  DWORD    dwNtVerifierSize
    );


static
DWORD
LsaPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    );

DWORD
LsaJoinDomainUac(
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    LSA_NET_JOIN_FLAGS dwFlags,
    LSA_USER_ACCOUNT_CONTROL_FLAGS dwUac,
    PSTR pszServicePrincipalNameList
    )
{
    DWORD dwError = 0;
    PWSTR pwszHostname = NULL;
    PWSTR pwszHostDnsDomain = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszOU = NULL;
    PWSTR pwszOSName = NULL;
    PWSTR pwszOSVersion = NULL;
    PWSTR pwszOSServicePack = NULL;
    DWORD dwOptions = (LSAJOIN_JOIN_DOMAIN |
                       LSAJOIN_ACCT_CREATE |
                       LSAJOIN_DOMAIN_JOIN_IF_JOINED);
    PLSA_CREDS_FREE_INFO pAccessInfo = NULL;

    BAIL_ON_INVALID_STRING(pszHostname);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    if (geteuid() != 0) {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_LOG_DEBUG("LsaJoinDomainUac(%s, %s, %s, %s, %s, ********, %s, %s, %s, %x, %x)",
            LSA_SAFE_LOG_STRING(pszHostname),
            LSA_SAFE_LOG_STRING(pszHostDnsDomain),
            LSA_SAFE_LOG_STRING(pszDomain),
            LSA_SAFE_LOG_STRING(pszOU),
            LSA_SAFE_LOG_STRING(pszUsername),
            LSA_SAFE_LOG_STRING(pszOSName),
            LSA_SAFE_LOG_STRING(pszOSVersion),
            LSA_SAFE_LOG_STRING(pszOSServicePack),
            dwFlags, dwUac);

    if ( !(dwFlags & LSA_NET_JOIN_DOMAIN_NOTIMESYNC) )
    {
        dwError = LsaSyncTimeToDC(pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwFlags & LSA_NET_JOIN_DOMAIN_NO_PRE_COMPUTER_ACCOUNT)
    {
        dwOptions |= LSAJOIN_NO_PRE_COMPUTER_ACCOUNT;
    }

    // TODO-2011/01/13-dalmeida -- Ensure we use UPN.
    // Otherwise, whether this works depends on krb5.conf.
    // Can cons up UPN if needed since we have domain.
    dwError = LsaSetSMBCreds(
                pszUsername,
                pszPassword,
                TRUE,
                &pAccessInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszHostDnsDomain))
    {
        dwError = LwMbsToWc16s(
                        pszHostDnsDomain,
                        &pwszHostDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(
                    pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOU))
    {
        dwError = LwMbsToWc16s(
                    pszOU,
                    &pwszOU);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSName)) {
        dwError = LwMbsToWc16s(
                    pszOSName,
                    &pwszOSName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSVersion)) {
        dwError = LwMbsToWc16s(
                    pszOSVersion,
                    &pwszOSVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSServicePack)) {
        dwError = LwMbsToWc16s(
                    pszOSServicePack,
                    &pwszOSServicePack);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaJoinDomainInternal(
            pwszHostname,
            pwszHostDnsDomain,
            pwszDomain,
            pwszOU,
            NULL,
            NULL,
            dwOptions,
            dwUac,
            pwszOSName,
            pwszOSVersion,
            pwszOSServicePack,
            pszServicePrincipalNameList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPstoreSetDomainWTrustEnumerationWaitTime(pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaFreeSMBCreds(&pAccessInfo);

    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszHostDnsDomain);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszOU);
    LW_SAFE_FREE_MEMORY(pwszOSName);
    LW_SAFE_FREE_MEMORY(pwszOSVersion);
    LW_SAFE_FREE_MEMORY(pwszOSServicePack);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDCTime = 0;

    dwError = LWNetGetDCTime(
                    pszDomain,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);

    ttDCTime = (time_t) dcTime;

    if (labs(ttDCTime - time(NULL)) > LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS) {
        dwError = LwSetSystemTime(ttDCTime);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


// Distinguish between nfs, nfs/ and http/www.linuxcomputer.com. If it is
// service class name with trailing slash, then strip the slash.
static
VOID
GroomSpn(PSTR *ppszSpn, BOOLEAN *bIsServiceClass)
{
   PSTR pszHasPeriod = NULL;
   PSTR pszHasSlash = NULL;

   LwStripLeadingWhitespace(*ppszSpn);
   LwStripTrailingWhitespace(*ppszSpn);

   LwStrChr(*ppszSpn, '.', &pszHasPeriod);
   if (pszHasPeriod)
   {
     *bIsServiceClass = FALSE;
     return;
   }

   LwStrChr(*ppszSpn, '/', &pszHasSlash);
   if ((pszHasSlash) && (strlen(pszHasSlash) == 1))
   {
      *pszHasSlash = '\0';
      *bIsServiceClass = TRUE;
   }
   else if ((pszHasSlash) && (strlen(pszHasSlash) >= 1))
     *bIsServiceClass = FALSE;
   else
     *bIsServiceClass = TRUE;

   return;
}


// Input:  nfs, NFS, http/linuxhostbox, host, HOST/, http/linuxhostbox2
// Output: NFS, HOST, http/linuxhostbox, http/linuxhostbox2
static
VOID GroomSpnList(PSTR pszSPNameList, PSTR *ppGroomedServicePrincipalList)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   BOOLEAN bIsServiceClass = FALSE;
   PSTR pszServicePrincipalNameList = NULL;
   PSTR saveStrPtr = NULL;
   PSTR aStr = NULL;
   PSTR isStrThere = NULL;
   PSTR pszTempStr = NULL;
   PSTR pszNewServicePrincipalNameList = NULL;
   PSTR pszFqdnList = NULL;
   PSTR pszServiceClassList = NULL;

   dwError = LwStrDupOrNull(pszSPNameList, &pszServicePrincipalNameList);
   BAIL_ON_LSA_ERROR(dwError);

   // Tokenize the provided SPN list checking for duplicates SPN values and
   // upper casing SPN.
   aStr = strtok_r(pszServicePrincipalNameList, ",", &saveStrPtr);
   while (aStr != NULL)
   {
      GroomSpn(&aStr, &bIsServiceClass);

      if (bIsServiceClass)
      {
         LwStrToUpper(aStr);
         LwStrStr(pszServiceClassList, aStr, &isStrThere);
         if (!isStrThere)
         {
            if (pszServiceClassList)
               LwAllocateStringPrintf(&pszTempStr, "%s,%s", pszServiceClassList, aStr);
            else
               LwAllocateStringPrintf(&pszTempStr, "%s", aStr);
            LW_SAFE_FREE_STRING(pszServiceClassList);
            pszServiceClassList = pszTempStr;
            pszTempStr = NULL;
         }
      }
      else
      {
         LwStrStr(pszFqdnList, aStr, &isStrThere);
         if ((!isStrThere) || (strlen(aStr) != (strlen(isStrThere))))
         {
            if (pszFqdnList)
               LwAllocateStringPrintf(&pszTempStr, "%s,%s", pszFqdnList, aStr);
            else
               LwAllocateStringPrintf(&pszTempStr, "%s", aStr);

            LW_SAFE_FREE_STRING(pszFqdnList);
            pszFqdnList = pszTempStr;
            pszTempStr = NULL;
         }
      }
      aStr = strtok_r(NULL, ",", &saveStrPtr);
   }

   if (pszFqdnList && pszServiceClassList)
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s,%s", pszServiceClassList, pszFqdnList);
   else if (pszFqdnList && !pszServiceClassList)
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s", pszFqdnList);
   else
     LwAllocateStringPrintf(&pszNewServicePrincipalNameList, "%s", pszServiceClassList);

   // pszNewServicePrincipalNameList contains non-duplicated, uppercase service class.
   // Fully qualified SPN is added as is.
   *ppGroomedServicePrincipalList = pszNewServicePrincipalNameList;

cleanup:

   LW_SAFE_FREE_STRING(pszFqdnList);
   LW_SAFE_FREE_STRING(pszServiceClassList);

   return;

error:
   goto cleanup;
}

static
VOID
LsaFreeSpnAttrVal(
    PWSTR *ppwszSpnAttrVal,
    DWORD dwNumberOfSpnEntries)
{

  DWORD i = 0;

  for (i = 0; i < dwNumberOfSpnEntries; i++)
  {
     LW_SAFE_FREE_MEMORY(ppwszSpnAttrVal[i]);
  }

  LwFreeMemory(ppwszSpnAttrVal);
}

static
DWORD
LsaCreateSpnAttrVal(
    PWSTR pwszDnsHostName,
    PWSTR pwszHostname,
    PSTR  pszServicePrincipalNameList,
    PWSTR **pppwszSpnAttrVal,
    DWORD *pdwNumberOfSpnEntries)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOLEAN bIsServiceClass = FALSE;
    DWORD dwNumberOfSpnEntries = 150;
    DWORD i = 0;
    PSTR aStr = NULL;
    PSTR saveStrPtr = NULL;
    PSTR pszGroomedSPNList = NULL;
    PWSTR *ppwszSPNAttrVal = NULL;

    GroomSpnList(pszServicePrincipalNameList, &pszGroomedSPNList);

    dwError = LwAllocateMemory(dwNumberOfSpnEntries * sizeof(PWSTR), (PVOID*) &ppwszSPNAttrVal);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNumberOfSpnEntries; i++)
        ppwszSPNAttrVal[i] = NULL;

    i = 0;
    aStr = strtok_r(pszGroomedSPNList, ",", &saveStrPtr);
    while ((aStr != NULL) && (i < (dwNumberOfSpnEntries-1)))
    {
       GroomSpn(&aStr, &bIsServiceClass);
       if (bIsServiceClass)
       {
          ppwszSPNAttrVal[i] = LdapAttrValSvcPrincipalName(aStr, pwszDnsHostName);
          ppwszSPNAttrVal[i+1] = LdapAttrValSvcPrincipalName(aStr, pwszHostname);
          i+=2;
       }
       else
       {
          LwMbsToWc16s(aStr, &ppwszSPNAttrVal[i]);
          i++;
       }
       aStr = strtok_r(NULL, ",", &saveStrPtr);
    }

    *pdwNumberOfSpnEntries = dwNumberOfSpnEntries;
    *pppwszSpnAttrVal = ppwszSPNAttrVal;

cleanup:

    LW_SAFE_FREE_STRING(pszGroomedSPNList);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
LsaJoinDomainInternal(
    PWSTR  pwszHostname,
    PWSTR  pwszDnsDomain,
    PWSTR  pwszDomain,
    PWSTR  pwszAccountOu,
    PWSTR  pwszAccount,
    PWSTR  pwszPassword,
    DWORD  dwJoinFlags,
    DWORD  dwUserAccountAttributes,
    PWSTR  pwszOsName,
    PWSTR  pwszOsVersion,
    PWSTR  pwszOsServicePack,
    PSTR   pszServicePrincipalNameList
    )
{
    const DWORD dwLsaAccess = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                              LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    LSA_BINDING hLsaBinding = NULL;
    POLICY_HANDLE hLsaPolicy = NULL;
    LsaPolicyInformation *pLsaPolicyInfo = NULL;
    PWSTR pwszMachineName = NULL;
    PWSTR pwszMachineAcctName = NULL;
    WCHAR pwszMachinePassword[MACHPASS_LEN+1] = {0};
    PWSTR pwszDomainName = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszDnsDomainName = NULL;
    PWSTR pwszDCName = NULL;
    LDAP *pLdap = NULL;
    PWSTR pwszMachineNameLc = NULL;    /* lower cased machine name */
    PWSTR pwszBaseDn = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszDnsAttrName = NULL;
    PWSTR pwszDnsAttrVal[2] = {0};
    PWSTR pwszSpnAttrName = NULL;
    PWSTR *ppwszSpnAttrVal = NULL;
    PWSTR pwszOSNameAttrName = NULL;
    PWSTR pwszOSNameAttrVal[2] = {0};
    PWSTR pwszOSVersionAttrName = NULL;
    PWSTR pwszOSVersionAttrVal[2] = {0};
    PWSTR pwszOSServicePackAttrName = NULL;
    PWSTR pwszOSServicePackAttrVal[2] = {0};
    PWSTR pwszUserAccountControlName = NULL;
    PWSTR pwszUserAccountControlVal[2] = {0};
    PWSTR pwszSupportedEncryptionTypesName = NULL;
    PWSTR pwszSupportedEncryptionTypesVal[2] = {0};
    DWORD dwSupportedEncryptionTypes = 0;
    PWSTR pwszSidStr = NULL;
    PWSTR pwszComputerContainer = NULL;
    WCHAR wszUacVal[11] = {0};
    LW_PIO_CREDS pCreds = NULL;
    BOOLEAN account_exists = FALSE;
    DWORD dwNumberOfSpnEntries = 0;

    dwError = LwAllocateWc16String(&pwszMachineName,
                                   pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToUpper(pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError)

    dwError = LsaGetRwDcName(pwszDomain,
                             TRUE,
                             &pwszDCName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszAccount && pwszPassword)
    {
        ntStatus = LwIoCreatePlainCredsW(pwszAccount,
                                         pwszDomain,
                                         pwszPassword,
                                         &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = LwIoGetActiveCreds(NULL,
                                      &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                     pwszDCName,
                                     pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszDCName,
                              NULL,
                              dwLsaAccess,
                              &hLsaPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy2(hLsaBinding,
                                   hLsaPolicy,
                                   LSA_POLICY_INFO_DNS,
                                   &pLsaPolicyInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWc16StringFromUnicodeString(
                                  &pwszDnsDomainName,
                                  &pLsaPolicyInfo->dns.dns_domain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetAccountName(
                             pwszMachineName,
                             pwszDCName,
                             pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName,
                             &pwszMachineAcctName,
                             &account_exists);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDirectoryConnect(
                    pwszDCName,
                    &pLdap,
                    &pwszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszAccountOu == NULL)
    {
        if (!(dwJoinFlags & LSAJOIN_NO_PRE_COMPUTER_ACCOUNT))
        {
            pwszComputerContainer = pwszAccountOu = LdapGetWellKnownObject(pLdap, pwszBaseDn, LW_GUID_COMPUTERS_CONTAINER);
        }
    }

    /* Pre-create a disabled machine
       account object in given branch of directory. It will
       be reset afterwards by means of rpc calls */
    if (pwszAccountOu)
    {
        int move = (dwJoinFlags & LSAJOIN_DOMAIN_JOIN_IF_JOINED) == LSAJOIN_DOMAIN_JOIN_IF_JOINED ? TRUE : FALSE;

        // If a specific OU wasn't requested then we don't want to move the Computer account
        if (pwszComputerContainer) move = FALSE;

        dwError = LsaMachAcctCreate(
                      pLdap,
                      pwszMachineName,
                      pwszMachineAcctName,
                      pwszAccountOu,
                      move,
                      account_exists);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDirectoryDisconnect(pLdap);
    pLdap = NULL;
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_MEMORY(pwszBaseDn);

    dwError = LsaGenerateMachinePassword(
               (PWSTR)pwszMachinePassword,
               sizeof(pwszMachinePassword)/sizeof(pwszMachinePassword[0]));
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszMachinePassword[0] == '\0')
    {
        BAIL_ON_NT_STATUS(STATUS_INTERNAL_ERROR);
    }

    ntStatus = LsaCreateMachineAccount(pwszDCName,
                                       pCreds,
                                       pwszMachineAcctName,
                                       (PWSTR)pwszMachinePassword,
                                       dwJoinFlags,
                                       &pwszDomainName,
                                       &pDomainSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSidStr,
                                            pDomainSid);
    BAIL_ON_NT_STATUS(ntStatus);

    // Make sure we can access the account
    dwError = LsaDirectoryConnect(
                    pwszDCName,
                    &pLdap,
                    &pwszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMachAcctSearch(
                  pLdap,
                  pwszMachineAcctName,
                  pwszBaseDn,
                  &pwszDn,
                  NULL);
    if (dwError == ERROR_INVALID_PARAMETER)
    {
        dwError = LW_ERROR_LDAP_INSUFFICIENT_ACCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSaveMachinePassword(
              pwszMachineName,
              pwszMachineAcctName,
              pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName,
              pwszDomainName,
              pwszDnsDomainName,
              pwszDCName,
              pwszSidStr,
              (PWSTR)pwszMachinePassword,
              pszServicePrincipalNameList);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Open connection to directory server if it's going to be needed
     */
    if (!(dwJoinFlags & LSAJOIN_DEFER_SPN_SET) ||
        pwszOsName || pwszOsVersion || pwszOsServicePack)
    {

        /*
         * Set SPN and dnsHostName attributes unless this part is to be deferred
         */
        if (!(dwJoinFlags & LSAJOIN_DEFER_SPN_SET))
        {
            PWSTR pwszDnsHostName = NULL;

            dwError = LwAllocateWc16String(&pwszMachineNameLc,
                                           pwszMachineName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sToLower(pwszMachineNameLc);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwMbsToWc16s("dNSHostName", &pwszDnsAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            pwszDnsAttrVal[0] = LdapAttrValDnsHostName(pwszMachineNameLc,
                                  pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName);
            pwszDnsAttrVal[1] = NULL;
            pwszDnsHostName = pwszDnsAttrVal[0];

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszDnsAttrName,
                                       (const wchar16_t**)pwszDnsAttrVal, 0);
            if (dwError == ERROR_DS_CONSTRAINT_VIOLATION)
            {
                dwError = ERROR_DS_NAME_ERROR_NO_MAPPING;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwMbsToWc16s("servicePrincipalName",
                                       &pwszSpnAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaCreateSpnAttrVal(
                               pwszDnsHostName,
                               pwszHostname,
                               pszServicePrincipalNameList,
                               &ppwszSpnAttrVal,
                               &dwNumberOfSpnEntries);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn, pwszSpnAttrName,
                                                  (const wchar16_t**)ppwszSpnAttrVal, 0);

            if (dwError == LW_ERROR_LDAP_CONSTRAINT_VIOLATION)
            {
               dwError = LW_ERROR_LDAP_CONSTRAINT_VIOLATION_SPN;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }

        /*
         * Set operating system name and version attributes if specified
         */
        if (pwszOsName)
        {
            dwError = LwMbsToWc16s("operatingSystem",
                                   &pwszOSNameAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSNameAttrVal[0],
                                           pwszOsName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSNameAttrName,
                                       (const wchar16_t**)pwszOSNameAttrVal,
                                       0);
            if (dwError == LW_ERROR_LDAP_INSUFFICIENT_ACCESS)
            {
                /* The user must be a non-admin. In this case, we cannot
                 * set the attribute.
                 */
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if (pwszOsVersion)
        {
            dwError = LwMbsToWc16s("operatingSystemVersion",
                                   &pwszOSVersionAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSVersionAttrVal[0],
                                           pwszOsVersion);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSVersionAttrName,
                                       (const wchar16_t**)pwszOSVersionAttrVal,
                                       0);
            if (dwError == LW_ERROR_LDAP_INSUFFICIENT_ACCESS)
            {
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if (pwszOsServicePack)
        {
            dwError = LwMbsToWc16s("operatingSystemServicePack",
                                   &pwszOSServicePackAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSServicePackAttrVal[0],
                                           pwszOsServicePack);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSServicePackAttrName,
                                       (const wchar16_t**)pwszOSServicePackAttrVal,
                                       0);
            if (dwError == LW_ERROR_LDAP_INSUFFICIENT_ACCESS)
            {
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if (dwUserAccountAttributes == 0)
        {
            dwUserAccountAttributes = LSAJOIN_WORKSTATION_TRUST_ACCOUNT;
        }
        if (dwUserAccountAttributes)
        {
            dwError = LwMbsToWc16s("userAccountControl",
                                   &pwszUserAccountControlName);
            BAIL_ON_LSA_ERROR(dwError);

           sw16printfw(
                wszUacVal,
                sizeof(wszUacVal)/sizeof(wszUacVal[0]),
                L"%u",
                dwUserAccountAttributes);
            pwszUserAccountControlVal[0] = wszUacVal;
            dwError = LwAllocateWc16String(&pwszUserAccountControlVal[0],
                                           wszUacVal);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszUserAccountControlName,
                                       (const wchar16_t**)pwszUserAccountControlVal,
                                       0);
            if (dwError == LW_ERROR_LDAP_INSUFFICIENT_ACCESS)
            {
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

		LwKrb5GetSupportedEncryptionTypes(&dwSupportedEncryptionTypes);

        if (dwSupportedEncryptionTypes)
        {
            dwError = LwMbsToWc16s("msDS-SupportedEncryptionTypes",
                                   &pwszSupportedEncryptionTypesName);
            BAIL_ON_LSA_ERROR(dwError);

           sw16printfw(
                wszUacVal,
                sizeof(wszUacVal)/sizeof(wszUacVal[0]),
                L"%u",
                dwSupportedEncryptionTypes);

            dwError = LwAllocateWc16String(&pwszSupportedEncryptionTypesVal[0],
                                           wszUacVal);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszSupportedEncryptionTypesName,
                                       (const wchar16_t**)pwszSupportedEncryptionTypesVal,
                                       0);
            if (dwError == LW_ERROR_LDAP_INSUFFICIENT_ACCESS || dwError == LW_ERROR_LDAP_NO_SUCH_ATTRIBUTE)
            {
                // Ignore if we don't have access or are talking to an old domain that does not have this attribute
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

cleanup:
    if (hLsaBinding && hLsaPolicy)
    {
        LsaClose(hLsaBinding, hLsaPolicy);

        LsaFreeBinding(&hLsaBinding);
    }

    if (pLsaPolicyInfo)
    {
        LsaRpcFreeMemory(pLsaPolicyInfo);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (pLdap)
    {
        LsaDirectoryDisconnect(pLdap);
    }

    if (ppwszSpnAttrVal)
    {
        LsaFreeSpnAttrVal(ppwszSpnAttrVal, dwNumberOfSpnEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszDomainName);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pwszSidStr);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pwszMachineName);
    LW_SAFE_FREE_MEMORY(pwszMachineAcctName);
    LW_SAFE_FREE_MEMORY(pwszMachineNameLc);
    LW_SAFE_FREE_MEMORY(pwszBaseDn);
    LW_SAFE_FREE_MEMORY(pwszDn);
    LW_SAFE_FREE_MEMORY(pwszDnsAttrName);
    LW_SAFE_FREE_MEMORY(pwszDnsAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszSpnAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSNameAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSNameAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszOSVersionAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSVersionAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszOSServicePackAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSServicePackAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszDCName);
    LW_SAFE_FREE_MEMORY(pwszSupportedEncryptionTypesName);
    LW_SAFE_FREE_MEMORY(pwszSupportedEncryptionTypesVal[0]);
    LW_SAFE_FREE_MEMORY(pwszComputerContainer);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaGetAccountName(
    const wchar16_t *machname,
    const wchar16_t *domain_controller_name,
    const wchar16_t *dns_domain_name,
    wchar16_t       **account_name,
    BOOLEAN         *exists
    )
{
    int err = ERROR_SUCCESS;
    LDAP *ld = NULL;
    wchar16_t *base_dn = NULL;
    wchar16_t *dn = NULL;
    wchar16_t *machname_lc = NULL;
    wchar16_t *samname = NULL;     /* short name valid for SAM account */
    wchar16_t *dnsname = NULL;
    wchar16_t *fqdn = NULL;
    wchar16_t *hashstr = NULL;
    wchar16_t *samacctname = NULL; /* account name (with trailing '$') */
    UINT32    hash = 0;
    UINT32    offset = 0;
    UINT32    hashoffset = 0;
    wchar16_t newname[16] = {0};
    wchar16_t searchname[17] = {0};
    size_t    hashstrlen = 0;
    size_t    machname_len = 0;
    size_t    samacctname_len = 0;

    if (exists) *exists = FALSE;

    err = LwWc16sLen(machname, &machname_len);
    BAIL_ON_LSA_ERROR(err);

    err = LwAllocateWc16String(&machname_lc, machname);
    BAIL_ON_LSA_ERROR(err);

    wc16slower(machname_lc);

    fqdn = LdapAttrValDnsHostName(machname_lc, dns_domain_name);
    if (!fqdn)
    {
        err = ERROR_OUTOFMEMORY;
        BAIL_ON_LSA_ERROR(err);
    }

    /* look for an existing account using the dns_host_name attribute */
    err = LsaDirectoryConnect(
                domain_controller_name,
                &ld,
                &base_dn);
    BAIL_ON_LSA_ERROR(err);

    err = LsaMachDnsNameSearch(
                ld,
                fqdn,
                machname_lc,
                base_dn,
                &samname);
    if (err == ERROR_SUCCESS)
    {
        size_t samname_len = 0;

        err = LwWc16sLen(samname, &samname_len);
        BAIL_ON_LSA_ERROR(err);

        samname[samname_len - 1] = 0;

        if (exists) *exists = TRUE;
    }
    else
    {
        err = ERROR_SUCCESS;
    }

    if (!samname)
    {
        LSA_LOG_DEBUG("Machine account not found with DNS name");

        /* the host name is short enough to use as is */
        if (machname_len < 16)
        {
            LSA_LOG_DEBUG("Machine account name length < 16");

            if (sw16printfw(searchname,
                            sizeof(searchname)/sizeof(wchar16_t),
                            L"%ws$",
                            machname) < 0)
            {
                err = ErrnoToWin32Error(errno);
                BAIL_ON_LSA_ERROR(err);
            }

            err = LsaMachAcctSearch(ld, searchname, base_dn, NULL, &dnsname);
            if ( err != ERROR_SUCCESS || !dnsname)
            {
                if (err == ERROR_SUCCESS)
                {
                    if (exists) *exists = TRUE;
                    LSA_LOG_DEBUG("Machine account found with samAccountName but has no DNS name defined");
                }
                else
                {
                    LSA_LOG_DEBUG("Machine account not found with samAccountName");
                }

                err = ERROR_SUCCESS;

                err = LwAllocateWc16String(&samname, machname);
                BAIL_ON_LSA_ERROR(err);
            }

            if (dnsname) LSA_LOG_DEBUG("Machine account found with different DNS name");

            LW_SAFE_FREE_MEMORY(dnsname);
        }
    }

     /*
      * No account was found and the name is too long so hash the host
      * name and combine with as much of the existing name as will fit
      * in the available space.  Search for an existing account with
      * that name and if a collision is detected, increment the hash
      * and try again.
      */
    if (!samname)
    {
        LSA_LOG_DEBUG("Machine account name too long or found with wrong DNS name");

        dnsname = LdapAttrValDnsHostName(
                      machname_lc,
                      dns_domain_name);

        if (dnsname)
        {
            err = LsaWc16sHash(dnsname, &hash);
            BAIL_ON_LSA_ERROR(err);

            LW_SAFE_FREE_MEMORY(dnsname);
        }
        else
        {
            err = LsaWc16sHash(machname_lc, &hash);
            BAIL_ON_LSA_ERROR(err);
        }

        for (offset = 0 ; offset < 100 ; offset++)
        {
            err = LsaHashToWc16s(hash + offset, &hashstr);
            BAIL_ON_LSA_ERROR(err);

            err = LwWc16sLen(hashstr, &hashstrlen);
            BAIL_ON_LSA_ERROR(err);


            // allow for '-' + hash
            hashoffset = 15 - (hashstrlen + 1);
            if (hashoffset > machname_len)
            {
                hashoffset = machname_len;
            }
            wc16sncpy(newname, machname, hashoffset);
            newname[hashoffset++] = (WCHAR)'-';
            wc16sncpy(newname + hashoffset, hashstr, hashstrlen + 1);

            LW_SAFE_FREE_MEMORY(hashstr);

            if (sw16printfw(searchname,
                            sizeof(searchname)/sizeof(wchar16_t),
                            L"%ws$",
                            newname) < 0)
            {
                err = ErrnoToWin32Error(errno);
                BAIL_ON_LSA_ERROR(err);
            }

            LSA_LOG_DEBUG("Machine account checking hashed name");

            err = LsaMachAcctSearch(ld, searchname, base_dn, NULL, &dnsname);
            if ( err != ERROR_SUCCESS || !dnsname)
            {
                if (err == ERROR_SUCCESS)
                {
                    if (exists) *exists = TRUE;
                    LSA_LOG_DEBUG("Hashed Machine account found with samAccountName but has no DNS name defined");
                }
                else
                {
                    LSA_LOG_DEBUG("Hashed Machine account not found with samAccountName");
                }

                err = ERROR_SUCCESS;

                err = LwAllocateWc16String(&samname, newname);
                BAIL_ON_LSA_ERROR(err);

                break;
            }
            LW_SAFE_FREE_MEMORY(dnsname);
        }
        if (offset == 100)
        {
            LSA_LOG_ERROR("Failed to create unique Machine account name after 100 attempts");

            err = ERROR_DUP_NAME;
            goto error;
        }
    }

    err = LwWc16sLen(samname, &samacctname_len);
    BAIL_ON_LSA_ERROR(err);

    samacctname_len += 2;

    err = LwAllocateMemory(sizeof(samacctname[0]) * samacctname_len,
                           OUT_PPVOID(&samacctname));
    BAIL_ON_LSA_ERROR(err);

    if (sw16printfw(samacctname,
                    samacctname_len,
                    L"%ws$",
                    samname) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(err);
    }

    // Upper case the sam account name in case it was incorrectly created in AD
    // by a previous version.
    LwWc16sToUpper(samacctname);

    *account_name = samacctname;

cleanup:
    if (ld)
    {
        LsaDirectoryDisconnect(ld);
    }

    LW_SAFE_FREE_MEMORY(machname_lc);
    LW_SAFE_FREE_MEMORY(hashstr);
    LW_SAFE_FREE_MEMORY(dn);
    LW_SAFE_FREE_MEMORY(dnsname);
    LW_SAFE_FREE_MEMORY(samname);
    LW_SAFE_FREE_MEMORY(base_dn);

    return err;

error:
    LW_SAFE_FREE_MEMORY(samacctname);
    *account_name = NULL;

    goto cleanup;
}


static
DWORD
LsaGenerateMachinePassword(
    PWSTR  pwszPassword,
    size_t sPasswordLen
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszPassword = NULL;

    BAIL_ON_INVALID_POINTER(pwszPassword);
    pwszPassword[0] = (WCHAR) '\0';

    dwError = LwAllocateMemory(sizeof(pszPassword[0]) * sPasswordLen,
                               OUT_PPVOID(&pszPassword));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGenerateRandomString(pszPassword, sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* "Cast" A string to W string */
    for (i=0; i<sPasswordLen; i++)
    {
        pwszPassword[i] = (WCHAR) pszPassword[i];
    }
cleanup:
    LW_SECURE_FREE_STRING(pszPassword);
    return dwError;

error:
    goto cleanup;
}


static const CHAR RandomCharsLc[] = "abcdefghijklmnopqrstuvwxyz";
static const CHAR RandomCharsUc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const CHAR RandomCharsDigits[] = "0123456789";
static const CHAR RandomCharsPunct[] = "-+/*,.;:!<=>%'&()";

static
DWORD
LsaRandBytes(
    PBYTE pBuffer,
    DWORD dwCount
    )
{
    DWORD dwError = 0;
    unsigned long dwRandError = 0;
    const char *pszRandFile = NULL;
    int iRandLine = 0;
    char *pszData = NULL;
    int iFlags = 0;

    if (!RAND_bytes(pBuffer, dwCount))
    {
        dwRandError = ERR_get_error_line_data(
                            &pszRandFile,
                            &iRandLine,
                            (const char **)&pszData,
                            &iFlags);
        if (iFlags & ERR_TXT_STRING)
        {
            LSA_LOG_DEBUG("RAND_bytes failed with message '%s' and error code %ld at %s:%d",
                    pszData, dwRandError, pszRandFile, iRandLine);
        }
        else
        {
            LSA_LOG_DEBUG("RAND_bytes failed with error code %ld at %s:%d",
                    dwRandError, pszRandFile, iRandLine);
        }
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (iFlags & ERR_TXT_MALLOCED)
    {
        LW_SAFE_FREE_STRING(pszData);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaGenerateRandomString(
    PSTR    pszBuffer,
    size_t  sBufferLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pBuffer = NULL;
    PBYTE pClassBuffer = NULL;
    DWORD i = 0;
    DWORD iClass = 0;
    CHAR iChar = 0;
    DWORD iLcCount = 0;
    DWORD iUcCount = 0;
    DWORD iDigitsCount = 0;
    DWORD iPunctCount = 0;

    dwError = LwAllocateMemory(sizeof(pBuffer[0]) * sBufferLen,
                               OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(pClassBuffer[0]) * sBufferLen,
                               OUT_PPVOID(&pClassBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaRandBytes(pBuffer, sBufferLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaRandBytes((unsigned char*)pClassBuffer, (int)sBufferLen);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < sBufferLen-1; i++)
    {
        /*
         * Check for missing character class,
         * and force selection of the missing class.
         * The two missing classes will be at the end of
         * the string, which may be a password weakness
         * issue.
         */
        if (i >= sBufferLen-3)
        {
            if (iLcCount == 0)
            {
                iClass = 0;
            }
            else if (iUcCount == 0)
            {
                iClass = 1;
            }
            else if (iDigitsCount == 0)
            {
                iClass = 2;
            }
            else if (iPunctCount == 0)
            {
                iClass = 3;
            }
        }
        else
        {
            iClass = pClassBuffer[i] % 4;
        }

        switch (iClass)
        {
            case 0:
                iChar = RandomCharsLc[
                            pBuffer[i] % (sizeof(RandomCharsLc) - 1)];
                iLcCount++;
                break;
            case 1:
                iChar = RandomCharsUc[
                            pBuffer[i] % (sizeof(RandomCharsUc) - 1)];
                iUcCount++;
                break;
            case 2:
                iChar = RandomCharsDigits[
                            pBuffer[i] % (sizeof(RandomCharsDigits) - 1)];
                iDigitsCount++;
                break;
            case 3:
                iChar = RandomCharsPunct[
                            pBuffer[i] % (sizeof(RandomCharsPunct) - 1)];
                iPunctCount++;
                break;
        }
        pszBuffer[i] = iChar;
    }

    pszBuffer[sBufferLen-1] = '\0';

cleanup:
    LW_SECURE_FREE_MEMORY(pBuffer, sBufferLen);
    LW_SECURE_FREE_MEMORY(pClassBuffer, sBufferLen);

    return dwError;

error:
    memset(pszBuffer, 0, sizeof(pszBuffer[0]) * sBufferLen);

    goto cleanup;
}


static
NTSTATUS
LsaCreateMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    PWSTR          pwszMachinePassword,
    DWORD          dwJoinFlags,
    PWSTR         *ppwszDomainName,
    PSID          *ppDomainSid
    )
{
    const DWORD dwConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    const DWORD dwDomainAccess = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                 DOMAIN_ACCESS_CREATE_USER;

    const DWORD dwUserAccess = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    unsigned32 rpcStatus = 0;
    SAMR_BINDING hSamrBinding = NULL;
    CONNECT_HANDLE hConnect = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;
    unsigned16 sessionKeyLen = 0;
    PSID pBuiltinSid = NULL;
    DWORD dwResume = 0;
    DWORD dwSize = 256;
    PWSTR *ppwszDomainNames = NULL;
    DWORD i = 0;
    PWSTR pwszDomainName = NULL;
    DWORD dwNumEntries = 0;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    BOOLEAN bNewAccount = FALSE;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserAccessGranted = 0;
    DWORD dwRid = 0;
    DWORD dwLevel = 0;
    UserInfo *pInfo = NULL;
    DWORD dwFlagsEnable = 0;
    DWORD dwFlagsDisable = 0;
    UserInfo Info;
    size_t sMachinePasswordLen = 0;
    UserInfo PassInfo;
    PWSTR pwszMachineName = NULL;
    PUNICODE_STRING pFullName = NULL;

    memset(&Info, 0, sizeof(Info));
    memset(&PassInfo, 0, sizeof(PassInfo));

    ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                      pwszDCName,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrConnect2(hSamrBinding,
                            pwszDCName,
                            dwConnAccess,
                            &hConnect);
    BAIL_ON_NT_STATUS(ntStatus);

    rpc_binding_inq_transport_info(hSamrBinding,
                                   &hTransportInfo,
                                   &rpcStatus);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    rpc_binding_inq_prot_seq(hSamrBinding,
                             (unsigned32*)&dwProtSeq,
                             &rpcStatus);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (dwProtSeq == rpc_c_protseq_id_ncacn_np)
    {
        rpc_smb_transport_info_inq_session_key(
                                   hTransportInfo,
                                   (unsigned char**)&pSessionKey,
                                   &sessionKeyLen);
        dwSessionKeyLen = (DWORD)sessionKeyLen;
    }
    else
    {
        ntStatus = STATUS_INVALID_CONNECTION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LwAllocateWellKnownSid(WinBuiltinDomainSid,
                                   NULL,
                                   &pBuiltinSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        ntStatus = SamrEnumDomains(hSamrBinding,
                                   hConnect,
                                   &dwResume,
                                   dwSize,
                                   &ppwszDomainNames,
                                   &dwNumEntries);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        for (i = 0; pDomainSid == NULL && i < dwNumEntries; i++)
        {
            ntStatus = SamrLookupDomain(hSamrBinding,
                                        hConnect,
                                        ppwszDomainNames[i],
                                        &pSid);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!RtlEqualSid(pSid, pBuiltinSid))
            {
                dwError = LwAllocateWc16String(&pwszDomainName,
                                               ppwszDomainNames[i]);
                BAIL_ON_LSA_ERROR(dwError);

                ntStatus = RtlDuplicateSid(&pDomainSid, pSid);
                BAIL_ON_NT_STATUS(ntStatus);
            }


            SamrFreeMemory(pSid);
            pSid = NULL;
        }

        if (ppwszDomainNames)
        {
            SamrFreeMemory(ppwszDomainNames);
            ppwszDomainNames = NULL;
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    ntStatus = SamrOpenDomain(hSamrBinding,
                              hConnect,
                              dwDomainAccess,
                              pDomainSid,
                              &hDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    /* for start, let's assume the account already exists */
    bNewAccount = FALSE;

    ntStatus = SamrLookupNames(hSamrBinding,
                               hDomain,
                               1,
                               &pwszMachineAccountName,
                               &pdwRids,
                               &pdwTypes,
                               NULL);
    if (ntStatus == STATUS_NONE_MAPPED)
    {
        if (!(dwJoinFlags & LSAJOIN_ACCT_CREATE)) goto error;

        ntStatus = SamrCreateUser2(hSamrBinding,
                                   hDomain,
                                   pwszMachineAccountName,
                                   ACB_WSTRUST,
                                   dwUserAccess,
                                   &hUser,
                                   &dwUserAccessGranted,
                                   &dwRid);
        BAIL_ON_NT_STATUS(ntStatus);

        bNewAccount = TRUE;

    }
    else if (ntStatus == STATUS_SUCCESS &&
             !(dwJoinFlags & LSAJOIN_DOMAIN_JOIN_IF_JOINED))
    {
        BAIL_ON_LSA_ERROR(NERR_SetupAlreadyJoined);
    }
    else if (ntStatus == STATUS_SUCCESS)
    {
        ntStatus = SamrOpenUser(hSamrBinding,
                                hDomain,
                                dwUserAccess,
                                pdwRids[0],
                                &hUser);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /*
     * Flip ACB_DISABLED flag - this way password timeout counter
     * gets restarted
     */

    dwLevel = 16;

    ntStatus = SamrQueryUserInfo(hSamrBinding,
                                 hUser,
                                 dwLevel,
                                 &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwFlagsEnable = pInfo->info16.account_flags & (~ACB_DISABLED);
    dwFlagsDisable = pInfo->info16.account_flags | ACB_DISABLED;

    Info.info16.account_flags = dwFlagsEnable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    Info.info16.account_flags = dwFlagsDisable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    Info.info16.account_flags = dwFlagsEnable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwWc16sLen(pwszMachinePassword,
                         &sMachinePasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (bNewAccount)
    {
        UserInfo25 *pInfo25 = &PassInfo.info25;

        ntStatus = LsaEncryptPasswordBufferEx(pInfo25->password.data,
                                              sizeof(pInfo25->password.data),
                                              pwszMachinePassword,
                                              sMachinePasswordLen,
                                              pSessionKey,
                                              dwSessionKeyLen);
        BAIL_ON_NT_STATUS(ntStatus);

        dwError = LwAllocateWc16String(&pwszMachineName,
                                       pwszMachineAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        pwszMachineName[sMachinePasswordLen - 1] = '\0';

        pInfo25->info.account_flags = ACB_WSTRUST;

        pFullName = &pInfo25->info.full_name;
        dwError = LwAllocateUnicodeStringFromWc16String(
                                      pFullName,
                                      pwszMachineName);
        BAIL_ON_LSA_ERROR(dwError);

        pInfo25->info.fields_present = SAMR_FIELD_FULL_NAME |
                                       SAMR_FIELD_ACCT_FLAGS |
                                       SAMR_FIELD_PASSWORD;
        dwLevel = 25;
    }
    else
    {
        UserInfo26 *pInfo26 = &PassInfo.info26;

        ntStatus = LsaEncryptPasswordBufferEx(pInfo26->password.data,
                                              sizeof(pInfo26->password.data),
                                              pwszMachinePassword,
                                              sMachinePasswordLen,
                                              pSessionKey,
                                              dwSessionKeyLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pInfo26->password_len = sMachinePasswordLen;

        dwLevel = 26;
    }

    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &PassInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszDomainName = pwszDomainName;
    *ppDomainSid     = pDomainSid;

cleanup:
    if (hSamrBinding && hUser)
    {
        SamrClose(hSamrBinding, hUser);
    }

    if (hSamrBinding && hDomain)
    {
        SamrClose(hSamrBinding, hDomain);
    }

    if (hSamrBinding && hConnect)
    {
        SamrClose(hSamrBinding, hConnect);
    }

    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    if (pFullName)
    {
        LwFreeUnicodeString(pFullName);
    }

    if (pInfo)
    {
        SamrFreeMemory(pInfo);
    }

    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    if (ppwszDomainNames)
    {
        SamrFreeMemory(ppwszDomainNames);
    }

    LW_SAFE_FREE_MEMORY(pBuiltinSid);
    LW_SAFE_FREE_MEMORY(pwszMachineName);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    RTL_FREE(&pDomainSid);

    *ppwszDomainName = NULL;
    *ppDomainSid     = NULL;

    goto cleanup;
}


static
NTSTATUS
LsaEncryptPasswordBufferEx(
    PBYTE  pPasswordBuffer,
    DWORD  dwPasswordBufferSize,
    PWSTR  pwszPassword,
    DWORD  dwPasswordLen,
    PBYTE  pSessionKey,
    DWORD  dwSessionKeyLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    MD5_CTX ctx;
    RC4_KEY rc4_key;
    BYTE InitValue[16] = {0};
    BYTE DigestedSessKey[16] = {0};
    BYTE PasswordBuffer[532] = {0};

    BAIL_ON_INVALID_POINTER(pPasswordBuffer);
    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pSessionKey);

    // We require the first 16 bytes of the session key for the MD5 hash
    if (dwSessionKeyLen < 16)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswordBufferSize < sizeof(PasswordBuffer))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&ctx, 0, sizeof(ctx));
    memset(&rc4_key, 0, sizeof(rc4_key));

    ntStatus = LsaEncodePasswordBuffer(pwszPassword,
                                       PasswordBuffer,
                                       sizeof(PasswordBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaRandBytes((unsigned char*)InitValue, sizeof(InitValue));
    BAIL_ON_LSA_ERROR(dwError);

    MD5_Init(&ctx);
    MD5_Update(&ctx, InitValue, 16);
    MD5_Update(&ctx, pSessionKey, dwSessionKeyLen > 16 ? 16 : dwSessionKeyLen);
    MD5_Final(DigestedSessKey, &ctx);

    LSA_LOG_DEBUG("RC4_KEY structure is using %d bytes", sizeof(rc4_key));

    RC4_set_key(&rc4_key, 16, (unsigned char*)DigestedSessKey);
    RC4(&rc4_key, 516, PasswordBuffer, PasswordBuffer);

    memcpy((PVOID)&PasswordBuffer[516], InitValue, 16);

    memcpy(pPasswordBuffer, PasswordBuffer, sizeof(PasswordBuffer));

cleanup:
    memset(PasswordBuffer, 0, sizeof(PasswordBuffer));

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    DWORD dwPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    BYTE BlobInit[512] = {0};
    DWORD iByte = 0;

    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pBlob);

    if (dwBlobSize < sizeof(PasswordBlob))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Sanity check - password cannot be longer than the buffer size
     */
    if ((sPasswordLen * sizeof(pwszPassword[0])) >
        (sizeof(PasswordBlob) - sizeof(dwPasswordSize)))
    {
        dwError = ERROR_INVALID_PASSWORD;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* size doesn't include terminating zero here */
    dwPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(dwPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    /*
     * Encode the password length (in bytes) in the last 4 bytes
     * as little-endian number
     */
    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize) & 0xff);

    /*
     * Copy the password and the initial random bytes
     */
    iByte -= dwPasswordSize;
    memcpy(&(PasswordBlob[iByte]), pwszPasswordLE, dwPasswordSize);

    /*
     * Fill the rest of the buffer with (pseudo) random mess
     * to increase security.
     */
    dwError = LsaRandBytes((unsigned char*)BlobInit, iByte);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(PasswordBlob, BlobInit, iByte);

    memcpy(pBlob, PasswordBlob, sizeof(PasswordBlob));

cleanup:
    memset(PasswordBlob, 0, sizeof(PasswordBlob));

    LW_SECURE_FREE_WSTRING(pwszPasswordLE);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pBlob)
    {
        memset(pBlob, 0, dwBlobSize);
    }

    goto cleanup;
}


static
DWORD
LsaSaveMachinePassword(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachineAccountName,
    PCWSTR  pwszMachineDnsDomain,
    PCWSTR  pwszDomainName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszDCName,
    PCWSTR  pwszSidStr,
    PCWSTR  pwszPassword,
    PSTR    pszServicePrincipalNameList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszAccount = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszAdDnsDomainNameLc = NULL;
    PWSTR pwszAdDnsDomainNameUc = NULL;
    PWSTR pwszMachDnsDomainNameLc = NULL;
    PWSTR pwszSid = NULL;
    PWSTR pwszHostnameLc = NULL;
    PWSTR pwszPass = NULL;
    LSA_MACHINE_PASSWORD_INFO_W passwordInfo = { { 0 } };
    size_t sPassLen = 0;
    DWORD dwKvno = 0;
    PWSTR pwszBaseDn = NULL;
    PWSTR pwszSalt = NULL;
    /* various forms of principal name for keytab */
    PWSTR pwszPrincipal = NULL;
    PWSTR pwszFqdn = NULL;
    PWSTR pwszServiceClass = NULL;
    PWSTR principalName = NULL;
    PSTR pszSpn = NULL;
    PSTR pszServicePrincipalListSave = NULL;
    PSTR pszTmpServicePrincipalList = NULL;
    BOOLEAN bIsServiceClass = FALSE;

    dwError = LwAllocateWc16String(&pwszAccount,
                                   pwszMachineAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomain,
                                   pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszAdDnsDomainNameLc,
                                   pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszAdDnsDomainNameLc);

    dwError = LwAllocateWc16String(&pwszAdDnsDomainNameUc,
                                   pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pwszAdDnsDomainNameUc);

    dwError = LwAllocateWc16String(&pwszMachDnsDomainNameLc,
                                   pwszMachineDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszMachDnsDomainNameLc);

    dwError = LwAllocateWc16String(&pwszSid,
                                   pwszSidStr);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszHostnameLc,
                                   pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszHostnameLc);

    dwError = LwAllocateWc16sPrintfW(
                    &pwszFqdn,
                    L"%ws.%ws",
                    pwszHostnameLc,
                    pwszMachDnsDomainNameLc);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszPass,
                                   pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Find the current key version number for machine account
     */

    dwError = KtKrb5FormatPrincipalW(pwszAccount,
                                     pwszAdDnsDomainNameUc,
                                     &pwszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    KtLdapSetSaslMaxBufSize(LsaSrvSaslMaxBufSize());

    /* Get the directory base naming context first */
    dwError = KtLdapGetBaseDnW(pwszDCName, &pwszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtLdapGetKeyVersionW(pwszDCName,
                                   pwszBaseDn,
                                   pwszPrincipal,
                                   &dwKvno);
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        /*
         * This is probably win2k DC we're talking to, because it doesn't
         * store kvno in directory. In such case return default key version
         */
        dwKvno = 0;
        dwError = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Store the machine password
     */

    passwordInfo.Account.DnsDomainName = pwszAdDnsDomainNameUc;
    passwordInfo.Account.NetbiosDomainName = pwszDomain;
    passwordInfo.Account.DomainSid = pwszSid;
    passwordInfo.Account.SamAccountName = pwszAccount;
    passwordInfo.Account.AccountFlags = LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
    passwordInfo.Account.KeyVersionNumber = dwKvno;
    passwordInfo.Account.Fqdn = pwszFqdn;
    passwordInfo.Account.LastChangeTime = 0;
    passwordInfo.Password = pwszPass;

    dwError = LsaPstoreSetPasswordInfoW(&passwordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszPass, &sPassLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtKrb5GetSaltingPrincipalW(pwszMachineName,
                                         pwszAccount,
                                         pwszMachineDnsDomain,
                                         pwszAdDnsDomainNameUc,
                                         pwszDCName,
                                         pwszBaseDn,
                                         &pwszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszSalt == NULL)
    {
        dwError = LwAllocateWc16String(&pwszSalt, pwszPrincipal);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Update keytab records with various forms of machine principal
     */


    // MACHINE$@DOMAIN.NET

    dwError = LsaSavePrincipalKey(
                  pwszAccount,
                  pwszPass,
                  sPassLen,
                  pwszAdDnsDomainNameUc,
                  pwszSalt,
                  pwszDCName,
                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pszServicePrincipalNameList, &pszTmpServicePrincipalList);
    BAIL_ON_LSA_ERROR(dwError);

    pszSpn = strtok_r(pszTmpServicePrincipalList, ",", &pszServicePrincipalListSave);
    while (pszSpn)
    {
       if (pwszServiceClass)
          LW_SAFE_FREE_MEMORY(pwszServiceClass);

       GroomSpn(&pszSpn, &bIsServiceClass);

       LSA_LOG_VERBOSE("Generating keytab entry for %s", pszSpn);

       dwError = LwMbsToWc16s(pszSpn, &pwszServiceClass);
       BAIL_ON_LSA_ERROR(dwError);

       if (!bIsServiceClass)
       {
          // Add pszSpn as is.
          dwError = LwAllocateWc16sPrintfW(&principalName, L"%ws", pwszServiceClass);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);
          LW_SAFE_FREE_MEMORY(principalName);
       }
       else
       {
          // serviceclass/MACHINE@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        TRUE,
                        NULL,
                        FALSE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);

          // serviceclass/machine@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        FALSE,
                        NULL,
                        FALSE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);

          // serviceclass/MACHINE.DOMAIN.NET@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        TRUE,
                        pwszMachineDnsDomain,
                        TRUE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);

          // serviceclass/machine.domain.net@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        FALSE,
                        pwszMachineDnsDomain,
                        FALSE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);

          // serviceclass/MACHINE.domain.net@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        TRUE,
                        pwszMachineDnsDomain,
                        FALSE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);

          // serviceclass/machine.DOMAIN.NET@DOMAIN.NET

          dwError = LsaBuildPrincipalName(
                        &principalName,
                        pwszServiceClass,
                        pwszMachineName,
                        FALSE,
                        pwszMachineDnsDomain,
                        TRUE);
          BAIL_ON_LSA_ERROR(dwError);

          dwError = LsaSavePrincipalKey(
                        principalName,
                        pwszPass,
                        sPassLen,
                        pwszAdDnsDomainNameUc,
                        pwszSalt,
                        pwszDCName,
                        dwKvno);
          BAIL_ON_LSA_ERROR(dwError);

          LW_SAFE_FREE_MEMORY(principalName);
       }

       pszSpn = strtok_r(NULL, ",", &pszServicePrincipalListSave);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(principalName);
    LW_SAFE_FREE_MEMORY(pwszBaseDn);
    LW_SAFE_FREE_MEMORY(pwszSalt);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszAdDnsDomainNameLc);
    LW_SAFE_FREE_MEMORY(pwszAdDnsDomainNameUc);
    LW_SAFE_FREE_MEMORY(pwszMachDnsDomainNameLc);
    LW_SAFE_FREE_MEMORY(pwszSid);
    LW_SAFE_FREE_MEMORY(pwszHostnameLc);
    LW_SAFE_FREE_MEMORY(pwszPass);
    LW_SAFE_FREE_MEMORY(pwszAccount);
    LW_SAFE_FREE_MEMORY(pwszPrincipal);
    LW_SAFE_FREE_MEMORY(pwszFqdn);
    LW_SAFE_FREE_MEMORY(pwszServiceClass);
    LW_SAFE_FREE_STRING(pszTmpServicePrincipalList);

    return dwError;

error:
    goto cleanup;
}


////////////////////////////////////////////////////////////////////////

static
DWORD
LsaBuildPrincipalName(
    OUT PWSTR *ppPrincipalName,
    IN PCWSTR InstanceName,
    IN PCWSTR HostName,
    IN BOOLEAN UpperCaseHostName,
    IN OPTIONAL PCWSTR RealmName,
    IN OPTIONAL BOOLEAN UpperCaseRealmName
    )
{
    DWORD dwError= ERROR_SUCCESS;
    PWSTR hostName = NULL;
    PWSTR realmName = NULL;
    PWSTR principalName = NULL;

    dwError = LwAllocateWc16String(&hostName, HostName);
    BAIL_ON_LSA_ERROR(dwError);

    if (UpperCaseHostName)
    {
        LwWc16sToUpper(hostName);
    }
    else
    {
        LwWc16sToLower(hostName);
    }

    if (RealmName)
    {
        dwError = LwAllocateWc16String(&realmName, RealmName);
        BAIL_ON_LSA_ERROR(dwError);

        if (UpperCaseRealmName)
        {
            LwWc16sToUpper(realmName);
        }
        else
        {
            LwWc16sToLower(realmName);
        }
    }

    if (realmName)
    {
        dwError = LwAllocateWc16sPrintfW(
                      &principalName,
                      L"%ws/%ws.%ws",
                      InstanceName,
                      hostName,
                      realmName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateWc16sPrintfW(
                      &principalName,
                      L"%ws/%ws",
                      InstanceName,
                      hostName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppPrincipalName = principalName;

error:
    if (dwError != ERROR_SUCCESS)
    {
        LW_SAFE_FREE_MEMORY(principalName);
    }

    LW_SAFE_FREE_MEMORY(hostName);
    LW_SAFE_FREE_MEMORY(realmName);

    return dwError;
}


////////////////////////////////////////////////////////////////////////

static
DWORD
LsaSavePrincipalKey(
    PCWSTR  pwszName,
    PCWSTR  pwszPassword,
    DWORD   dwPasswordLen,
    PCWSTR  pwszRealm,
    PCWSTR  pwszSalt,
    PCWSTR  pwszDCName,
    DWORD   dwKvno
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszPrincipal = NULL;

    BAIL_ON_INVALID_POINTER(pwszName);
    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pwszDCName);

    dwError = KtKrb5FormatPrincipalW(pwszName,
                                     pwszRealm,
                                     &pwszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtKrb5AddKeyW(pwszPrincipal,
                            (PVOID)pwszPassword,
                            dwPasswordLen,
                            NULL,
                            pwszSalt,
                            pwszDCName,
                            dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszPrincipal);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaDirectoryConnect(
    PCWSTR pDomain,
    LDAP** ppLdConn,
    PWSTR* ppDefaultContext
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAP *pLdConn = NULL;
    // Do not free
    LDAPMessage *pInfo = NULL;
    LDAPMessage *pRes = NULL;
    PWSTR pAttributeName = NULL;
    PWSTR* ppAttributeValue = NULL;
    PWSTR pDefaultContext = NULL;

    BAIL_ON_INVALID_POINTER(pDomain);
    BAIL_ON_INVALID_POINTER(ppLdConn);
    BAIL_ON_INVALID_POINTER(ppDefaultContext);

    dwError = LdapInitConnection(&pLdConn, pDomain, FALSE);
    BAIL_ON_LSA_ERROR(dwError);

    lderr = LdapGetDirectoryInfo(&pInfo, &pRes, pLdConn);
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwMbsToWc16s("defaultNamingContext",
                           &pAttributeName);
    BAIL_ON_LSA_ERROR(dwError);

    ppAttributeValue = LdapAttributeGet(pLdConn, pInfo, pAttributeName, NULL);
    if (ppAttributeValue == NULL) {
        /* TODO: find more descriptive error code */
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LDAP_ERROR(lderr);
    }

    dwError = LwAllocateWc16String(&pDefaultContext, ppAttributeValue[0]);
    BAIL_ON_LSA_ERROR(dwError);

    *ppLdConn = pLdConn;
    *ppDefaultContext = pDefaultContext;

cleanup:
    LW_SAFE_FREE_MEMORY(pAttributeName);

    if (ppAttributeValue)
    {
        LdapAttributeValueFree(ppAttributeValue);
    }

    if (pRes)
    {
        LdapMessageFree(pRes);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:
    if (pLdConn)
    {
        LdapCloseConnection(pLdConn);
    }
    LW_SAFE_FREE_MEMORY(pDefaultContext);

    *ppLdConn = NULL;
    *ppDefaultContext = NULL;
    goto cleanup;
}


static
DWORD
LsaDirectoryDisconnect(
    LDAP *ldconn
    )
{
    int lderr = LdapCloseConnection(ldconn);
    return LwMapLdapErrorToLwError(lderr);
}


static
DWORD
LsaMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    BOOLEAN move,
    BOOLEAN exists
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *machacct = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *info = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;
    wchar16_t *dn_name = NULL;
    wchar16_t **dn_val = NULL;

    BAIL_ON_INVALID_POINTER(ld);
    BAIL_ON_INVALID_POINTER(machine_name);
    BAIL_ON_INVALID_POINTER(machacct_name);
    BAIL_ON_INVALID_POINTER(ou);

    if (exists == FALSE)
    {
        lderr = LdapMachAcctCreate(ld, machine_name, machacct_name, ou);
        BAIL_ON_LDAP_ERROR(lderr);
    }
    else if (move)
    {
        lderr = LdapGetDirectoryInfo(&info, &res, ld);
        BAIL_ON_LDAP_ERROR(lderr);

        dwError = LwMbsToWc16s("defaultNamingContext",
                &dn_context_name);
        BAIL_ON_LSA_ERROR(dwError);

        dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
        if (dn_context_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctSearch(&machacct, ld, machacct_name, dn_context_val[0]);

        // If the machine account with this sAMAccountName doesn't exist then we have a naming conflict
        if (lderr == LDAP_NO_SUCH_OBJECT) lderr = LDAP_ALREADY_EXISTS;
        BAIL_ON_LDAP_ERROR(lderr);

        dwError = LwMbsToWc16s("distinguishedName", &dn_name);
        BAIL_ON_LSA_ERROR(dwError);

        dn_val = LdapAttributeGet(ld, machacct, dn_name, NULL);
        if (dn_val == NULL) {
            dwError = LW_ERROR_LDAP_INSUFFICIENT_ACCESS;
            goto error;
        }

        lderr = LdapMachAcctMove(ld, dn_val[0], machine_name, ou);
        BAIL_ON_LDAP_ERROR(lderr);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(dn_context_name);
    LW_SAFE_FREE_MEMORY(dn_name);

    if (res)
    {
        LdapMessageFree(res);
    }

    if (machacct)
    {
        LdapMessageFree(machacct);
    }

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (dn_val) {
        LdapAttributeValueFree(dn_val);
    }

    if (dwError == ERROR_SUCCESS && lderr != 0)
    {
        dwError = LwMapLdapErrorToLwError(lderr);

        if (dwError == LW_ERROR_UNKNOWN && move)
        {
            /* provide a more useful error if possible */
            dwError = LW_ERROR_LDAP_RENAME_FAILED;
        }
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaMachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *fqdn,
    const wchar16_t *machname,
    const wchar16_t *dn_context,
    wchar16_t **samacct
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *samacct_attr_name = NULL;
    wchar16_t **samacct_attr_val = NULL;

    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(fqdn);
    BAIL_ON_INVALID_POINTER(dn_context);
    BAIL_ON_INVALID_POINTER(samacct);

    *samacct = NULL;

    // Attempt to find the computer account using the CN and FQDN (improve performance for pre-staged accounts and re-joining)
    lderr = LdapMachDnsNameSearch(
                &res,
                ldconn,
                fqdn,
                machname,
                dn_context);

    if (lderr != LDAP_SUCCESS) {
        // Couldn't find using the CN so try the old dNSHostName search
        lderr = LdapMachDnsNameSearch(
                    &res,
                    ldconn,
                    fqdn,
                    NULL,
                    dn_context);
    }
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwMbsToWc16s("sAMAccountName",
                           &samacct_attr_name);
    BAIL_ON_LSA_ERROR(dwError);

    samacct_attr_val = LdapAttributeGet(ldconn, res, samacct_attr_name, NULL);
    if (!samacct_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LDAP_ERROR(lderr);
    }

    dwError = LwAllocateWc16String(samacct, samacct_attr_val[0]);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(samacct_attr_name);
    LdapAttributeValueFree(samacct_attr_val);

    if (res)
    {
        LdapMessageFree(res);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:

    *samacct = NULL;
    goto cleanup;
}


static
DWORD
LsaMachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **pDn,
    wchar16_t **pDnsName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *dn_attr_name = NULL;
    wchar16_t **dn_attr_val = NULL;
    wchar16_t *dns_name_attr_name = NULL;
    wchar16_t **dns_name_attr_val = NULL;
    wchar16_t *dn = NULL;
    wchar16_t *dnsName = NULL;

    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(name);
    BAIL_ON_INVALID_POINTER(dn_context);

    lderr = LdapMachAcctSearch(&res, ldconn, name, dn_context);
    BAIL_ON_LDAP_ERROR(lderr);

    if (pDn)
    {
        dwError = LwMbsToWc16s("distinguishedName", &dn_attr_name);
        BAIL_ON_LSA_ERROR(dwError);

        dn_attr_val = LdapAttributeGet(ldconn, res, dn_attr_name, NULL);
        if (!dn_attr_val) {
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            BAIL_ON_LDAP_ERROR(lderr);
        }

        dwError = LwAllocateWc16String(&dn, dn_attr_val[0]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDnsName)
    {
        dwError = LwMbsToWc16s("dNSHostName", &dns_name_attr_name);
        BAIL_ON_LSA_ERROR(dwError);

        dns_name_attr_val = LdapAttributeGet(
                                ldconn,
                                res,
                                dns_name_attr_name,
                                NULL);
        if (dns_name_attr_val) {
            dwError = LwAllocateWc16String(&dnsName, dns_name_attr_val[0]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (pDn)
    {
        *pDn = dn;
    }
    if (pDnsName)
    {
        *pDnsName = dnsName;
    }

    LW_SAFE_FREE_MEMORY(dn_attr_name);
    LdapAttributeValueFree(dn_attr_val);
    LW_SAFE_FREE_MEMORY(dns_name_attr_name);
    LdapAttributeValueFree(dns_name_attr_val);

    if (res) {
        LdapMessageFree(res);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LwMapLdapErrorToLwError(lderr);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(dn);
    LW_SAFE_FREE_MEMORY(dnsName);
    goto cleanup;
}


static
DWORD
LsaMachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    )
{
    int lderr = LDAP_SUCCESS;

    lderr = LdapMachAcctSetAttribute(ldconn, dn, attr_name, attr_val, new);
    return LwMapLdapErrorToLwError(lderr);
}


DWORD
LsaGetDcName(
    const wchar16_t *DnsDomainName,
    BOOLEAN Force,
    wchar16_t** DomainControllerName
    )
{
    DWORD dwError = 0;
    wchar16_t *domain_controller_name = NULL;
    char *dns_domain_name_mbs = NULL;
    DWORD get_dc_name_flags = 0;
    PLWNET_DC_INFO pDC = NULL;

    if (Force)
    {
        get_dc_name_flags |= DS_FORCE_REDISCOVERY;
    }

    dwError = LwWc16sToMbs(DnsDomainName, &dns_domain_name_mbs);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDCName(NULL, dns_domain_name_mbs, NULL, get_dc_name_flags, &pDC);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pDC->pszDomainControllerName,
                           &domain_controller_name);
    BAIL_ON_LSA_ERROR(dwError)

cleanup:
    LW_SAFE_FREE_MEMORY(dns_domain_name_mbs);
    LWNET_SAFE_FREE_DC_INFO(pDC);
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(domain_controller_name);
    }

    *DomainControllerName = domain_controller_name;

    // ISSUE-2008/07/14-dalmeida -- Need to do error code conversion

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaGetRwDcName(
    const wchar16_t *DnsDomainName,
    BOOLEAN Force,
    wchar16_t** DomainControllerName
    )
{
    DWORD dwError = 0;
    wchar16_t *domain_controller_name = NULL;
    char *dns_domain_name_mbs = NULL;
    DWORD get_dc_name_flags = DS_WRITABLE_REQUIRED;
    PLWNET_DC_INFO pDC = NULL;

    if (Force)
    {
        get_dc_name_flags |= DS_FORCE_REDISCOVERY;
    }

    dwError = LwWc16sToMbs(DnsDomainName, &dns_domain_name_mbs);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDCName(NULL, dns_domain_name_mbs, NULL, get_dc_name_flags, &pDC);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pDC->pszDomainControllerName,
                           &domain_controller_name);
    BAIL_ON_LSA_ERROR(dwError)

cleanup:
    LW_SAFE_FREE_MEMORY(dns_domain_name_mbs);
    LWNET_SAFE_FREE_DC_INFO(pDC);
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(domain_controller_name);
    }

    *DomainControllerName = domain_controller_name;

    // ISSUE-2008/07/14-dalmeida -- Need to do error code conversion

    return dwError;

error:
    goto cleanup;
}


DWORD
LsaEnableDomainGroupMembership(
    PCSTR pszDomainName,
    PCSTR pszDomainSID
    )
{
    return LsaChangeDomainGroupMembership(pszDomainName,
                                          pszDomainSID,
					  TRUE);
}


DWORD
LsaChangeDomainGroupMembership(
    IN  PCSTR    pszDomainName,
    IN  PCSTR    pszDomainSID,
    IN  BOOLEAN  bEnable
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pDomainSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pBuiltinUsersSid = NULL;
    PSID pDomainAdminsSid = NULL;
    PSID pDomainUsersSid = NULL;
    PSTR pszBuiltinAdminsSid = NULL;
    PSTR pszBuiltinUsersSid = NULL;
    PSTR pszDomainAdminsSid = NULL;
    PSTR pszDomainUsersSid = NULL;
    LSA_GROUP_MOD_INFO_2 adminsMods = {0};
    LSA_GROUP_MOD_INFO_2 usersMods = {0};
    PSTR pszTargetProvider = NULL;
    HANDLE hConnection = NULL;

    ntStatus = RtlAllocateSidFromCString(
                   &pDomainSid,
                   pszDomainSID);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinBuiltinUsersSid,
                                   NULL,
                                   &pBuiltinUsersSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinAccountDomainAdminsSid,
                                   pDomainSid,
                                   &pDomainAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinAccountDomainUsersSid,
                                   pDomainSid,
                                   &pDomainUsersSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlAllocateCStringFromSid(
                  &pszBuiltinAdminsSid,
                  pBuiltinAdminsSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAllocateCStringFromSid(
                  &pszBuiltinUsersSid,
                  pBuiltinUsersSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAllocateCStringFromSid(
                  &pszDomainAdminsSid,
                  pDomainAdminsSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAllocateCStringFromSid(
                  &pszDomainUsersSid,
                  pDomainUsersSid);
    BAIL_ON_NT_STATUS(ntStatus);

    adminsMods.pszSid = pszBuiltinAdminsSid;
    if (bEnable)
    {
        adminsMods.actions.bAddMembers = TRUE;
        adminsMods.dwAddMembersNum = 1;
        adminsMods.ppszAddMembers = &pszDomainAdminsSid;
    }
    else
    {
        adminsMods.actions.bRemoveMembers = TRUE;
        adminsMods.dwRemoveMembersNum = 1;
        adminsMods.ppszRemoveMembers = &pszDomainAdminsSid;
    }

    usersMods.pszSid = pszBuiltinUsersSid;
    if (bEnable)
    {
        usersMods.actions.bAddMembers = TRUE;
        usersMods.dwAddMembersNum = 1;
        usersMods.ppszAddMembers = &pszDomainUsersSid;
    }
    else
    {
        usersMods.actions.bRemoveMembers = TRUE;
        usersMods.dwRemoveMembersNum = 1;
        usersMods.ppszRemoveMembers = &pszDomainUsersSid;
    }

    dwError = LwAllocateStringPrintf(
                  &pszTargetProvider,
                  ":%s",
                  pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvOpenServer(0, 0, getpid(), &hConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvModifyGroup2(
                  hConnection,
                  pszTargetProvider,
                  &adminsMods);
    if ((bEnable && (dwError == ERROR_MEMBER_IN_ALIAS ||
                     dwError == ERROR_MEMBER_IN_GROUP)) ||
        (!bEnable && (dwError == ERROR_MEMBER_NOT_IN_ALIAS ||
                      dwError == ERROR_MEMBER_NOT_IN_GROUP)))
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvModifyGroup2(
                  hConnection,
                  pszTargetProvider,
                  &usersMods);
    if ((bEnable && (dwError == ERROR_MEMBER_IN_ALIAS ||
                     dwError == ERROR_MEMBER_IN_GROUP)) ||
        (!bEnable && (dwError == ERROR_MEMBER_NOT_IN_ALIAS ||
                      dwError == ERROR_MEMBER_NOT_IN_GROUP)))
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

error:
    LW_SAFE_FREE_MEMORY(pDomainSid);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pBuiltinUsersSid);
    LW_SAFE_FREE_MEMORY(pDomainAdminsSid);
    LW_SAFE_FREE_MEMORY(pDomainUsersSid);
    LW_SAFE_FREE_MEMORY(pszBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pszBuiltinUsersSid);
    LW_SAFE_FREE_MEMORY(pszDomainAdminsSid);
    LW_SAFE_FREE_MEMORY(pszDomainUsersSid);
    LW_SAFE_FREE_MEMORY(pszTargetProvider);

    if (hConnection)
    {
        LsaSrvCloseServer(hConnection);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;
}


DWORD
LsaMachineChangePassword(
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN PSTR           pszServicePrincipalNameList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszDnsDomainName = NULL;
    PWSTR pwszDCName = NULL;
    size_t sDCNameLen = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;
    PWSTR pwszUserName = NULL;
    PWSTR pwszOldPassword = NULL;
    WCHAR wszNewPassword[MACHPASS_LEN+1];
    PWSTR pwszHostname = NULL;
    PCWSTR pwszFqdnSuffix = NULL;
    int i = 0;

    memset(wszNewPassword, 0, sizeof(wszNewPassword));

    if (pszDnsDomainName)
    {
        dwError = LwMbsToWc16s(pszDnsDomainName, &pwszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPstoreGetPasswordInfoW(pwszDnsDomainName, &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetDcName(pPasswordInfo->Account.DnsDomainName, FALSE,
                            &pwszDCName);
    BAIL_ON_LSA_ERROR(dwError);

    pwszUserName     = pPasswordInfo->Account.SamAccountName;
    pwszOldPassword  = pPasswordInfo->Password;

    dwError = LwAllocateWc16String(&pwszHostname, pPasswordInfo->Account.Fqdn);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; pwszHostname[i]; i++)
    {
        if ('.' == pwszHostname[i])
        {
            pwszHostname[i] = 0;
            pwszFqdnSuffix = &pwszHostname[i+1];
            break;
        }
    }

    LsaGenerateMachinePassword(
                  wszNewPassword,
                  sizeof(wszNewPassword)/sizeof(wszNewPassword[0]));

    dwError = LwWc16sLen(pwszDCName, &sDCNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUserChangePassword(pwszDCName,
                                    pwszUserName,
                                    pwszOldPassword,
                                    (PWSTR)wszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO-2010/01/10-dalmeida -- Simplify this calling sequence
    // by using keytab plugin in lsapstore...

    dwError = LsaSaveMachinePassword(
                    pwszHostname,
                    pPasswordInfo->Account.SamAccountName,
                    pwszFqdnSuffix ? pwszFqdnSuffix : pPasswordInfo->Account.DnsDomainName,
                    pPasswordInfo->Account.NetbiosDomainName,
                    pPasswordInfo->Account.DnsDomainName,
                    pwszDCName,
                    pPasswordInfo->Account.DomainSid,
                    wszNewPassword,
                    pszServicePrincipalNameList);
    BAIL_ON_LSA_ERROR(dwError);

error:
    LW_SAFE_FREE_MEMORY(pwszDCName);
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LSA_PSTORE_FREE_PASSWORD_INFO_W(&pPasswordInfo);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);

    return dwError;
}


DWORD
LsaUserChangePassword(
    PWSTR  pwszDCName,
    PWSTR  pwszUserName,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    SAMR_BINDING hSamrBinding = NULL;
    size_t sOldPasswordLen = 0;
    size_t sNewPasswordLen = 0;
    BYTE OldNtHash[16] = {0};
    BYTE NewNtHash[16] = {0};
    BYTE NtPasswordBuffer[516] = {0};
    BYTE NtVerHash[16] = {0};
    RC4_KEY RC4Key;
    PIO_CREDS pCreds = NULL;

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrInitBindingDefault(&hSamrBinding, pwszDCName, pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwWc16sLen(pwszOldPassword, &sOldPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszNewPassword, &sNewPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* prepare NT password hashes */
    dwError = LsaGetNtPasswordHash(pwszOldPassword,
                                   OldNtHash,
                                   sizeof(OldNtHash));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetNtPasswordHash(pwszNewPassword,
                                   NewNtHash,
                                   sizeof(NewNtHash));
    BAIL_ON_LSA_ERROR(dwError);

    /* encode password buffer */
    dwError = LsaEncodePasswordBuffer(pwszNewPassword,
                                      NtPasswordBuffer,
                                      sizeof(NtPasswordBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    RC4_set_key(&RC4Key, 16, (unsigned char*)OldNtHash);
    RC4(&RC4Key, sizeof(NtPasswordBuffer), NtPasswordBuffer, NtPasswordBuffer);

    /* encode NT verifier */
    dwError = LsaEncryptNtHashVerifier(NewNtHash, sizeof(NewNtHash),
                                       OldNtHash, sizeof(OldNtHash),
                                       NtVerHash, sizeof(NtVerHash));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrChangePasswordUser2(hSamrBinding,
                                       pwszDCName,
                                       pwszUserName,
                                       NtPasswordBuffer,
                                       NtVerHash,
                                       0,
                                       NULL,
                                       NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    memset(OldNtHash, 0, sizeof(OldNtHash));
    memset(NewNtHash, 0, sizeof(NewNtHash));
    memset(NtPasswordBuffer, 0, sizeof(NtPasswordBuffer));

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pNtHash,
    IN  DWORD   dwNtHashSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE Hash[16] = {0};

    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pNtHash);

    if (dwNtHashSize < sizeof(Hash))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Make sure the password is 2-byte little-endian
     */
    dwError = LwAllocateMemory((sPasswordLen + 1) * sizeof(pwszPasswordLE[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    MD4((PBYTE)pwszPasswordLE,
        sPasswordLen * sizeof(pwszPasswordLE[0]),
        Hash);

    memcpy(pNtHash, Hash, sizeof(Hash));

cleanup:
    LW_SECURE_FREE_WSTRING(pwszPasswordLE);

    memset(Hash, 0, sizeof(Hash));

    return dwError;

error:
    memset(pNtHash, 0, dwNtHashSize);

    goto cleanup;
}


static
DWORD
LsaEncryptNtHashVerifier(
    IN  PBYTE    pNewNtHash,
    IN  DWORD    dwNewNtHashLen,
    IN  PBYTE    pOldNtHash,
    IN  DWORD    dwOldNtHashLen,
    OUT PBYTE    pNtVerifier,
    IN  DWORD    dwNtVerifierSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE Verifier[16] = {0};

    BAIL_ON_INVALID_POINTER(pNewNtHash);
    BAIL_ON_INVALID_POINTER(pOldNtHash);
    BAIL_ON_INVALID_POINTER(pNtVerifier);

    if (dwNtVerifierSize < sizeof(Verifier))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    dwError = LsaPrepareDesKey(&pNewNtHash[0],
			       (PBYTE)KeyBlockLo);
    BAIL_ON_LSA_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    dwError = LsaPrepareDesKey(&pNewNtHash[7],
			       (PBYTE)KeyBlockHi);
    BAIL_ON_LSA_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[0],
                    (DES_cblock*)&Verifier[0],
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[8],
                    (DES_cblock*)&Verifier[8],
                    &KeyHi,
                    DES_ENCRYPT);

    memcpy(pNtVerifier, Verifier, sizeof(Verifier));

cleanup:
    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pInput);
    BAIL_ON_INVALID_POINTER(pOutput);

    /*
     * Expand the input 7x8 bits so that each 7 bits are
     * appended with 1 bit space for parity bit and yield
     * 8x8 bits ready to become a DES key
     */
    pOutput[0] = pInput[0] >> 1;
    pOutput[1] = ((pInput[0]&0x01) << 6) | (pInput[1] >> 2);
    pOutput[2] = ((pInput[1]&0x03) << 5) | (pInput[2] >> 3);
    pOutput[3] = ((pInput[2]&0x07) << 4) | (pInput[3] >> 4);
    pOutput[4] = ((pInput[3]&0x0F) << 3) | (pInput[4] >> 5);
    pOutput[5] = ((pInput[4]&0x1F) << 2) | (pInput[5] >> 6);
    pOutput[6] = ((pInput[5]&0x3F) << 1) | (pInput[6] >> 7);
    pOutput[7] = pInput[6]&0x7F;

    for (i = 0; i < 8; i++)
    {
        pOutput[i] = pOutput[i] << 1;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}
