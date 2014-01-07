/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_wbc_sid.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include "util_str.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <lwmem.h>

#define MAX_SID_STRING_LEN 1024

wbcErr
wbcSidCopy(
    struct wbcDomainSid *dst,
    struct wbcDomainSid *src
    )
{    
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbcStatus = WBC_ERR_UNKNOWN_FAILURE;
    

    BAIL_ON_NULL_PTR_PARAM(dst, dwErr);
    BAIL_ON_NULL_PTR_PARAM(src, dwErr);

    memcpy(dst, src, sizeof(struct wbcDomainSid));

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    wbcStatus = map_error_to_wbc_status(dwErr);
    
    return wbcStatus;
}

wbcErr
wbcSidAppendRid(
    struct wbcDomainSid *sid,
    DWORD rid
    )
{
    DWORD dwErr = LW_ERROR_INTERNAL;    

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    /* See if there is room */

    if (sid->num_auths >= WBC_MAXSUBAUTHS) {
        dwErr = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERR(dwErr);
    }

    sid->sub_auths[sid->num_auths] = rid;
    sid->num_auths++;

    dwErr = LW_ERROR_SUCCESS;

cleanup:

    return dwErr;
}

const char*
wbcSidTypeString(
    enum wbcSidType type
    )
{
    switch (type)
    {
        case WBC_SID_NAME_USE_NONE:
            return "None";
        case WBC_SID_NAME_USER:
            return "User";
        case WBC_SID_NAME_DOM_GRP:
            return "Domain Group";
        case WBC_SID_NAME_DOMAIN:
            return "Domain";
        case WBC_SID_NAME_ALIAS:
            return "Alias";
        case WBC_SID_NAME_WKN_GRP:
            return "Workstation Group";
        case WBC_SID_NAME_DELETED:
            return "Deleted Name";
        case WBC_SID_NAME_INVALID:
            return "Invalid Name";
        case WBC_SID_NAME_UNKNOWN:
            return "Unknown";
        case WBC_SID_NAME_COMPUTER:
            return "Computer";
        default:
            return "Unknown";
    }
}

int wbcSidToStringBuf(
    const struct wbcDomainSid *sid,
    char *buf,
    int buflen
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    uint32_t dwAuthId = 0;
    int i = 0;
    DWORD dwErr = LW_ERROR_INTERNAL;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);
    BAIL_ON_NULL_PTR_PARAM(buf, dwErr);

    dwAuthId = sid->id_auth[5] +
        (sid->id_auth[4] << 8) +
        (sid->id_auth[3] << 16) +
        (sid->id_auth[2] << 24);

    snprintf(buf,
         buflen,
         "S-%d-%d",
         sid->sid_rev_num,
         dwAuthId);

    for (i=0; i<sid->num_auths; i++) {
        char pszAuth[12];

        snprintf(pszAuth, sizeof(pszAuth), "-%u", sid->sub_auths[i]);
        strncat(buf, pszAuth, buflen-strlen(buf));
    }

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    wbc_status = map_error_to_wbc_status(dwErr);

    if (wbc_status)
    {
        return -1;
    }
    else
    {
        return strlen(buf);
    }
}

wbcErr wbcSidToString(
    const struct wbcDomainSid *sid,
    char **sid_string
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    CHAR pszSidStr[MAX_SID_STRING_LEN] = "";
    uint32_t dwAuthId = 0;
    int i = 0;
    DWORD dwErr = LW_ERROR_INTERNAL;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);
    BAIL_ON_NULL_PTR_PARAM(sid_string, dwErr);

    dwAuthId = sid->id_auth[5] +
        (sid->id_auth[4] << 8) +
        (sid->id_auth[3] << 16) +
        (sid->id_auth[2] << 24);

    snprintf(pszSidStr,
         sizeof(pszSidStr)-strlen(pszSidStr),
         "S-%d-%d",
         sid->sid_rev_num,
         dwAuthId);

    for (i=0; i<sid->num_auths; i++) {
        char pszAuth[12];

        snprintf(pszAuth, sizeof(pszAuth), "-%u", sid->sub_auths[i]);
        strncat(pszSidStr, pszAuth, sizeof(pszSidStr)-strlen(pszSidStr));
    }

    *sid_string = _wbc_strdup(pszSidStr);
    BAIL_ON_NULL_PTR(*sid_string, dwErr);

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}


wbcErr
wbcStringToSid(
    const char *sid_string,
    struct wbcDomainSid *sid
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwErr = LW_ERROR_INTERNAL;
    const CHAR *pszStrToken = NULL;
    CHAR *pszStrNextToken = NULL;
    DWORD dwX;

    BAIL_ON_NULL_PTR_PARAM(sid_string, dwErr);
    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    /* Some additional sanity checks on the SID string format */

    if ((strlen((const char*)sid_string) < 2)
        || (sid_string[0] != 's' && sid_string[0] != 'S')
        || (sid_string[1] != '-'))
    {
        dwErr = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Revision */

    pszStrToken = sid_string+2;
    dwX = (DWORD)strtol(pszStrToken, &pszStrNextToken, 10);
    if ((dwX == 0) || !pszStrNextToken || (pszStrNextToken[0] != '-')) {
        dwErr = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERR(dwErr);
    }
    sid->sid_rev_num = (uint8_t)dwX;

    /* Id Auth */

    pszStrToken = pszStrNextToken + 1;
    dwX = (DWORD)strtoul(pszStrToken, &pszStrNextToken, 10);
    if ((dwX == 0) || !pszStrNextToken || (pszStrNextToken[0] != '-')) {
        dwErr = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERR(dwErr);
    }

    sid->id_auth[5] = (dwX & 0x000000FF);
    sid->id_auth[4] = (dwX & 0x0000FF00) >> 8;
    sid->id_auth[3] = (dwX & 0x00FF0000) >> 16;
    sid->id_auth[2] = (dwX & 0xFF000000) >> 24;
    sid->id_auth[1] = 0;
    sid->id_auth[0] = 0;

    /* Subauths */

    sid->num_auths = 0;
    do {
        pszStrToken = pszStrNextToken + 1;

        errno = 0;
        dwX = (DWORD)strtoul(pszStrToken, &pszStrNextToken, 10);
        if (errno || pszStrToken == pszStrNextToken) {
            break;
        }

        sid->sub_auths[sid->num_auths++] = dwX;

        if (!pszStrNextToken || (pszStrNextToken[0] != '-')) {
            break;
        }

    } while (sid->num_auths < WBC_MAXSUBAUTHS);

    /* Check for a premature end to the above loop */

    if (pszStrNextToken && (pszStrNextToken[0] != '\0')) {
        dwErr = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERR(dwErr);
    }

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr
wbcLookupName(
    const char *dom_name,
    const char *name,
    struct wbcDomainSid *sid,
    enum wbcSidType *name_type
    )
{
    LSA_USER_INFO_0 *pUserInfo = NULL;
    LSA_GROUP_INFO_1 *pGroupInfo = NULL;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    CHAR pszQualifiedName[512] = "";

    BAIL_ON_NULL_PTR_PARAM(name, dwErr);

    if (dom_name) {
        snprintf(pszQualifiedName, sizeof(pszQualifiedName),
            "%s\\", dom_name);
    }
    strncat(pszQualifiedName, name, sizeof(pszQualifiedName) - (strlen(pszQualifiedName)+1));

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    /* First try to lookup the name as a user */

    dwErr = LsaFindUserByName(hLsa, pszQualifiedName, 0, (PVOID*)&pUserInfo);
    if (dwErr ==  LW_ERROR_SUCCESS) {
        if (sid) {
            wbc_status = wbcStringToSid(pUserInfo->pszSid, sid);
            dwErr = map_wbc_to_lsa_error(wbc_status);
            BAIL_ON_LSA_ERR(dwErr);
        }
        if (name_type) {
            *name_type = WBC_SID_NAME_USER;
        }
        goto cleanup;
    }

    /* Fall back and try it as a group */

    dwErr = LsaFindGroupByName(hLsa, pszQualifiedName, LSA_FIND_FLAGS_NSS, 1, (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERR(dwErr);
    if (sid) {
        wbc_status = wbcStringToSid(pGroupInfo->pszSid, sid);
        dwErr = map_wbc_to_lsa_error(wbc_status);
        BAIL_ON_LSA_ERR(dwErr);
    }
    if (name_type) {
        *name_type = WBC_SID_NAME_DOM_GRP;
    }

    dwErr = LsaCloseServer(hLsa);
    BAIL_ON_LSA_ERR(dwErr);
    hLsa = (HANDLE)NULL;

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if ( hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (pUserInfo) {
        LsaFreeUserInfo(0, pUserInfo);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(1, pGroupInfo);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

static enum wbcSidType
map_lsa_sid_type_to_wbc(
    ADAccountType type
    )
{
    if (type == AccountType_User)
        return WBC_SID_NAME_USER;

    if (type == AccountType_Group)
        return WBC_SID_NAME_DOM_GRP;

    return WBC_SID_NAME_UNKNOWN;
}

wbcErr wbcLookupSid(
    const struct wbcDomainSid *sid,
    char **domain,
    char **name,
    enum wbcSidType *name_type
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_INTERNAL;
    PSTR pszSidString = NULL;
    PSTR ppszSidList[2];
    PLSA_SID_INFO pNameList = NULL;

    BAIL_ON_NULL_PTR_PARAM(sid, dwErr);

    /* Validate the SID */

    wbc_status = wbcSidToString(sid, &pszSidString);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

    ppszSidList[0] = pszSidString;
    ppszSidList[1] = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetNamesBySidList(
                hLsa,
                1,
                ppszSidList,
        &pNameList,
                NULL);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    /* Fill in return values here  Some of these could have been
       passed in as NULL */

    if (pNameList[0].accountType == AccountType_NotFound) {
        dwErr = LW_ERROR_NOT_MAPPED;
        BAIL_ON_LSA_ERR(dwErr);
    }

    if (domain) {
        *domain = _wbc_strdup(pNameList[0].pszDomainName);
        BAIL_ON_NULL_PTR(*domain, dwErr);

        StrUpper(*domain);
    }

    if (name) {
        *name = _wbc_strdup(pNameList[0].pszSamAccountName);
        BAIL_ON_NULL_PTR(*name, dwErr);
    }

    if (name_type) {
        *name_type = map_lsa_sid_type_to_wbc(pNameList[0].accountType);
    }

    dwErr = LW_ERROR_SUCCESS;

cleanup:
    if (pNameList) {
        LsaFreeSIDInfoList(pNameList, 1);
    }

    if (pszSidString) {
        wbcFreeMemory(pszSidString);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
        hLsa = (HANDLE)NULL;
    }

    if (dwErr != LW_ERROR_SUCCESS) {
        _WBC_FREE(*domain);
        _WBC_FREE(*name);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

static
int
_wbc_free_translated_names(
    void *p
    )
{
    struct wbcTranslatedName *pNames = (struct wbcTranslatedName *)p;
    int index = 0;

    if (pNames)
    {
        for (index = 0; pNames[index].type != -1; index++)
        {
            _WBC_FREE(pNames[index].name);
        }
    }
    return 0;
}

wbcErr wbcLookupSids(
    const struct wbcDomainSid *sids,
    int num_sids,
    struct wbcDomainInfo **ppDomains,
    int *pNumDomains,
    struct wbcTranslatedName **ppNames
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    HANDLE hLsa = (HANDLE)NULL;
    DWORD dwErr = LW_ERROR_SUCCESS;
    PSTR pszSidString = NULL;
    PSTR* ppszSidList = NULL;
    PLSA_SID_INFO pNameList = NULL;
    int index = 0;
    int domainIndex = 0;
    size_t numDomains = 0;
    struct wbcTranslatedName *pNames = NULL;
    struct wbcDomainInfo *pDomains = NULL;

    BAIL_ON_NULL_PTR_PARAM(sids, dwErr);
    BAIL_ON_NULL_PTR_PARAM(ppDomains, dwErr);
    BAIL_ON_NULL_PTR_PARAM(pNumDomains, dwErr);
    BAIL_ON_NULL_PTR_PARAM(ppNames, dwErr);

    wbc_status = wbcListTrusts(
                    &pDomains,
                    &numDomains);
    dwErr = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LwAllocateMemory(
                sizeof(ppszSidList[0]) * (num_sids + 1),
                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERR(dwErr);

    for (index = 0; index < num_sids; index++)
    {
        wbc_status = wbcSidToString(&sids[index], &pszSidString);
        dwErr = map_wbc_to_lsa_error(wbc_status);
        BAIL_ON_LSA_ERR(dwErr);

        ppszSidList[index] = pszSidString;
        pszSidString = NULL;
    }

    ppszSidList[index] = NULL;

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetNamesBySidList(
                hLsa,
                1,
                ppszSidList,
                &pNameList,
                NULL);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaCloseServer(hLsa);
    hLsa = (HANDLE)NULL;
    BAIL_ON_LSA_ERR(dwErr);

    pNames = _wbc_malloc_zero(sizeof(pNames[0])*(num_sids + 1),
                 _wbc_free_translated_names);
    BAIL_ON_NULL_PTR(pNames, dwErr);

    pNames[num_sids].type = -1;

    for (index = 0; index < num_sids; index++)
    {
        if (pNameList[index].accountType == AccountType_NotFound)
        {
            pNames[index].type = WBC_SID_NAME_USE_NONE;
        }
        else
        {
            pNames[index].type = map_lsa_sid_type_to_wbc(
                    pNameList[index].accountType);

            pNames[index].name = _wbc_strdup(
                    pNameList[index].pszSamAccountName);
            BAIL_ON_NULL_PTR(pNames[index].name, dwErr);

            pNames[index].domain_index = -1;
            for (domainIndex = 0; domainIndex < numDomains; domainIndex++)
            {
                if (!strcmp(pDomains[domainIndex].short_name,
                            pNameList[index].pszDomainName) ||
                    !strcmp(pDomains[domainIndex].dns_name,
                            pNameList[index].pszDomainName))
                {
                    pNames[index].domain_index = domainIndex;
                    break;
                }
            }
        }
    }

cleanup:
    if (dwErr)
    {
        *pNumDomains = 0;
        *ppNames = NULL;
        *ppDomains = NULL;
        _WBC_FREE(pNames);
        _WBC_FREE(pDomains);
    }
    else
    {
        *pNumDomains = numDomains;
        *ppNames = pNames;
        *ppDomains = pDomains;
    }
    if (pNameList)
    {
        LsaFreeSIDInfoList(pNameList, num_sids);
    }
    if (ppszSidList)
    {
        for (index = 0; index < num_sids; index++)
        {
            wbcFreeMemory(ppszSidList[index]);
        }
        LW_SAFE_FREE_MEMORY(ppszSidList);
    }

    if (pszSidString) {
        wbcFreeMemory(pszSidString);
    }

    if (hLsa) {
        LsaCloseServer(hLsa);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr wbcLookupRids(
    struct wbcDomainSid *dom_sid,
    int num_rids,
    uint32_t *rids,
    const char **domain_name,
    const char ***names,
    enum wbcSidType **types
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwErr = LW_ERROR_INTERNAL;
    struct wbcDomainSid sid;
    int i;
        char *domain = NULL;

    BAIL_ON_NULL_PTR_PARAM(dom_sid, dwErr);
    BAIL_ON_NULL_PTR_PARAM(rids, dwErr);

    if (names) {
        /* Add one more to the end to NULL terminate the
           array of strings.  Required by _wbc_free_string_array() */

        *names = _wbc_malloc_zero(sizeof(char*)*(num_rids+1),
                     _wbc_free_string_array);
        BAIL_ON_NULL_PTR(*names, dwErr);
    }

    if (types) {
        *types = _wbc_malloc_zero(sizeof(enum wbcSidType)*num_rids, NULL);
        BAIL_ON_NULL_PTR(*types, dwErr);
    }

    for (i=0; i<num_rids; i++) {
        dwErr = wbcSidCopy(&sid, dom_sid);
        BAIL_ON_LSA_ERR(dwErr);

        dwErr = wbcSidAppendRid(&sid, rids[i]);
        BAIL_ON_LSA_ERR(dwErr);

        wbc_status = wbcLookupSid(&sid,
                      &domain,
                      &(*(char***)names)[i],
                      &(*types)[i]);
        dwErr = map_wbc_to_lsa_error(wbc_status);
        BAIL_ON_LSA_ERR(dwErr);

        /* Only copy the domaoin name one time */

        if (domain_name && !*domain_name) {
            *domain_name = _wbc_strdup(domain);
            BAIL_ON_NULL_PTR(*domain_name, dwErr);

            StrUpper((char *)(*domain_name));
        }

        wbcFreeMemory(domain);
        domain = NULL;
    }

cleanup:
    if (domain) {
        wbcFreeMemory(domain);
    }

    if (dwErr != LW_ERROR_SUCCESS) {
        if (types && *types)
            _WBC_FREE(*types);
        if (names && *names)
            _WBC_FREE(*names)
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}

wbcErr
wbcGetSidAliases(
    const struct wbcDomainSid *dom_sid,
    struct wbcDomainSid *sids,
    uint32_t num_sids,
    uint32_t **alias_rids,
    uint32_t *num_alias_rids
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}

DWORD
wbcFindSecurityObjectBySid(
    IN const struct wbcDomainSid *sid,
    PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD error = 0;
    HANDLE hLsa = (HANDLE)NULL;
    PSTR pszSidString = NULL;
    PCSTR ppszSidList[2] = { NULL, NULL };
    LSA_QUERY_LIST query = { 0 };
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    wbcErr wbc_status = 0;

    BAIL_ON_NULL_PTR_PARAM(sid, error);

    /* Validate the SID */

    wbc_status = wbcSidToString(sid, &pszSidString);
    error = map_wbc_to_lsa_error(wbc_status);
    BAIL_ON_LSA_ERR(error);

    ppszSidList[0] = pszSidString;
    query.ppszStrings = ppszSidList;

    error = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(error);

    error = LsaFindObjects(
                hLsa,
                NULL,
                0,
                LSA_OBJECT_TYPE_USER,
                LSA_QUERY_TYPE_BY_SID,
                1,
                query,
                &ppResults);
    BAIL_ON_LSA_ERR(error);

    if (!ppResults[0])
    {
        error = LW_ERROR_NOT_MAPPED;
        BAIL_ON_LSA_ERR(error);
    }

    *ppResult = ppResults[0];
    LW_SAFE_FREE_MEMORY(ppResults);

cleanup:
    _WBC_FREE(pszSidString);

    if (hLsa)
    {
        LsaCloseServer(hLsa);
    }

    if (error != LW_ERROR_SUCCESS)
    {
        if (ppResults)
        {
            LsaFreeSecurityObjectList(1, ppResults);
        }
        *ppResult = NULL;
    }

    return map_error_to_wbc_status(error);
}

// Return the gecos
wbcErr
wbcGetDisplayName(
    const struct wbcDomainSid *sid,
    char **domain,
    char **name,
    enum wbcSidType *name_type
    )
{
    DWORD error = 0;
    PLSA_SECURITY_OBJECT pResult = NULL;

    BAIL_ON_NULL_PTR_PARAM(sid, error);

    error = wbcFindSecurityObjectBySid(
        sid,
        &pResult);
    BAIL_ON_LSA_ERR(error);

    if (domain)
    {
        *domain = _wbc_strdup(pResult->pszNetbiosDomainName);
        BAIL_ON_NULL_PTR(*domain, error);

        StrUpper(*domain);
    }

    if (name)
    {
        *name = _wbc_strdup(pResult->userInfo.pszGecos);
        BAIL_ON_NULL_PTR(*name, error);
    }

    if (name_type)
    {
        *name_type = map_lsa_sid_type_to_wbc(pResult->type);
    }

cleanup:
    if (pResult)
    {
        LsaFreeSecurityObject(pResult);
    }

    if (error != LW_ERROR_SUCCESS)
    {
        if (domain)
        {
            _WBC_FREE(*domain);
        }
        if (name)
        {
            _WBC_FREE(*name);
        }
        if (name_type)
        {
            *name_type = 0;
        }
    }

    return map_error_to_wbc_status(error);
}

