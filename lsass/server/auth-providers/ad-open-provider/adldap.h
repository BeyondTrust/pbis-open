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
 *        adldap.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
ADGetUserPrimaryGroupSid(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDomainDnsName,
    IN PCSTR pszUserDN,
    IN PCSTR pszUserObjectsid,
    OUT PSTR* ppszPrimaryGroupSID
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
ADGetUserOrGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList
    );

DWORD
ADGetUserRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList
    );

DWORD
ADGetUserPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList
    );

DWORD
ADGetGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList
    );

DWORD
ADGetGroupPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList
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

DWORD
ADLdap_GetAccountType(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    OUT LSA_OBJECT_TYPE* pAccountType
    );

#endif

