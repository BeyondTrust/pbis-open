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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        adldap.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP helper functions (public header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __ADLDAP_H__
#define __ADLDAP_H__

DWORD
ADGetDefaultDomainPrefixedName(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pAlias,
    OUT PLSA_LOGIN_NAME_INFO* ppPrefixedName
    );

DWORD
ADGetDomainQualifiedString(
    PCSTR pszNetBIOSDomainName,
    PCSTR pszName,
    PSTR* ppszQualifiedName
    );

DWORD
ADGetLDAPUPNString(
    IN OPTIONAL HANDLE hDirectory,
    IN OPTIONAL LDAPMessage* pMessage,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszSamaccountName,
    OUT PSTR* ppszUPN,
    OUT PBOOLEAN pbIsGeneratedUPN
    );

DWORD
ADFindComputerDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR pszSamAccountName,
    PCSTR pszDomainName,
    PSTR* ppszComputerDN
    );

DWORD
ADGetCellInformation(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    PSTR*  ppszCellContainer
    );

DWORD
ADGetDomainMaxPwdAge(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDomainName,
    PUINT64 pMaxPwdAge);

DWORD
ADGetConfigurationMode(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    ADConfigurationMode* pADConfMode
    );

DWORD
ADisJoinedToAD(
    BOOLEAN *pbIsJoinedToAD
    );

DWORD
ADGuidStrToHex(
    PCSTR pszStr,
    PSTR* ppszHexStr
    );

DWORD
ADLdap_GetGroupMembers(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PCSTR pszDomainName,
    IN PCSTR pszSid,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
ADLdap_GetObjectGroupMembership(
    IN PAD_PROVIDER_CONTEXT pContext,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT int* piPrimaryGroupIndex,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppGroupInfoList
    );

DWORD
ADLdap_IsValidDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR    pszDN,
    PBOOLEAN pbValidDN
    );

DWORD
ADLdap_GetObjectSid(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszSid
    );

#endif

