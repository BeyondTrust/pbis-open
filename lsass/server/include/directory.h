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
 *        directory.h
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *
 *      Directory Interface API
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

typedef ULONG DIRECTORY_ATTR_TYPE;

#define DIRECTORY_ATTR_TYPE_BOOLEAN                 1
#define DIRECTORY_ATTR_TYPE_INTEGER                 2
#define DIRECTORY_ATTR_TYPE_LARGE_INTEGER           3
#define DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR  4
#define DIRECTORY_ATTR_TYPE_OCTET_STREAM            5
#define DIRECTORY_ATTR_TYPE_UNICODE_STRING          6
#define DIRECTORY_ATTR_TYPE_ANSI_STRING             7

typedef struct _OCTET_STRING {
    ULONG ulNumBytes;
    PBYTE pBytes;
} OCTET_STRING, *POCTET_STRING;

typedef struct _ATTRIBUTE_VALUE
{
    DIRECTORY_ATTR_TYPE Type;

    union _ATTRIBUTE_VALUE_DATA
    {
        ULONG  ulValue;
        LONG64 llValue;
        PWSTR  pwszStringValue;
        PSTR   pszStringValue;
        BOOL   bBooleanValue;
        POCTET_STRING pOctetString;
    } data;

} ATTRIBUTE_VALUE, *PATTRIBUTE_VALUE;

typedef struct _DIRECTORY_ATTRIBUTE
{
    PWSTR            pwszName;

    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pValues;

} DIRECTORY_ATTRIBUTE, *PDIRECTORY_ATTRIBUTE;

typedef struct _DIRECTORY_ENTRY
{
    ULONG                ulNumAttributes;
    PDIRECTORY_ATTRIBUTE pAttributes;

} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef ULONG DIR_MOD_FLAGS;

#define DIR_MOD_FLAGS_ADD     0x0
#define DIR_MOD_FLAGS_REPLACE 0x1
#define DIR_MOD_FLAGS_DELETE  0x2

typedef struct _DIRECTORY_MOD
{
    DIR_MOD_FLAGS    ulOperationFlags;
    PWSTR            pwszAttrName;
    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

} DIRECTORY_MOD, *PDIRECTORY_MOD;

#define ENTRY(i, ppDirectoryEntry) = *(ppDirectoryEntry + i)
#define ATTRIBUTE(i, ppDirectoryAttributes) = *(ppDirectoryAttributes + i)

#define VALUE(i, pAttribute) = *(pAttribute->ppAttributeValues + i)
#define NUM_VALUES(pAttribute) = pAttribute->ulNumValues
#define INTEGER(pValue) =  pValue->ulValue;
#define PRINTABLE_STRING(pValue) = pValue->pPrintableString;
#define IA5_STRING(pValue) = pValue->pIA5String;
#define OCTET_STRING_LENGTH(pValue) = pValue->pOctetString->ulNumBytes;
#define OCTET_STRING_DATA(pValue) = pValue->pOctetString->pBytes;
#define NT_SECURITY_DESCRIPTOR_LENGTH(pValue) = pValue->pNTSecurityDescriptor->ulNumBytes;
#define NT_SECURITY_DESCRIPTOR_DATA(pValue) = pValue->pNTSecurityDescriptor->pBytes;

#define DIR_OBJECT_CLASS_UNKNOWN         (0)
#define DIR_OBJECT_CLASS_DOMAIN          (1)
#define DIR_OBJECT_CLASS_BUILTIN_DOMAIN  (2)
#define DIR_OBJECT_CLASS_CONTAINER       (3)
#define DIR_OBJECT_CLASS_LOCAL_GROUP     (4)
#define DIR_OBJECT_CLASS_USER            (5)
#define DIR_OBJECT_CLASS_LOCALGRP_MEMBER (6)
#define DIR_OBJECT_CLASS_DOMAIN_GROUP    (7)
#define DIR_OBJECT_CLASS_SENTINEL        (8)


#define DIRECTORY_ATTR_RECORD_ID \
    {'O','b','j','e','c','t','R','e','c','o','r','d','I','d',0}
#define DIRECTORY_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define DIRECTORY_ATTR_SECURITY_DESCRIPTOR \
    {'S','e','c','u','r','i','t','y','D','e','s','c','r','i','p','t','o','r',0}
#define DIRECTORY_ATTR_DISTINGUISHED_NAME  \
    {'D','i','s','t','i','n','g','u','i','s','h','e','d','N','a','m','e',0}
#define DIRECTORY_ATTR_PARENT_DN \
    {'P','a','r','e','n','t','D','N',0}
#define DIRECTORY_ATTR_OBJECT_CLASS \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define DIRECTORY_ATTR_DOMAIN_NAME \
    {'D','o','m','a','i','n',0}
#define DIRECTORY_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define DIRECTORY_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define DIRECTORY_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define DIRECTORY_ATTR_USER_PRINCIPAL_NAME \
    {'U','s','e','r','P','r','i','n','c','i','p','a','l','N','a','m','e',0}
#define DIRECTORY_ATTR_DESCRIPTION \
    {'D','e','s','c','r','i','p','t','i','o','n',0}
#define DIRECTORY_ATTR_COMMENT \
    {'C','o','m','m','e','n','t',0}
#define DIRECTORY_ATTR_UID \
    {'U','I','D',0}
#define DIRECTORY_ATTR_PASSWORD \
    {'P','a','s','s','w','o','r','d',0}
#define DIRECTORY_ATTR_ACCOUNT_FLAGS \
    {'A','c','c','o','u','n','t','F','l','a','g','s',0}
#define DIRECTORY_ATTR_GECOS \
    {'G','e','c','o','s',0}
#define DIRECTORY_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define DIRECTORY_ATTR_HOME_DRIVE \
    {'H','o','m','e','d','r','i','v','e',0}
#define DIRECTORY_ATTR_LOGON_SCRIPT \
    {'L','o','g','o','n','S','c','r','i','p','t',0}
#define DIRECTORY_ATTR_PROFILE_PATH \
    {'P','r','o','f','i','l','e','P','a','t','h',0}
#define DIRECTORY_ATTR_WORKSTATIONS \
    {'W','o','r','k','s','t','a','t','i','o','n','s',0}
#define DIRECTORY_ATTR_PARAMETERS \
    {'P','a','r','a','m','e','t','e','r','s',0}
#define DIRECTORY_ATTR_SHELL \
    {'L','o','g','i','n','S','h','e','l','l',0}
#define DIRECTORY_ATTR_PASSWORD_LAST_SET \
    {'P','a','s','s','w','o','r','d','L','a','s','t','S','e','t',0}
#define DIRECTORY_ATTR_ALLOW_PASSWORD_CHANGE \
    {'A','l','l','o','w','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DIRECTORY_ATTR_FORCE_PASSWORD_CHANGE \
    {'F','o','r','c','e','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DIRECTORY_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define DIRECTORY_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define DIRECTORY_ATTR_LM_HASH \
    {'L','M','H','a','s','h',0}
#define DIRECTORY_ATTR_NT_HASH \
    {'N','T','H','a','s','h',0}
#define DIRECTORY_ATTR_PRIMARY_GROUP \
    {'P','r','i','m','a','r','y','G','r','o','u','p',0}
#define DIRECTORY_ATTR_GID \
    {'G','I','D',0}
#define DIRECTORY_ATTR_COUNTRY_CODE \
    {'C','o','u','n','t','r','y','C','o','d','e',0}
#define DIRECTORY_ATTR_CODE_PAGE \
    {'C','o','d','e','P','a','g','e',0}
#define DIRECTORY_ATTR_MAX_PWD_AGE \
    {'M','a','x','P','w','d','A','g','e',0}
#define DIRECTORY_ATTR_MIN_PWD_AGE \
    {'M','i','n','P','w','d','A','g','e',0}
#define DIRECTORY_ATTR_PWD_PROMPT_TIME \
    {'P','w','d','P','r','o','m','p','t','T','i','m','e',0}
#define DIRECTORY_ATTR_LAST_LOGON \
    {'L','a','s','t','L','o','g','o','n',0}
#define DIRECTORY_ATTR_LAST_LOGOFF \
    {'L','a','s','t','L','o','g','o','f','f',0}
#define DIRECTORY_ATTR_LOCKOUT_TIME \
    {'L','o','c','k','o','u','t','T','i','m','e',0}
#define DIRECTORY_ATTR_LOGON_COUNT \
    {'L','o','g','o','n','C','o','u','n','t',0}
#define DIRECTORY_ATTR_BAD_PASSWORD_COUNT \
    {'B','a','d','P','w','d','C','o','u','n','t',0}
#define DIRECTORY_ATTR_LOGON_HOURS \
    {'L','o','g','o','n','H','o','u','r','s',0}
#define DIRECTORY_ATTR_ROLE \
    {'R','o','l','e',0}
#define DIRECTORY_ATTR_MIN_PWD_LENGTH \
    {'M','i','n','P','w','d','L','e','n','g','t','h',0}
#define DIRECTORY_ATTR_PWD_HISTORY_LENGTH \
    {'P','w','d','H','i','s','t','o','r','y','L','e','n','g','t','h',0}
#define DIRECTORY_ATTR_PWD_PROPERTIES \
    {'P','w','d','P','r','o','p','e','r','t','i','e','s',0}
#define DIRECTORY_ATTR_FORCE_LOGOFF_TIME \
    {'F','o','r','c','e','L','o','g','o','f','f','T','i','m','e',0}
#define DIRECTORY_ATTR_PRIMARY_DOMAIN \
    {'P','r','i','m','a','r','y','D','o','m','a','i','n',0}
#define DIRECTORY_ATTR_SEQUENCE_NUMBER \
    {'S','e','q','u','e','n','c','e','N','u','m','b','e','r',0}
#define DIRECTORY_ATTR_LOCKOUT_DURATION \
    {'L','o','c','k','o','u','t','D','u','r','a','t','i','o','n',0}
#define DIRECTORY_ATTR_LOCKOUT_WINDOW \
    {'L','o','c','k','o','u','t','W','i','n','d','o','w',0}
#define DIRECTORY_ATTR_LOCKOUT_THRESHOLD \
    {'L','o','c','k','o','u','t','T','h','r','e','s','h','o','l','d',0}
#define DIRECTORY_ATTR_CREATED_TIME \
    {'C','r','e','a','t','e','d','T','i','m','e',0}
#define DIRECTORY_ATTR_MEMBERS \
    {'M','e','m','b','e','r','s',0}


#define DIRECTORY_FREE_STRING(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
    }

#define DIRECTORY_FREE_STRING_AND_RESET(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
        (pszStr) = NULL; \
    }

#define DIRECTORY_FREE_MEMORY(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
    }

#define DIRECTORY_FREE_MEMORY_AND_RESET(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
        (pMem) = NULL; \
    }

DWORD
DirectoryOpen(
    PHANDLE phDirectory
    );

DWORD
DirectoryBind(
    HANDLE hDirectory,
    PWSTR  pwszDistinguishedName,
    PWSTR  pwszCredentials,
    ULONG  ulMethod
    );

DWORD
DirectoryAddObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD attributes[]
    );

DWORD
DirectoryModifyObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
DirectorySearch(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    );

DWORD
DirectorySetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryGetGroupMembers(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryGetMemberships(
    HANDLE            hDirectory,
    PWSTR             pwszUserDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryAddToGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    );

DWORD
DirectoryRemoveFromGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    );

DWORD
DirectoryGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    );

DWORD
DirectoryGetGroupCount(
    HANDLE hBindHandle,
    PDWORD pdwNumGroups
    );

DWORD
DirectoryChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

DWORD
DirectoryGetPasswordChangeInterval(
    HANDLE hBindHandle,
    PDWORD pdwInterval
    );

VOID
DirectoryClose(
    HANDLE hDirectory
    );

VOID
DirectoryFreeEntries(
    PDIRECTORY_ENTRY pEntries,
    DWORD            dwNumEntries
    );

VOID
DirectoryFreeAttributes(
    PDIRECTORY_ATTRIBUTE pAttributes,
    DWORD                dwNumAttributes
    );

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    );

DWORD
DirectoryGetEntryAttributeSingle(
    PDIRECTORY_ENTRY pEntry,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetAttributeValue(
    PDIRECTORY_ATTRIBUTE pAttribute,
    PATTRIBUTE_VALUE *ppAttrValue
    );

DWORD
DirectoryGetEntryAttrValueByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryGetEntryAttrValueByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryGetEntrySecurityDescriptor(
    PDIRECTORY_ENTRY               pEntry,
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

DWORD
DirectorySetEntrySecurityDescriptor(
    HANDLE                         hDirectory,
    PCWSTR                         pwszDn,
    PSECURITY_DESCRIPTOR_ABSOLUTE  pSecDesc
    );

DWORD
DirectoryAllocateMemory(
    size_t sSize,
    PVOID* ppMemory
    );

DWORD
DirectoryReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    size_t sSize
    );

VOID
DirectoryFreeMemory(
    PVOID pMemory
    );

DWORD
DirectoryAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    );

DWORD
DirectoryAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

DWORD
DirectoryAllocateWC16StringFilterPrintf(
    OUT PWSTR* pOutput,
    IN PCSTR Format,
    ...
    );

VOID
DirectoryFreeStringW(
    PWSTR pwszString
    );

VOID
DirectoryFreeString(
    PSTR pszString
    );

VOID
DirectoryFreeStringArray(
    PWSTR* ppStringArray,
    DWORD  dwCount
    );

VOID
DirectoryFreeEntrySecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );


#endif /* __DIRECTORY_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
