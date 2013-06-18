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
 *        AD LDAP Group Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

static
DWORD
ADMarshalNSSArtefactInfoList_0(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessage,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pwdNumNSSArtefacts
    );

DWORD
ADSchemaMarshalNSSArtefactInfoList(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessagePseudo,
    DWORD         dwNSSArtefactInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pNumNSSArtefacts
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD NumNSSArtefacts = 0;

    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADSchemaMarshalNSSArtefactInfoList_0(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            dwMapFlags,
                            &ppNSSArtefactInfoList,
                            &NumNSSArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pNumNSSArtefacts = NumNSSArtefacts;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pNumNSSArtefacts = 0;

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, (PVOID*)ppNSSArtefactInfoList, NumNSSArtefacts);
    }
    goto cleanup;
}

DWORD
ADSchemaMarshalNSSArtefactInfoList_0(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessagePseudo,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pwdNumNSSArtefacts
    )
{
    return ADMarshalNSSArtefactInfoList_0(
                    hDirectory,
                    pszDomainName,
                    pMessagePseudo,
                    dwMapFlags,
                    pppNSSArtefactInfoList,
                    pwdNumNSSArtefacts);
}

DWORD
ADNonSchemaMarshalNSSArtefactInfoList(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessagePseudo,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    DWORD         dwNSSArtefactInfoLevel,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pNumArtefacts
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD dwNumArtefacts = 0;

    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADNonSchemaMarshalNSSArtefactInfoList_0(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            dwMapFlags,
                            &ppNSSArtefactInfoList,
                            &dwNumArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            dwError = LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pNumArtefacts = dwNumArtefacts;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pNumArtefacts = 0;

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, (PVOID*)ppNSSArtefactInfoList, dwNumArtefacts);
    }
    goto cleanup;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfoList_0(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessagePseudo,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pwdNumNSSArtefacts
    )
{
    return ADMarshalNSSArtefactInfoList_0(
                    hDirectory,
                    pszDomainName,
                    pMessagePseudo,
                    dwMapFlags,
                    pppNSSArtefactInfoList,
                    pwdNumNSSArtefacts);
}

static
DWORD
ADMarshalNSSArtefactInfoList_0(
    HANDLE        hDirectory,
    PCSTR         pszDomainName,
    LDAPMessage*  pMessagePseudo,
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags,
    PVOID**       pppNSSArtefactInfoList,
    PDWORD        pwdNumNSSArtefacts
    )
{
    DWORD dwError = 0;
    PLSA_NSS_ARTEFACT_INFO_0* ppArtefactInfoList = NULL;
    PLSA_NSS_ARTEFACT_INFO_0  pArtefactInfo = NULL;
    DWORD iArtefact = 0;
    DWORD nArtefact = 0;
    DWORD dwArtefactInfoLevel = 0;
    // Do not free
    LDAPMessage *pArtefactMessage = NULL;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    LDAP *pLd = NULL;

    if (!pMessagePseudo)
    {
        goto done;
    }

    pLd = LwLdapGetSession(hDirectory);

    nArtefact = ldap_count_entries(
                    pLd,
                    pMessagePseudo);
    if (nArtefact < 0) {
        dwError = LW_ERROR_LDAP_ERROR;
    } else if (nArtefact == 0) {
        dwError = LW_ERROR_NO_MORE_NSS_ARTEFACTS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(PLSA_NSS_ARTEFACT_INFO_0) * nArtefact,
                    (PVOID*)&ppArtefactInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    pArtefactMessage = ldap_first_entry(
                            pLd,
                            pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);

    while (pArtefactMessage)
    {
        dwError = LwAllocateMemory(
                        sizeof(LSA_NSS_ARTEFACT_INFO_0),
                        (PVOID*)&pArtefactInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwLdapGetString(
                        hDirectory,
                        pArtefactMessage,
                        "name",
                        &pArtefactInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwMapFlags & LSA_NIS_MAP_QUERY_VALUES)
        {
            if (ppszValues) {
                LwFreeStringArray(ppszValues, dwNumValues);
                ppszValues = NULL;
            }

            dwError = LwLdapGetStrings(
                            hDirectory,
                            pArtefactMessage,
                            AD_LDAP_KEYWORDS_TAG,
                            &ppszValues,
                            &dwNumValues);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADNonSchemaKeywordGetString(
                            ppszValues,
                            dwNumValues,
                            "value",
                            &pArtefactInfo->pszValue);
            BAIL_ON_LSA_ERROR(dwError);
        }

        ppArtefactInfoList[iArtefact++] = pArtefactInfo;
        pArtefactInfo = NULL;

        pArtefactMessage = ldap_next_entry(
                                     pLd,
                                     pArtefactMessage);
    }

done:

    *pppNSSArtefactInfoList = (PVOID*)ppArtefactInfoList;
    *pwdNumNSSArtefacts = iArtefact;

cleanup:

    if (ppszValues) {
        LwFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pwdNumNSSArtefacts = 0;

    if (ppArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwArtefactInfoLevel, (PVOID*)ppArtefactInfoList, nArtefact);
    }

    if (pArtefactInfo)
    {
        LsaFreeNSSArtefactInfo(dwArtefactInfoLevel, pArtefactInfo);
    }

    goto cleanup;
}

