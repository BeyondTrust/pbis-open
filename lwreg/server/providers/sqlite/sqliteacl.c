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
 *        sqliteacl.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

NTSTATUS
SqliteSetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescRel
    )
{
	NTSTATUS status = STATUS_ACCESS_DENIED;
	PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
	BOOLEAN bInLock = FALSE;
	PSECURITY_DESCRIPTOR_RELATIVE pCurrSecDescRel = NULL;
	ULONG ulCurrSecDescLen = 0;
	PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
	ULONG ulNewSecDescLen = 0;
	// Do not free
	PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet = NULL;
	ULONG ulSecDescToSetLen = 0;
	PREG_KEY_CONTEXT pKeyCtx = NULL;
	ACCESS_MASK accessRequired = KEY_ALL_ACCESS;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
    	accessRequired |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
    	accessRequired |= WRITE_DAC;
    }

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, accessRequired);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

	/* Sanity checks */
	if (SecurityInformation == 0)
	{
		status = STATUS_INVALID_PARAMETER;
		BAIL_ON_NT_STATUS(status);
	}

	if (!RtlValidRelativeSecurityDescriptor(pSecDescRel, ulSecDescRel, SecurityInformation))
	{
		status = STATUS_INVALID_SECURITY_DESCR;
		BAIL_ON_NT_STATUS(status);
	}

	LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyCtx->mutex);

	// If this key has no SD
    if (pKeyCtx->qwSdId != -1)
    {
    	// Key's SD is not cached
    	if (!pKeyCtx->bHasSdInfo)
    	{
    		status = RegDbGetKeyAclByKeyId(ghCacheConnection,
    				                       pKeyCtx->qwId,
    				                       &pKeyCtx->qwSdId,
    				                       &pCurrSecDescRel,
    				                       &ulCurrSecDescLen);
    		BAIL_ON_NT_STATUS(status);
    	}
    	else
    	{
    	    status = LW_RTL_ALLOCATE((PVOID*)&pCurrSecDescRel, VOID, pKeyCtx->ulSecDescLength);
    	    BAIL_ON_NT_STATUS(status);

        	status = SqliteGetKeySecurityDescriptor_inlock(pKeyCtx,
       							            pCurrSecDescRel,
    							            pKeyCtx->ulSecDescLength);
    	    BAIL_ON_NT_STATUS(status);

    	    ulCurrSecDescLen = pKeyCtx->ulSecDescLength;
    	}

    	ulNewSecDescLen = ulCurrSecDescLen + ulSecDescRel;
    	status = LW_RTL_ALLOCATE((PVOID*)&pNewSecDescRel, VOID, ulNewSecDescLen);
        BAIL_ON_NT_STATUS(status);

    	status = RtlSetSecurityDescriptorInfo(
    				  SecurityInformation,
    				  pSecDescRel,
    				  pCurrSecDescRel,
    				  pNewSecDescRel,
    				  &ulNewSecDescLen,
    				  &gRegKeyGenericMapping);
    	BAIL_ON_NT_STATUS(status);

    	pSecDescRelToSet = pNewSecDescRel;
    	ulSecDescToSetLen = ulNewSecDescLen;
    }
    else
    {
    	pSecDescRelToSet = pSecDescRel;
    	ulSecDescToSetLen = ulSecDescRel;
    }

    status = RegDbUpdateKeyAcl(ghCacheConnection,
    		                   (PCWSTR)pKeyCtx->pwszKeyName,
							   pKeyCtx->qwId,
							   pKeyCtx->qwSdId,
							   pSecDescRelToSet,
							   ulSecDescToSetLen);
	BAIL_ON_NT_STATUS(status);

	status = SqliteSetKeySecurityDescriptor_inlock(pKeyCtx, pSecDescRelToSet, ulSecDescToSetLen);
	BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    LWREG_SAFE_FREE_MEMORY(pCurrSecDescRel);
    LWREG_SAFE_FREE_MEMORY(pNewSecDescRel);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteGetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN OUT PULONG pulSecDescRelLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    BOOLEAN bInLock = FALSE;
    PBYTE pTmpSecDescRel = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, KEY_READ | READ_CONTROL);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    if (!pulSecDescRelLen || *pulSecDescRelLen == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (!pSecDescRel)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pTmpSecDescRel, VOID, *pulSecDescRelLen);
        BAIL_ON_NT_STATUS(status);
    }

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyCtx->mutex);

    // this key currently has no SD assigned
    if (pKeyCtx->qwSdId == -1)
    {
    	status = STATUS_NO_SECURITY_ON_OBJECT;
    	BAIL_ON_NT_STATUS(status);
    }

    status = SqliteCacheKeySecurityDescriptor_inlock(pKeyCtx);
    BAIL_ON_NT_STATUS(status);

	status = RtlQuerySecurityDescriptorInfo(
				  SecurityInformation,
				  pSecDescRel ? pSecDescRel : (PSECURITY_DESCRIPTOR_RELATIVE)pTmpSecDescRel,
				  pulSecDescRelLen,
				  pKeyCtx->pSecurityDescriptor);
	BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyCtx->mutex);

    LWREG_SAFE_FREE_MEMORY(pTmpSecDescRel);

    return status;

error:
    goto cleanup;
}
