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
 *        regshare.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Server sub-system
 *
 *        Server share registry interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 *
 */
#include "includes.h"

static
NTSTATUS
NfsShareRegWriteToShareInfo(
    IN  REG_DATA_TYPE    regDataType,
    IN  PWSTR            pwszShareName,
    IN  PBYTE            pData,
    IN  ULONG            ulDataLen,
    IN  REG_DATA_TYPE    regSecDataType,
    IN  PBYTE            pSecData,
    IN  ULONG            ulSecDataLen,
    OUT PNFS_SHARE_INFO* ppShareInfo
    );

static
VOID
NfsShareFreeStringArray(
    PWSTR* ppwszValues,
    ULONG  ulNumValues
    );

NTSTATUS
NfsShareRegInit(
    VOID
    )
{
    return 0;
}

NTSTATUS
NfsShareRegOpen(
    OUT PHANDLE phRepository
    )
{
    return NtRegOpenServer(phRepository);
}

NTSTATUS
NfsShareRegFindByName(
    HANDLE           hRepository,
    PWSTR            pwszShareName,
    PNFS_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS        ntStatus       = STATUS_SUCCESS;
    HKEY            hRootKey       = NULL;
    HKEY            hKey           = NULL;
    HKEY            hSecKey        = NULL;
    REG_DATA_TYPE   dataType       = REG_UNKNOWN;
    ULONG           ulValueLen     = MAX_VALUE_LENGTH;
    REG_DATA_TYPE   dataSecType    = REG_UNKNOWN;
    ULONG           ulSecValueLen  = MAX_VALUE_LENGTH;
    PNFS_SHARE_INFO pShareInfo     = NULL;
    BYTE            pData[MAX_VALUE_LENGTH]    = {0};
    BYTE            pSecData[MAX_VALUE_LENGTH] = {0};
    wchar16_t       wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t       wszSharesKey[]   = REG_KEY_PATH_NFS_SHARES_W;
    wchar16_t       wszShareSecKey[] = REG_KEY_PATH_NFS_SHARES_SECURITY_W;

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_READ,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszShareSecKey[0],
                    0,
                    KEY_READ,
                    &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegGetValueW(
                    hRepository,
                    hKey,
                    NULL,
                    pwszShareName,
                    RRF_RT_REG_MULTI_SZ,
                    &dataType,
                    pData,
                    &ulValueLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegGetValueW(
                    hRepository,
                    hSecKey,
                    NULL,
                    pwszShareName,
                    RRF_RT_REG_BINARY,
                    &dataSecType,
                    pSecData,
                    &ulSecValueLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsShareRegWriteToShareInfo(
                    dataType,
                    pwszShareName,
                    pData,
                    ulValueLen,
                    dataSecType,
                    pSecData,
                    ulSecValueLen,
                    &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = pShareInfo;

cleanup:

    if (hRootKey)
    {
        NtRegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
    	NtRegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
    	NtRegCloseKey(hRepository, hSecKey);
    }

    return ntStatus;

error:

    if (pShareInfo)
    {
        NfsShareReleaseInfo(pShareInfo);
    }

    goto cleanup;
}

NTSTATUS
NfsShareRegAdd(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName,
    IN PWSTR  pwszPath,
    IN PWSTR  pwszComment,
    IN PBYTE  pSecDesc,
    IN ULONG  ulSecDescLen,
    IN PWSTR  pwszService
    )
{
    NTSTATUS ntStatus    = 0;
    HKEY     hRootKey    = NULL;
    HKEY     hKey        = NULL;
    HKEY     hSecKey     = NULL;
    PWSTR*   ppwszValues = NULL;
    PBYTE    pOutData    = NULL;
    SSIZE_T  cOutDataLen = 0;
    ULONG    ulCount     = 0;
    wchar16_t wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t wszSharesKey[]   = REG_KEY_PATH_NFS_SHARES_W;
    wchar16_t wszShareSecKey[] = REG_KEY_PATH_NFS_SHARES_SECURITY_W;
    wchar16_t wszPathPrefix[]    = REG_KEY_PATH_PREFIX_W;
    ULONG     ulPathPrefixLen =
                        (sizeof(wszPathPrefix)/sizeof(wchar16_t)) - 1;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if (IsNullOrEmptyString(pwszPath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_ALL_ACCESS,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    0,
                    KEY_ALL_ACCESS,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAllocateMemory(sizeof(PWSTR) * 4, (PVOID*)&ppwszValues);
    BAIL_ON_NT_STATUS(ntStatus);

    // Path
    ntStatus = NfsAllocateMemory(
                    sizeof(wszPathPrefix) + wc16slen(pwszPath) * sizeof(wchar16_t),
                    (PVOID*)&ppwszValues[ulCount]);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy( (PBYTE)&ppwszValues[ulCount][0],
            (PBYTE)&wszPathPrefix[0],
            sizeof(wszPathPrefix) - sizeof(wchar16_t));
    memcpy( (PBYTE)&ppwszValues[ulCount][ulPathPrefixLen],
            (PBYTE)pwszPath,
            wc16slen(pwszPath) * sizeof(wchar16_t));

    if (!IsNullOrEmptyString(pwszComment))
    {
        wchar16_t wszCommentPrefix[] = REG_KEY_COMMENT_PREFIX_W;
        ULONG     ulCommentPrefixLen =
                            (sizeof(wszCommentPrefix)/sizeof(wchar16_t)) - 1;

        ntStatus = NfsAllocateMemory(
                            sizeof(wszCommentPrefix) + wc16slen(pwszComment) * sizeof(wchar16_t),
                            (PVOID*)&ppwszValues[++ulCount]);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy( (PBYTE)&ppwszValues[ulCount][0],
                (PBYTE)&wszCommentPrefix[0],
                sizeof(wszCommentPrefix) - sizeof(wchar16_t));
        memcpy( (PBYTE)&ppwszValues[ulCount][ulCommentPrefixLen],
                (PBYTE)pwszComment,
                wc16slen(pwszComment) * sizeof(wchar16_t));
    }

    if (!IsNullOrEmptyString(pwszService))
    {
        wchar16_t wszServicePrefix[] = REG_KEY_SERVICE_PREFIX_W;
        ULONG     ulServicePrefixLen =
                            (sizeof(wszServicePrefix)/sizeof(wchar16_t)) - 1;

        ntStatus = NfsAllocateMemory(
                            sizeof(wszServicePrefix) + wc16slen(pwszService) * sizeof(wchar16_t),
                            (PVOID*)&ppwszValues[++ulCount]);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy( (PBYTE)&ppwszValues[ulCount][0],
                (PBYTE)&wszServicePrefix[0],
                sizeof(wszServicePrefix) - sizeof(wchar16_t));
        memcpy( (PBYTE)&ppwszValues[ulCount][ulServicePrefixLen],
                (PBYTE)pwszService,
                wc16slen(pwszService) * sizeof(wchar16_t));
    }

    ntStatus = NtRegMultiStrsToByteArrayW(ppwszValues, &pOutData, &cOutDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegSetValueExW(
                    hRepository,
                    hKey,
                    pwszShareName,
                    0,
                    REG_MULTI_SZ,
                    pOutData,
                    cOutDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pSecDesc && !ulSecDescLen) || (!pSecDesc && ulSecDescLen))
    {
        ntStatus = STATUS_INVALID_PARAMETER_5;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszShareSecKey[0],
                    0,
                    KEY_ALL_ACCESS,
                    &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegSetValueExW(
                    hRepository,
                    hSecKey,
                    pwszShareName,
                    0,
                    REG_BINARY,
                    pSecDesc,
                    ulSecDescLen);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hRootKey)
    {
    	NtRegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
    	NtRegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
    	NtRegCloseKey(hRepository, hSecKey);
    }
    if (ppwszValues)
    {
        NfsShareFreeStringArray(ppwszValues, 4);
    }
    if (pOutData)
    {
        RegFreeMemory(pOutData);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsShareRegBeginEnum(
    HANDLE  hRepository,
    ULONG   ulBatchLimit,
    PHANDLE phResume
    )
{
    NTSTATUS  ntStatus       = 0;
    HKEY      hRootKey       = NULL;
    HKEY      hKey           = NULL;
    wchar16_t wszHKTM[]      = HKEY_THIS_MACHINE_W;
    wchar16_t wszSharesKey[] = REG_KEY_PATH_NFS_SHARES_W;
    PNFS_SHARE_REG_ENUM_CONTEXT pEnumContext = NULL;

    if (!ulBatchLimit || (ulBatchLimit == UINT32_MAX))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NfsAllocateMemory(
                    sizeof(NFS_SHARE_REG_ENUM_CONTEXT),
                    (PVOID*)&pEnumContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_READ,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegQueryInfoKeyW(
                    hRepository,
                    hKey,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &pEnumContext->ulValuesAvailable,
                    &pEnumContext->ulMaxValueNameLen,
                    &pEnumContext->ulMaxValueLen,
                    NULL,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    pEnumContext->ulBatchLimit = ulBatchLimit;

    *phResume = (HANDLE)pEnumContext;

cleanup:

    if (hRootKey)
    {
    	NtRegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
    	NtRegCloseKey(hRepository, hKey);
    }

    return ntStatus;

error:

    *phResume = NULL;

    NFS_SAFE_FREE_MEMORY(pEnumContext);

    goto cleanup;
}

NTSTATUS
NfsShareRegEnum(
    HANDLE            hRepository,
    HANDLE            hResume,
    PNFS_SHARE_INFO** pppShareInfoList,
    PULONG            pulNumSharesFound
    )
{
    NTSTATUS ntStatus          = 0;
    ULONG    ulIndex           = 0;
    ULONG    ulBatchIndex      = 0;
    HKEY     hRootKey          = NULL;
    HKEY     hKey              = NULL;
    HKEY     hSecKey           = NULL;
    PWSTR    pwszValueName     = NULL;
    PBYTE    pData             = NULL;
    REG_DATA_TYPE    dataType  = REG_UNKNOWN;
    wchar16_t wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t wszSharesKey[]   = REG_KEY_PATH_NFS_SHARES_W;
    wchar16_t wszShareSecKey[] = REG_KEY_PATH_NFS_SHARES_SECURITY_W;
    PNFS_SHARE_INFO* ppShareInfoList  = NULL;
    REG_DATA_TYPE    dataSecType      = REG_UNKNOWN;
    ULONG            ulNumSharesFound = 0;
    BYTE             pSecData[MAX_VALUE_LENGTH] = {0};
    PNFS_SHARE_REG_ENUM_CONTEXT pResume = (PNFS_SHARE_REG_ENUM_CONTEXT)hResume;

    if (!pResume)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_READ,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszShareSecKey[0],
                    0,
                    KEY_READ,
                    &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsAllocateMemory(
                    pResume->ulBatchLimit * sizeof(*ppShareInfoList),
                    (PVOID) &ppShareInfoList);
    BAIL_ON_NT_STATUS(ntStatus);

    ulBatchIndex = pResume->ulBatchIndex;

    for (ulIndex = 0; ulIndex < pResume->ulBatchLimit; ulIndex++)
    {
        ULONG ulMaxValueNameLen = 0;
        ULONG ulMaxValueLen     = 0;
        ULONG ulMaxSecValueLen  = 0;

        ulMaxValueNameLen =
                    (pResume->ulMaxValueNameLen + 1) * sizeof(*pwszValueName);
        ulMaxValueLen    = pResume->ulMaxValueLen;
        ulMaxSecValueLen = MAX_VALUE_LENGTH;

        if (ulMaxValueNameLen)
        {
            NFS_SAFE_FREE_MEMORY_AND_RESET(pwszValueName);

            ntStatus = NfsAllocateMemory(
                            ulMaxValueNameLen,
                            (PVOID*) &pwszValueName);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (ulMaxValueLen)
        {
            NFS_SAFE_FREE_MEMORY_AND_RESET(pData);

            ntStatus = NfsAllocateMemory(ulMaxValueLen, (PVOID*) &pData);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NtRegEnumValueW(
                      hRepository,
                      hKey,
                      ulBatchIndex++,
                      pwszValueName,
                      &ulMaxValueNameLen,
                      NULL,
                      &dataType,
                      pData,
                      &ulMaxValueLen);
        if (ntStatus == STATUS_NO_MORE_MATCHES)
        {
            ntStatus = STATUS_SUCCESS;
            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (REG_MULTI_SZ != dataType)
        {
            continue;
        }

        ntStatus = NtRegGetValueW(
                        hRepository,
                        hSecKey,
                        NULL,
                        pwszValueName,
                        RRF_RT_REG_BINARY,
                        &dataSecType,
                        pSecData,
                        &ulMaxSecValueLen);
        if (STATUS_OBJECT_NAME_NOT_FOUND == ntStatus)
        {
            ntStatus = 0;
            ulMaxSecValueLen = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NfsShareRegWriteToShareInfo(
                      dataType,
                      pwszValueName,
                      pData,
                      ulMaxValueLen,
                      dataSecType,
                      pSecData,
                      ulMaxSecValueLen,
                      &ppShareInfoList[ulNumSharesFound++]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResume->ulBatchIndex = ulBatchIndex;

    *pppShareInfoList = ppShareInfoList;
    *pulNumSharesFound = ulNumSharesFound;

cleanup:

    if (hRootKey)
    {
    	NtRegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
    	NtRegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
    	NtRegCloseKey(hRepository, hSecKey);
    }

    NFS_SAFE_FREE_MEMORY(pwszValueName);
    NFS_SAFE_FREE_MEMORY(pData);

    return ntStatus;

error:

    *pppShareInfoList = NULL;
    *pulNumSharesFound = 0;

    if (ppShareInfoList)
    {
        NfsShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    goto cleanup;
}

NTSTATUS
NfsShareRegEndEnum(
    IN HANDLE hRepository,
    IN HANDLE hResume
    )
{
    PNFS_SHARE_REG_ENUM_CONTEXT pEnumContext =
                                    (PNFS_SHARE_REG_ENUM_CONTEXT)hResume;

    NFS_SAFE_FREE_MEMORY(pEnumContext);

    return STATUS_SUCCESS;
}

NTSTATUS
NfsShareRegDelete(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName
    )
{
    NTSTATUS  ntStatus = 0;
    HKEY      hRootKey = NULL;
    wchar16_t wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t wszSharesKey[]   = REG_KEY_PATH_NFS_SHARES_W;
    wchar16_t wszShareSecKey[] = REG_KEY_PATH_NFS_SHARES_SECURITY_W;

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_ALL_ACCESS,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegDeleteKeyValueW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegDeleteKeyValueW(
                    hRepository,
                    hRootKey,
                    &wszShareSecKey[0],
                    pwszShareName);
    if (STATUS_OBJECT_NAME_NOT_FOUND == ntStatus)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hRootKey)
    {
    	NtRegCloseKey(hRepository, hRootKey);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NfsShareRegGetCount(
    IN     HANDLE  hRepository,
    IN OUT PULONG  pulNumShares
    )
{
    NTSTATUS  ntStatus    = 0;
    ULONG     ulNumShares = 0;
    HKEY      hRootKey    = NULL;
    HKEY      hKey        = NULL;
    wchar16_t wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t wszSharesKey[]   = REG_KEY_PATH_NFS_SHARES_W;

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_READ,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRepository,
                    hRootKey,
                    &wszSharesKey[0],
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegQueryInfoKeyW(
                    hRepository,
                    hKey,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &ulNumShares,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulNumShares = ulNumShares;

cleanup:

    if (hRootKey)
    {
    	NtRegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
    	NtRegCloseKey(hRepository, hKey);
    }

    return ntStatus;

error:

    *pulNumShares = 0;

    goto cleanup;
}

VOID
NfsShareRegClose(
    IN HANDLE hRepository
    )
{
	NtRegCloseServer(hRepository);
}

VOID
NfsShareRegShutdown(
    VOID
    )
{
    //Do nothing
}

static
NTSTATUS
NfsShareRegWriteToShareInfo(
    IN  REG_DATA_TYPE    regDataType,
    IN  PWSTR            pwszShareName,
    IN  PBYTE            pData,
    IN  ULONG            ulDataLen,
    IN  REG_DATA_TYPE    regSecDataType,
    IN  PBYTE            pSecData,
    IN  ULONG            ulSecDataLen,
    OUT PNFS_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS        ntStatus     = STATUS_SUCCESS;
    PNFS_SHARE_INFO pShareInfo   = NULL;
    PWSTR*          ppwszValues  = NULL;

    ntStatus = NfsAllocateMemory(sizeof(*pShareInfo), (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->refcount = 1;

    pthread_rwlock_init(&pShareInfo->mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->mutex;

    ntStatus = NfsAllocateStringW(pwszShareName, &pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pData)
    {
        ULONG iValue = 0;
        wchar16_t wszCommentPrefix[] = REG_KEY_COMMENT_PREFIX_W;
        ULONG     ulCommentPrefixLen =
                            (sizeof(wszCommentPrefix)/sizeof(wchar16_t)) - 1;
        wchar16_t wszPathPrefix[]    = REG_KEY_PATH_PREFIX_W;
        ULONG     ulPathPrefixLen =
                            (sizeof(wszPathPrefix)/sizeof(wchar16_t)) - 1;
        wchar16_t wszServicePrefix[] = REG_KEY_SERVICE_PREFIX_W;
        ULONG     ulServicePrefixLen =
                            (sizeof(wszServicePrefix)/sizeof(wchar16_t)) - 1;

        ntStatus = NtRegByteArrayToMultiStrs(pData, ulDataLen, &ppwszValues);
        BAIL_ON_NT_STATUS(ntStatus);

        for (; ppwszValues[iValue]; iValue++)
        {
            PWSTR pwszValue = &ppwszValues[iValue][0];

            if (!wc16sncmp(&wszPathPrefix[0], pwszValue, ulPathPrefixLen))
            {
                NFS_SAFE_FREE_MEMORY(pShareInfo->pwszPath);

                ntStatus = NfsAllocateStringW(
                                &pwszValue[ulPathPrefixLen],
                                &pShareInfo->pwszPath);
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else if (!wc16sncmp(&wszCommentPrefix[0], pwszValue, ulCommentPrefixLen))
            {
                NFS_SAFE_FREE_MEMORY(pShareInfo->pwszComment);

                ntStatus = NfsAllocateStringW(
                                &pwszValue[ulCommentPrefixLen],
                                &pShareInfo->pwszComment);
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else if (!wc16sncmp(&wszServicePrefix[0], pwszValue, ulServicePrefixLen))
            {
                ntStatus = NfsShareMapServiceStringToIdW(
                                &pwszValue[ulServicePrefixLen],
                                &pShareInfo->service);
                BAIL_ON_NT_STATUS(ntStatus);

            }
            else
            {
                    ntStatus = STATUS_INVALID_PARAMETER_3;
                    BAIL_ON_NT_STATUS(ntStatus);
            }
        }
    }

    if (!pShareInfo->pwszComment)
    {
        wchar16_t wszComment[] = {0};

        ntStatus = NfsAllocateStringW(
                        &wszComment[0],
                        &pShareInfo->pwszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulSecDataLen)
    {
        ntStatus = NfsShareSetSecurity(
                       pShareInfo, 
                       (PSECURITY_DESCRIPTOR_RELATIVE)pSecData,
                       ulSecDataLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppShareInfo = pShareInfo;

cleanup:

    if (ppwszValues)
    {
        RegFreeMultiStrsW(ppwszValues);
    }

    return ntStatus;

error:

    if (pShareInfo)
    {
        NfsShareReleaseInfo(pShareInfo);
    }

    goto cleanup;
}

static
VOID
NfsShareFreeStringArray(
    PWSTR* ppwszValues,
    ULONG  ulNumValues
    )
{
    ULONG iValue = 0;

    for (; iValue < ulNumValues; iValue++)
    {
        if (ppwszValues[iValue])
        {
            NfsFreeMemory(ppwszValues[iValue]);
        }
    }

    NfsFreeMemory(ppwszValues);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
