/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        dirattr.c
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *
 *      Directory Attribute Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

static
VOID
DirectoryInitializeOutputValue(
    IN DIRECTORY_ATTR_TYPE AttrType,
    OUT PVOID pValue
    );

VOID
DirectoryFreeEntries(
    PDIRECTORY_ENTRY pEntries,
    DWORD            dwNumEntries
    )
{
    DWORD iEntry = 0;

    for (; iEntry < dwNumEntries; iEntry++)
    {
        PDIRECTORY_ENTRY pEntry = &pEntries[iEntry];

        if (pEntry->pAttributes)
        {
            DirectoryFreeAttributes(
                    pEntry->pAttributes,
                    pEntry->ulNumAttributes);
        }
    }

    DirectoryFreeMemory(pEntries);
}

VOID
DirectoryFreeAttributes(
    PDIRECTORY_ATTRIBUTE pAttributes,
    DWORD                dwNumAttributes
    )
{
    DWORD iDirAttr = 0;

    for (; iDirAttr < dwNumAttributes; iDirAttr++)
    {
        PDIRECTORY_ATTRIBUTE pDirAttr = NULL;

        pDirAttr = &pAttributes[iDirAttr];

        if (pDirAttr->pwszName)
        {
            DirectoryFreeStringW(pDirAttr->pwszName);
        }

        if (pDirAttr->pValues)
        {
            DirectoryFreeAttributeValues(
                    pDirAttr->pValues,
                    pDirAttr->ulNumValues);
        }
    }

    DirectoryFreeMemory(pAttributes);
}

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    )
{
    DWORD iValue = 0;

    for (; iValue < dwNumValues; iValue++)
    {
        PATTRIBUTE_VALUE pAttrValue = &pAttrValues[iValue];

        switch (pAttrValue->Type)
        {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:

                if (pAttrValue->data.pwszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->data.pwszStringValue);
                }

                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:

                if (pAttrValue->data.pszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->data.pszStringValue);
                }

                break;

            case DIRECTORY_ATTR_TYPE_OCTET_STREAM:
            case DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR:

                if (pAttrValue->data.pOctetString)
                {
                    DIRECTORY_FREE_MEMORY(pAttrValue->data.pOctetString->pBytes);

                    DirectoryFreeMemory(pAttrValue->data.pOctetString);
                }

                break;

            default:

                break;
        }
    }

    DirectoryFreeMemory(pAttrValues);
}


DWORD
DirectoryGetEntryAttributeSingle(
    PDIRECTORY_ENTRY pEntry,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;

    if (pEntry == NULL || ppAttribute == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pEntry->ulNumAttributes) {
        pAttribute = &(pEntry->pAttributes[0]);
    }

    *ppAttribute = pAttribute;

error:
    return dwError;
}


DWORD
DirectoryGetEntryAttributeByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttrName = NULL;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;
    PDIRECTORY_ATTRIBUTE pAttrFound = NULL;
    DWORD i = 0;

    if (pEntry == NULL || ppAttribute == NULL ||
        pwszAttributeName == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pwszAttrName = wc16sdup(pwszAttributeName);
    if (pwszAttrName == NULL) {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (i = 0; i < pEntry->ulNumAttributes; i++) {
        pAttribute = &(pEntry->pAttributes[i]);

        if (wc16scmp(pAttribute->pwszName,
                     pwszAttrName) == 0) {
            pAttrFound = pAttribute;
            break;
        }
    }

    *ppAttribute = pAttrFound;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszAttrName);

    return dwError;

error:
    goto cleanup;
}


DWORD
DirectoryGetEntryAttributeByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttributeName = NULL;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;

    dwError = LwMbsToWc16s(pszAttributeName, &pwszAttributeName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttributeByName(pEntry,
                                               pwszAttributeName,
                                               &pAttribute);

    *ppAttribute = pAttribute;

cleanup:
    if (pwszAttributeName) {
        LW_SAFE_FREE_MEMORY(pwszAttributeName);
    }

    return dwError;

error:
    *ppAttribute = NULL;
    goto cleanup;
}


DWORD
DirectoryGetAttributeValue(
    PDIRECTORY_ATTRIBUTE pAttribute,
    PATTRIBUTE_VALUE *ppAttrValue
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pValue = NULL;

    if (pAttribute == NULL || ppAttrValue == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAttribute->ulNumValues) {
        pValue = &(pAttribute->pValues[0]);
    }

    *ppAttrValue = pValue;

error:
    return dwError;
}


DWORD
DirectoryGetEntryAttrValueByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    BOOLEAN *pbValue = NULL;
    ULONG *pulValue = NULL;
    LONG64 *pllValue = NULL;
    PWSTR *ppwszValue = NULL;
    PSTR *ppszValue = NULL;
    POCTET_STRING *ppBlob = NULL;
    BOOLEAN bTypeIsCorrect = FALSE;
    BOOLEAN bUseResult = FALSE;

    dwError = DirectoryGetEntryAttributeByName(pEntry,
                                               pwszAttrName,
                                               &pAttr);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryGetAttributeValue(pAttr,
                                         &pAttrVal);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (!pAttrVal) {
        goto cleanup;
    }

    bTypeIsCorrect = (pAttrVal->Type == AttrType);

    switch (AttrType)
    {
    case DIRECTORY_ATTR_TYPE_BOOLEAN:
        pbValue = (BOOLEAN*)pValue;
        *pbValue = (bTypeIsCorrect) ? pAttrVal->data.bBooleanValue : FALSE;
        break;

    case DIRECTORY_ATTR_TYPE_INTEGER:
        pulValue = (ULONG*)pValue;
        *pulValue = (bTypeIsCorrect) ? pAttrVal->data.ulValue : 0;
        break;

    case DIRECTORY_ATTR_TYPE_LARGE_INTEGER:
        pllValue = (LONG64*)pValue;
        *pllValue = (bTypeIsCorrect) ? pAttrVal->data.llValue : 0;
        break;

    case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
        ppwszValue = (PWSTR*)pValue;
        *ppwszValue = (bTypeIsCorrect) ? pAttrVal->data.pwszStringValue : NULL;
        break;

    case DIRECTORY_ATTR_TYPE_ANSI_STRING:
        ppszValue = (PSTR*)pValue;
        *ppszValue = (bTypeIsCorrect) ? pAttrVal->data.pszStringValue : NULL;
        break;

    case DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR:
        ppBlob = (POCTET_STRING*)pValue;
        *ppBlob = (bTypeIsCorrect) ? pAttrVal->data.pOctetString : NULL;
        break;

    case DIRECTORY_ATTR_TYPE_OCTET_STREAM:
        ppBlob = (POCTET_STRING*)pValue;
        *ppBlob = (bTypeIsCorrect) ? pAttrVal->data.pOctetString : NULL;
        break;

    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    bUseResult = TRUE;

cleanup:
    if (!bUseResult)
    {
        DirectoryInitializeOutputValue(AttrType, pValue);
    }

    return dwError;

error:
    // Make sure to not use result
    bUseResult = FALSE;
    goto cleanup;
}


DWORD
DirectoryGetEntryAttrValueByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttrName = NULL;

    dwError = LwMbsToWc16s(pszAttrName, &pwszAttrName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               pwszAttrName,
                                               AttrType,
                                               pValue);

cleanup:
    if (pwszAttrName) {
        LW_SAFE_FREE_MEMORY(pwszAttrName);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
DirectoryGetEntrySecurityDescriptor(
    PDIRECTORY_ENTRY               pEntry,
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    POCTET_STRING pSecDescBlob = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    DWORD dwSecDescSize = 32;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    DWORD dwDaclSize = 512;
    PACL pDacl = NULL;
    DWORD dwSaclSize = 0;
    PACL pSacl = NULL;
    DWORD dwOwnerSize = 34;
    PSID pOwnerSid = NULL;
    DWORD dwPrimaryGroupSize = 34;
    PSID pPrimaryGroupSid = NULL;
    DWORD i = 0;

    if (pEntry == NULL ||
        ppSecDesc == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    /*
     * Look for security descriptor value
     */
    for (i = 0; i < pEntry->ulNumAttributes; i++)
    {
        pAttr = &(pEntry->pAttributes[i]);

        /* Security descriptor is a single-value attribute */
        if (pAttr->ulNumValues &&
            pAttr->pValues[0].Type == DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR)
        {
            pSecDescBlob = pAttr->pValues[0].data.pOctetString;
        }
    }

    /*
     * Make sure there actually is anything that could
     * be a security descriptor
     */
    if (pSecDescBlob == NULL ||
        pSecDescBlob->pBytes == NULL ||
        pSecDescBlob->ulNumBytes == 0)
    {
        dwError = ERROR_NO_SECURITY_ON_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(pSecDescBlob->ulNumBytes,
                               OUT_PPVOID(&pSecDescRel));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pSecDescRel, pSecDescBlob->pBytes, pSecDescBlob->ulNumBytes);

    do
    {
        if (dwSecDescSize)
        {
            dwError = LwReallocMemory(pSecDesc,
                                      OUT_PPVOID(&pSecDesc),
                                      dwSecDescSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwDaclSize)
        {
            dwError = LwReallocMemory(pDacl,
                                      OUT_PPVOID(&pDacl),
                                      dwDaclSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwSaclSize)
        {
            dwError = LwReallocMemory(pSacl,
                                      OUT_PPVOID(&pSacl),
                                      dwSaclSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwOwnerSize)
        {
            dwError = LwReallocMemory(pOwnerSid,
                                      OUT_PPVOID(&pOwnerSid),
                                      dwOwnerSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwPrimaryGroupSize)
        {
            dwError = LwReallocMemory(pPrimaryGroupSid,
                                      OUT_PPVOID(&pPrimaryGroupSid),
                                      dwPrimaryGroupSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        ntStatus = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                               pSecDesc,
                                               &dwSecDescSize,
                                               pDacl,
                                               &dwDaclSize,
                                               pSacl,
                                               &dwSaclSize,
                                               pOwnerSid,
                                               &dwOwnerSize,
                                               pPrimaryGroupSid,
                                               &dwPrimaryGroupSize);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_BUFFER_TOO_SMALL)
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }
    while (ntStatus == STATUS_BUFFER_TOO_SMALL &&
           dwSecDescSize <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDesc = pSecDesc;

cleanup:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDesc);

    *ppSecDesc = NULL;

    goto cleanup;
}


DWORD
DirectorySetEntrySecurityDescriptor(
    HANDLE                         hDirectory,
    PCWSTR                         pwszDn,
    PSECURITY_DESCRIPTOR_ABSOLUTE  pSecDesc
    )
{
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszAttrDn[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSecurityDescriptor[] = DIRECTORY_ATTR_SECURITY_DESCRIPTOR;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    size_t sDnLen = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszObjectDn = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    OCTET_STRING SecDescBlob = {0};
    DWORD iMod = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        NULL
    };

    enum AttrValueIndex {
        ATTR_VAL_IDX_SECURITY_DESCRIPTOR = 0,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_SECURITY_DESCRIPTOR */
            .Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
            .data.pOctetString = NULL
        }
    };

    DIRECTORY_MOD ModSecDesc = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSecurityDescriptor,
        1,
        &AttrValues[ATTR_VAL_IDX_SECURITY_DESCRIPTOR]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    if (pwszDn == NULL ||
        pSecDesc == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszDn, &sDnLen);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrDn) / sizeof(WCHAR)) - 1) +
                  sDnLen +
                  (sizeof(wszFilterFmt) / sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(dwFilterLen * sizeof(pwszFilter[0]),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrDn,
                    pwszDn) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = ERROR_DS_NO_SUCH_OBJECT;
    }
    else if (dwNumEntries > 1)
    {
        dwError = ERROR_INTERNAL_DB_ERROR;
    }
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrDn,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              &pwszObjectDn);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    SecDescBlob.pBytes     = (PBYTE)pSecDescRel;
    SecDescBlob.ulNumBytes = ulSecDescLen;

    AttrValues[ATTR_VAL_IDX_SECURITY_DESCRIPTOR].data.pOctetString =
        &SecDescBlob;
    Mods[iMod++] = ModSecDesc;

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszObjectDn,
                                    Mods);
    BAIL_ON_DIRECTORY_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwNumEntries);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


VOID
DirectoryFreeEntrySecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSID pOwnerSid = NULL;
    BOOLEAN bOwnerDefaulted = FALSE;
    PSID pPrimaryGroupSid = NULL;
    BOOLEAN bPrimaryGroupDefaulted = FALSE;
    PACL pDacl = NULL;
    BOOLEAN bDaclPresent = FALSE;
    BOOLEAN bDaclDefaulted = FALSE;
    PACL pSacl = NULL;
    BOOLEAN bSaclPresent = FALSE;
    BOOLEAN bSaclDefaulted = FALSE;

    if (ppSecDesc == NULL ||
        *ppSecDesc == NULL)
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntStatus = RtlGetOwnerSecurityDescriptor(pSecDesc,
                                             &pOwnerSid,
                                             &bOwnerDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetGroupSecurityDescriptor(pSecDesc,
                                             &pPrimaryGroupSid,
                                             &bPrimaryGroupDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc,
                                            &bDaclPresent,
                                            &pDacl,
                                            &bDaclDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetSaclSecurityDescriptor(pSecDesc,
                                            &bSaclPresent,
                                            &pSacl,
                                            &bSaclDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    LW_SAFE_FREE_MEMORY(pOwnerSid);
    LW_SAFE_FREE_MEMORY(pPrimaryGroupSid);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pSacl);

    LW_SAFE_FREE_MEMORY(pSecDesc);
    *ppSecDesc = NULL;

cleanup:
    return;

error:
    goto cleanup;
}


static
VOID
DirectoryInitializeOutputValue(
    IN DIRECTORY_ATTR_TYPE AttrType,
    OUT PVOID pValue
    )
{
    size_t size = 0;

    switch (AttrType)
    {
    case DIRECTORY_ATTR_TYPE_BOOLEAN:
        size = sizeof(BOOLEAN);
        break;

    case DIRECTORY_ATTR_TYPE_INTEGER:
        size = sizeof(ULONG);
        break;

    case DIRECTORY_ATTR_TYPE_LARGE_INTEGER:
        size = sizeof(LONG64);
        break;

    case DIRECTORY_ATTR_TYPE_OCTET_STREAM:
        size = sizeof(POCTET_STRING);
        break;

    case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
        size = sizeof(PWSTR);
        break;

    case DIRECTORY_ATTR_TYPE_ANSI_STRING:
        size = sizeof(PSTR);
        break;

    default:
        // nothing can be done
        break;
    }

    if (pValue)
    {
        memset(pValue, 0, size);
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
