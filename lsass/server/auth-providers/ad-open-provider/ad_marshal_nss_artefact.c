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
 *        provider-main.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
    long nArtefact = 0;
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

