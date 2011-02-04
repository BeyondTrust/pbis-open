/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
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
 *        lpauthex.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AuthenticateUserEx routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"

/* Forward Declarations */

static DWORD
FillAuthUserInfo(
    HANDLE hProvider,
    PLSA_AUTH_USER_INFO pAuthInfo,
    PLSA_SECURITY_OBJECT pObject,
    PCSTR pszMachineName
    );

static DWORD
SidSplitString(
    IN OUT PSTR pszSidString,
    OUT PDWORD pdwRid
    );

static DWORD
AuthenticateNTLMv1(
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT PLSA_DATA_BLOB *ppSessionKeyBlob
    );

static DWORD
AuthenticateNTLMv2(
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT PLSA_DATA_BLOB *ppSessionKeyBlob
    );

/* Code */

/********************************************************
 *******************************************************/

DWORD
LocalAuthenticateUserExInternal(
    HANDLE                hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO*  ppUserInfo
    )
{
    DWORD    dwError = LW_ERROR_INTERNAL;
    PCSTR    pszDomain = NULL;
    PSTR     pszAccountName = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    BOOLEAN bUsingNTLMv2 = FALSE;
    BOOLEAN bAcceptNTLMv1 = TRUE;
    PLSA_DATA_BLOB pSessionKey = NULL;
    LSA_QUERY_LIST QueryList;
    BOOLEAN bUserIsGuest = FALSE;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_INVALID_POINTER(pUserParams->pszAccountName);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    /* Always assume authenticating a user from the local domain */
    LOCAL_RDLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);
    pszDomain = gLPGlobals.pszLocalDomain;

    dwError = LwAllocateStringPrintf(&pszAccountName,
                                     "%s\\%s",
                                     pszDomain,
                                     pUserParams->pszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    QueryList.ppszStrings = (PCSTR*) &pszAccountName;

    dwError = LocalFindObjects(
        hProvider,
        0,
        LSA_OBJECT_TYPE_USER,
        LSA_QUERY_TYPE_BY_NT4,
        1,
        QueryList,
        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppObjects[0] == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCheckIsGuest(ppObjects[0], &bUserIsGuest);
    BAIL_ON_LSA_ERROR(dwError);

    if (bUserIsGuest)
    {
        dwError = LW_ERROR_LOGON_FAILURE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCheckAccountFlags(ppObjects[0]);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgAcceptNTLMv1(&bAcceptNTLMv1);
    BAIL_ON_LSA_ERROR(dwError);

    /* generate the responses and compare */

    if (LsaDataBlobLength(pUserParams->pass.chap.pNT_resp) == 24)
    {
        bUsingNTLMv2 = FALSE;

        if (!bAcceptNTLMv1)
        {
            dwError = ERROR_INVALID_LOGON_TYPE;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = AuthenticateNTLMv1(pUserParams,
                                     ppObjects[0],
                                     &pSessionKey);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        bUsingNTLMv2 = TRUE;

        dwError = AuthenticateNTLMv2(pUserParams,
                                     ppObjects[0],
                                     &pSessionKey);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Fill in the LSA_AUTH_USER_INF0 data now */

    dwError = LwAllocateMemory(sizeof(LSA_AUTH_USER_INFO),
                                (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = FillAuthUserInfo(hProvider, pUserInfo, ppObjects[0], pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->pSessionKey = pSessionKey;
    pSessionKey = NULL;    

    *ppUserInfo = pUserInfo;
    pUserInfo = NULL;    

cleanup:
    LOCAL_UNLOCK_RWLOCK(bLocked, &gLPGlobals.rwlock);

    LsaUtilFreeSecurityObjectList(1, ppObjects);

    LsaFreeAuthUserInfo(&pUserInfo);

    if (pSessionKey) {
        LsaDataBlobFree(&pSessionKey);
    }    

    LW_SAFE_FREE_MEMORY(pszAccountName);

    return dwError;

error:

    goto cleanup;
}

/********************************************************
 *******************************************************/

static
DWORD
AuthenticateNTLMv1(
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT PLSA_DATA_BLOB *ppSessionKeyBlob
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD    dwError = LW_ERROR_INTERNAL;
    BYTE     NTResponse[24] = { 0 };
    PBYTE    pChal = NULL;
    PBYTE    pNTresp = NULL;
    PBYTE    pSessKeyBuf = NULL;
    PLSA_DATA_BLOB pSessKeyBlob = NULL;    

    /* Authenticate */

    pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);
    BAIL_ON_INVALID_POINTER(pChal);

    ntError = NetrNTLMv1EncryptChallenge(pChal,
                                         NULL,     /* ignore LM hash */
                                         pObject->userInfo.pNtHash,
                                         NULL,
                                         NTResponse);
    if (ntError != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pNTresp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
    BAIL_ON_INVALID_POINTER(pNTresp);

    if (memcmp(pNTresp, NTResponse, 24) != 0)
    {
        dwError = LW_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Calculate the session Key */

    dwError = LsaDataBlobAllocate(&pSessKeyBlob, 16);
    BAIL_ON_LSA_ERROR(dwError);    

    pSessKeyBuf = LsaDataBlobBuffer(pSessKeyBlob);

    MD4(pObject->userInfo.pNtHash, 16, pSessKeyBuf);

    *ppSessionKeyBlob = pSessKeyBlob;
    pSessKeyBlob = NULL;    

    /* Done */

    dwError = LW_ERROR_SUCCESS;    

cleanup:

    if (pSessKeyBlob) {
        LsaDataBlobFree(&pSessKeyBlob);
    }    

    return dwError;

error:

    goto cleanup;    
}



/********************************************************
 *******************************************************/

static DWORD
AuthenticateNTLMv2(
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    IN PLSA_SECURITY_OBJECT pObject,
    OUT PLSA_DATA_BLOB *ppSessionKeyBlob
    )
{
    DWORD dwError = LW_ERROR_PASSWORD_MISMATCH;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszDestination = NULL;
    BYTE pNTLMv2Hash[16];
    DWORD dwNTLMv2HashLen = 16;    
    PBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    DWORD dwAcctNameSize = 0;
    DWORD dwDestNameSize = 0;
    PBYTE pClientNonce = NULL;
    DWORD dwClientNonceLen = 0;
    PBYTE pChal = NULL;
    PBYTE pNTresp = NULL;
    DWORD dwNTrespLen = 0;
    PBYTE pData = NULL;
    BYTE pNTLMv2Resp[EVP_MAX_MD_SIZE];
    DWORD dwNTLMv2RespLen = 0;
    PLSA_DATA_BLOB pSessKeyBlob = NULL;    
    PBYTE pSessKeyBuf = NULL;
    DWORD dwSessKeyLen = 0;    

    /* Sanity Check */

    BAIL_ON_INVALID_POINTER(pUserParams->pass.chap.pNT_resp);
    BAIL_ON_INVALID_POINTER(pUserParams->pass.chap.pChallenge);

    pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);
    pNTresp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);

    BAIL_ON_INVALID_POINTER(pChal);
    BAIL_ON_INVALID_POINTER(pNTresp);

    dwNTrespLen = LsaDataBlobLength(pUserParams->pass.chap.pNT_resp);

    memset(pNTLMv2Hash, 0, 16);
    
    /* First calculate the NTLMv2 Hash */

    dwError = LwMbsToWc16s(pUserParams->pszAccountName,
                            &pwszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pUserParams->pszDomain,
                            &pwszDestination);
    BAIL_ON_LSA_ERROR(dwError);

    /* Emperical testing from WinXP sp3 shows that only
       the username need be upper cased */

    dwError = LwWc16sToUpper(pwszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwAcctNameSize = RtlWC16StringNumChars(pwszAccountName) * sizeof(WCHAR);
    dwDestNameSize = RtlWC16StringNumChars(pwszDestination) * sizeof(WCHAR);

    dwBufferLen = dwAcctNameSize + dwDestNameSize;    
    dwError = LwAllocateMemory(dwBufferLen, (PVOID*)&pBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pBuffer, pwszAccountName, dwAcctNameSize);
    memcpy(pBuffer+dwAcctNameSize, pwszDestination, dwDestNameSize);

    HMAC(EVP_md5(),
         pObject->userInfo.pNtHash, 16,
         pBuffer, dwBufferLen,
         pNTLMv2Hash, &dwNTLMv2HashLen);    

    /* grab the client nonce (starts at offset 16 following
       the HMAC) */

    pClientNonce = pNTresp + 16;
    dwClientNonceLen = dwNTrespLen - 16;    

    /* generate the Response for comparing the MAC */

    dwError = LwAllocateMemory(dwClientNonceLen + 8,
                                (PVOID*)&pData);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pData, pChal, 8);
    memcpy(pData+8, pClientNonce, dwClientNonceLen);

    HMAC(EVP_md5(),
         pNTLMv2Hash, 16,
         pData, dwClientNonceLen + 8,
         pNTLMv2Resp, &dwNTLMv2RespLen);

    if (memcmp(pNTLMv2Resp, pNTresp, 16) != 0) {        
        dwError = LW_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Calculate the session Key */

    dwError = LsaDataBlobAllocate(&pSessKeyBlob, 16);
    BAIL_ON_LSA_ERROR(dwError);    

    pSessKeyBuf = LsaDataBlobBuffer(pSessKeyBlob);

    HMAC(EVP_md5(),
         pNTLMv2Hash, 16,
         pNTresp, 16,
         pSessKeyBuf, &dwSessKeyLen);

    *ppSessionKeyBlob = pSessKeyBlob;
    pSessKeyBlob = NULL;

    /* Done */

    dwError = LW_ERROR_SUCCESS;    
    

cleanup:

    LW_SAFE_FREE_MEMORY(pBuffer);
    LW_SAFE_FREE_MEMORY(pData);
    LW_SAFE_FREE_MEMORY(pwszAccountName);
    LW_SAFE_FREE_MEMORY(pwszDestination);

    if (pSessKeyBlob) {
        LsaDataBlobFree(&pSessKeyBlob);
    }    
    
    return dwError;

error:

    goto cleanup;    
}


/********************************************************
 *******************************************************/

static DWORD
SidSplitString(
    IN OUT PSTR pszSidString,
    OUT PDWORD pdwRid
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;    
    PSTR p = NULL;
    PSTR q = NULL;    
    DWORD dwRid = 0;    

    BAIL_ON_INVALID_POINTER(pszSidString);

    p = strrchr(pszSidString, '-');
    if (p == NULL) {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Get the RID */

    p++;
    dwRid = strtol(p, &q, 10);
    if ((dwRid == 0) || (*q != '\0')) {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    /* Split the string now that we know the RID is valid */

    *pdwRid = dwRid;

    p--;
    *p = '\0';

    dwError = LW_ERROR_SUCCESS;    

cleanup:

    return dwError;    

error:

    goto cleanup;    
}

/********************************************************
 *******************************************************/

static DWORD
FillAuthUserInfo(
    HANDLE hProvider,
    OUT PLSA_AUTH_USER_INFO pAuthInfo,
    IN PLSA_SECURITY_OBJECT pObject,
    IN PCSTR pszMachineName
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;
    DWORD dwNumGroups = 0;
    PSTR* ppszGroups = NULL;
    int i = 0;

    /* Find the user's groups */
    dwError = LocalQueryMemberOf(
        hProvider,
        0,
        1,
        &pObject->pszObjectSid,
        &dwNumGroups,
        &ppszGroups);
    BAIL_ON_LSA_ERROR(dwError);

    /* leave user flags empty for now.  But fill in account flags */

    pAuthInfo->dwAcctFlags = ACB_NORMAL;
    if (pObject->userInfo.bAccountDisabled) {
        pAuthInfo->dwAcctFlags |= ACB_DISABLED;
    }    
    if (pObject->userInfo.bAccountExpired) {
        pAuthInfo->dwAcctFlags |= ACB_PW_EXPIRED;        
    }
    if (pObject->userInfo.bPasswordNeverExpires) {
        pAuthInfo->dwAcctFlags |= ACB_PWNOEXP;
    }

    /* Copy strings */

    dwError = LwStrDupOrNull(pObject->userInfo.pszUnixName,
                              &pAuthInfo->pszAccount);    
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(pObject->userInfo.pszGecos,
                              &pAuthInfo->pszFullName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwStrDupOrNull(pszMachineName,
                              &pAuthInfo->pszDomain);    
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwStrDupOrNull(pObject->pszObjectSid,
                              &pAuthInfo->pszDomainSid);    
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SidSplitString(pAuthInfo->pszDomainSid,
                             &pAuthInfo->dwUserRid);
    BAIL_ON_LSA_ERROR(dwError);

    /* This really needs a check to ensure that the
       primaryGroup SID is in the machine's domain */

    dwError = SidSplitString(pObject->userInfo.pszPrimaryGroupSid,
                             &pAuthInfo->dwPrimaryGroupRid);
    BAIL_ON_LSA_ERROR(dwError);
    
    /* Since all the groups we get back have full SIDs,
       place them in the SidAttributeList rather that
       filtering the ones in the host's own SAM domain
       to the RidAttributeList */

    pAuthInfo->dwNumRids = 0;
    pAuthInfo->dwNumSids = dwNumGroups;

    dwError = LwAllocateMemory(sizeof(LSA_SID_ATTRIB)*dwNumGroups,
                                (PVOID*)&pAuthInfo->pSidAttribList);
    BAIL_ON_LSA_ERROR(dwError);

    for (i=0; i<pAuthInfo->dwNumSids; i++)
    {
        dwError = LwStrDupOrNull(ppszGroups[i],
                                  &pAuthInfo->pSidAttribList[i].pszSid);
        BAIL_ON_LSA_ERROR(dwError);

        pAuthInfo->pSidAttribList[i].dwAttrib = 
            LSA_SID_ATTR_GROUP_MANDATORY |
            LSA_SID_ATTR_GROUP_ENABLED_BY_DEFAULT |
            LSA_SID_ATTR_GROUP_ENABLED;        
    }
    
    dwError = LW_ERROR_SUCCESS;    

cleanup:

    LwFreeStringArray(ppszGroups, dwNumGroups);

    return dwError;

error:

    goto cleanup;    
}
