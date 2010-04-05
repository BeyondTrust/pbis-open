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
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
CellModeFindNSSArtefactByKey(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszCellDN,
    PCSTR  pszNetBIOSDomainName,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PVOID pNSSArtefactInfo = NULL;

    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = ADGetConfigurationMode(
                         pConn,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode)
    {
       case SchemaMode:
           dwError = CellModeSchemaFindNSSArtefactByKey(
                           pConn,
                           pszCellDN,
                           pszNetBIOSDomainName,
                           pszKeyName,
                           pszMapName,
                           dwInfoLevel,
                           dwFlags,
                           &pNSSArtefactInfo);
           BAIL_ON_LSA_ERROR(dwError);
           break;

       case NonSchemaMode:
           dwError = CellModeNonSchemaFindNSSArtefactByKey(
                           pConn,
                           pszCellDN,
                           pszNetBIOSDomainName,
                           pszKeyName,
                           pszMapName,
                           dwInfoLevel,
                           dwFlags,
                           &pNSSArtefactInfo);
           BAIL_ON_LSA_ERROR(dwError);
           break;
       case UnknownMode:
           break;
    }

    *ppNSSArtefactInfo = pNSSArtefactInfo;

cleanup:

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (pNSSArtefactInfo) {
      LsaFreeNSSArtefactInfo(dwInfoLevel, pNSSArtefactInfo);
    }

    goto cleanup;
}

DWORD
CellModeSchemaFindNSSArtefactByKey(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszCellDN,
    PCSTR  pszNetBIOSDomainName,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PSTR   pszQuery = NULL;
    PSTR   pszDN = NULL;
    PSTR   pszEscapedDN = NULL;
    PSTR szAttributeList[] =
        {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
        };

    LDAPMessage *pMessagePseudo = NULL;
    PVOID* ppArtefactInfos = NULL;
    DWORD dwNumInfos = 0;
    LDAP *pLd = NULL;
    BOOLEAN bMapExists = FALSE;
    HANDLE hDirectory = NULL;

    BAIL_ON_INVALID_STRING(pszMapName);
    BAIL_ON_INVALID_STRING(pszKeyName);

    dwError = LwAllocateStringPrintf(
                    &pszDN,
                    "CN=%s,CN=Maps,%s",
                    pszMapName,
                    pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapEscapeString(
                   &pszEscapedDN,
                   pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_IsValidDN(
                    pConn,
                    pszEscapedDN,
                    &bMapExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bMapExists)
    {
        dwError = LW_ERROR_NO_SUCH_NSS_MAP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszQuery,
                    "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry)(name=%s))",
                    pszKeyName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapDirectorySearch(
                   pConn,
                   pszEscapedDN,
                   LDAP_SCOPE_ONELEVEL,
                   pszQuery,
                   szAttributeList,
                   &hDirectory,
                   &pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LwLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                      pLd,
                      pMessagePseudo);
    if (dwCount < 0) {
       dwError = LW_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LW_ERROR_NO_SUCH_NSS_KEY;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADSchemaMarshalNSSArtefactInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,
                    pMessagePseudo,
                    dwInfoLevel,
                    dwFlags,
                    &ppArtefactInfos,
                    &dwNumInfos);
    BAIL_ON_LSA_ERROR(dwError);

    *ppNSSArtefactInfo = *ppArtefactInfos;
    *ppArtefactInfos = NULL;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    if (ppArtefactInfos)
    {
        LsaFreeNSSArtefactInfoList(dwInfoLevel, ppArtefactInfos, dwNumInfos);
    }

    LW_SAFE_FREE_STRING(pszDN);
    LW_SAFE_FREE_STRING(pszEscapedDN);
    LW_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT)
    {
        dwError = LW_ERROR_NO_SUCH_NSS_KEY;
    }

    goto cleanup;
}

DWORD
CellModeNonSchemaFindNSSArtefactByKey(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszCellDN,
    PCSTR  pszNetBIOSDomainName,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PVOID pNSSArtefactInfo = NULL;

    dwError = DefaultModeNonSchemaFindNSSArtefactByKey(
                       pConn,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pszKeyName,
                       pszMapName,
                       dwInfoLevel,
                       dwFlags,
                       &pNSSArtefactInfo);
    BAIL_ON_LSA_ERROR(dwError);

    //ToDo: enum linked cell groups

    *ppNSSArtefactInfo = pNSSArtefactInfo;

cleanup:

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (pNSSArtefactInfo)
    {
        LsaFreeNSSArtefactInfo(dwInfoLevel, pNSSArtefactInfo);
    }

    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT)
    {
        dwError = LW_ERROR_NO_SUCH_NSS_MAP;
    }

    goto cleanup;
}

DWORD
CellModeEnumNSSArtefacts(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PVOID* ppNSSArtefactInfoList = NULL;

    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = ADGetConfigurationMode(
                         pConn,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode)
    {
       case SchemaMode:
           dwError = CellModeSchemaEnumNSSArtefacts(
                       pConn,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;

       case NonSchemaMode:
           dwError = CellModeNonSchemaEnumNSSArtefacts(
                       pConn,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;
       case UnknownMode:
           break;
    }

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
      LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    goto cleanup;
}

DWORD
CellModeSchemaEnumNSSArtefacts(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PSTR   pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry))";
    PSTR   pszDN = NULL;
    PSTR   pszEscapedDN = NULL;
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;
    PSTR szAttributeList[] =
        {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
        };

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = NULL;
    HANDLE hDirectory = NULL;

    DWORD dwNumNSSArtefactsWanted = dwMaxNumNSSArtefacts;

    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;

    BAIL_ON_INVALID_STRING(pEnumState->pszMapName);

    dwError = LwAllocateStringPrintf(
                   &pszDN,
                   "CN=%s,CN=Maps,%s",
                   pEnumState->pszMapName,
                   pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapEscapeString(
                   &pszEscapedDN,
                   pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (pEnumState->Cookie.bSearchFinished){
        dwError = LW_ERROR_NO_MORE_NSS_ARTEFACTS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    do
    {
        dwError = LsaDmLdapDirectoryOnePagedSearch(
                       pConn,
                       pszEscapedDN,
                       pszQuery,
                       szAttributeList,
                       dwNumNSSArtefactsWanted,
                       &pEnumState->Cookie,
                       LDAP_SCOPE_SUBTREE,
                       &hDirectory,
                       &pMessagePseudo);
        BAIL_ON_LSA_ERROR(dwError);

        pLd = LwLdapGetSession(hDirectory);

        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);
        if (dwCount < 0) {
           dwError = LW_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LW_ERROR_NO_MORE_NSS_ARTEFACTS;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADSchemaMarshalNSSArtefactInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,
                        dwNSSArtefactInfoLevel,
                        pEnumState->dwMapFlags,
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound);
        BAIL_ON_LSA_ERROR(dwError);

        dwNumNSSArtefactsWanted -= dwNumNSSArtefactsFound;

        dwError = LsaAppendAndFreePtrs(
                        &dwTotalNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (pMessagePseudo) {
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
        }
    } while (!pEnumState->Cookie.bSearchFinished && dwNumNSSArtefactsWanted);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    LW_SAFE_FREE_STRING(pszDN);
    LW_SAFE_FREE_STRING(pszEscapedDN);

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    if (ppNSSArtefactInfoList_accumulate) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList_accumulate, dwTotalNumNSSArtefactsFound);
    }

    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT)
    {
        dwError = LW_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}

DWORD
CellModeNonSchemaEnumNSSArtefacts(
    PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;

    dwError = DefaultModeNonSchemaEnumNSSArtefacts(
                       pConn,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    //ToDo: enum linked cell groups

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;

    if (ppNSSArtefactInfoList) {
          LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    if (dwError == LW_ERROR_LDAP_NO_SUCH_OBJECT)
    {
        dwError = LW_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}

