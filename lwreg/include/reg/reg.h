/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        reg.h
 *
 * Abstract:
 *
 *        Registry Data Definition
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __REG_H__
#define __REG_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/security-types.h>

typedef struct __REG_KEY_HANDLE *HKEY, **PHKEY;

typedef ACCESS_MASK REGSAM;


typedef DWORD REG_DATA_TYPE;
typedef DWORD *PREG_DATA_TYPE;

#define REG_NONE                           0  // No value type
#define REG_SZ                             1  // Unicode null terminated string
#define REG_EXPAND_SZ                      2  // hex(2): (Not supported)
#define REG_BINARY                         3  // hex:
#define REG_DWORD                          4  // dword
#define REG_DWORD_LITTLE_ENDIAN            4  // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN               5  // 32-bit number (Not supported)
#define REG_LINK                           6  // hex(7): (Not supported)
#define REG_MULTI_SZ                       7  // Multiple Unicode strings
#define REG_RESOURCE_LIST                  8  // hex(8): (Not supported)
#define REG_FULL_RESOURCE_DESCRIPTOR       9  // hex(9): (Not supported)
#define REG_RESOURCE_REQUIREMENTS_LIST     10 // hex(a): (Not supported)
#define REG_QWORD                          11 // hex(b): (Not supported)
#define REG_QWORD_LITTLE_ENDIAN            11 // hex(b):


#define REG_KEY                            21 // represent the reg entry is a Key
#define REG_KEY_DEFAULT                    22 // Default "@" entry
#define REG_PLAIN_TEXT                     23 // A string without "" around it
#define REG_SZA                            24 // Alias for REG_MULTI_SZ
#define REG_ATTRIBUTES                     25 // Registry schema type
#define REG_WSZ                            26 // String in PWSTR format
#define REG_WKEY                           27 // Key in PWSTR format


typedef DWORD REG_DATA_TYPE_FLAGS;

#define RRF_RT_REG_NONE       0x00000001
#define RRF_RT_REG_SZ         0x00000002 // Restrict type to REG_SZ.
#define RRF_RT_REG_EXPAND_SZ  0x00000004 // Restrict type to REG_EXPAND_SZ.
#define RRF_RT_REG_BINARY     0x00000008 // Restrict type to REG_BINARY.
#define RRF_RT_REG_DWORD      0x00000010 // Restrict type to REG_DWORD.
#define RRF_RT_REG_MULTI_SZ   0x00000020 // Restrict type to REG_MULTI_SZ.
#define RRF_RT_REG_QWORD      0x00000040 // Restrict type to REG_QWORD.
#define RRF_RT_DWORD          RRF_RT_REG_BINARY | RRF_RT_REG_DWORD
#define RRF_RT_QWORD          RRF_RT_REG_BINARY | RRF_RT_REG_QWORD
#define RRF_RT_ANY            0x0000FFFF // No type restriction.
#define RRF_NOEXPAND          0x10000000
#define RRF_ZEROONFAILURE     0x20000000


typedef DWORD REG_CREATE_KEY_DISPOSITION_FLAGS;

#define REG_CREATED_NEW_KEY     0x00000001L // The key did not exist and was created.
#define REG_OPENED_EXISTING_KEY 0x00000002L // The key existed and was simply opened without being changed.

#define HKEY_THIS_MACHINE "HKEY_THIS_MACHINE"

#define HKEY_THIS_MACHINE_W {'H','K','E','Y','_','T','H','I','S','_','M','A','C','H','I','N','E',0}

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 2048

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
}FILETIME, *PFILETIME;

typedef struct value_ent {
    PWSTR     ve_valuename;
    PDWORD    ve_valueptr;
    DWORD     ve_valuelen;
    DWORD     ve_type;
}VALENT, *PVALENT;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD  nLength;
    PVOID  pSecurityDescriptor;
    BOOL   bInheritHandle;
}SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;

//
// Value attribute
//
typedef enum _LWREG_VALUE_RANGE_TYPE
{
    LWREG_VALUE_RANGE_TYPE_NONE = 0,
    LWREG_VALUE_RANGE_TYPE_BOOLEAN,
    LWREG_VALUE_RANGE_TYPE_ENUM,
    LWREG_VALUE_RANGE_TYPE_INTEGER,
} LWREG_VALUE_RANGE_TYPE;

typedef enum _LWREG_VALUE_HINT
{
    LWREG_VALUE_HINT_NONE = 0,
    LWREG_VALUE_HINT_SECONDS,
    LWREG_VALUE_HINT_PATH,
    LWREG_VALUE_HINT_ACCOUNT
} LWREG_VALUE_HINT;

typedef struct _LWREG_RANGE_INTEGER
{
    DWORD Min;
    DWORD Max;
} LWREG_RANGE_INTEGER;

typedef struct _LWREG_VALUE_ATTRIBUTES_A
{
    REG_DATA_TYPE ValueType;
    PVOID pDefaultValue;
    DWORD DefaultValueLen;
    PSTR  pszDocString;
    LWREG_VALUE_RANGE_TYPE RangeType;
    LWREG_VALUE_HINT Hint;
    union _LWREG_RANGE_A{
        LWREG_RANGE_INTEGER RangeInteger;
        PSTR* ppszRangeEnumStrings;
    } Range;
} LWREG_VALUE_ATTRIBUTES_A, *PLWREG_VALUE_ATTRIBUTES_A;


typedef struct _LWREG_VALUE_ATTRIBUTES
{
    REG_DATA_TYPE ValueType;
    PVOID pDefaultValue;
    DWORD DefaultValueLen;
    PWSTR pwszDocString;
    LWREG_VALUE_RANGE_TYPE RangeType;
    LWREG_VALUE_HINT Hint;
    union _LWREG_RANGE{
        LWREG_RANGE_INTEGER RangeInteger;
        PWSTR* ppwszRangeEnumStrings;
    } Range;
} LWREG_VALUE_ATTRIBUTES, *PLWREG_VALUE_ATTRIBUTES;

typedef struct _LWREG_CURRENT_VALUEINFO{
    DWORD dwType;
    PVOID pvData;
    DWORD cbData;
}LWREG_CURRENT_VALUEINFO, *PLWREG_CURRENT_VALUEINFO;


void
RegFreeMultiStrsA(
    PSTR* ppszStrings
    );

void
RegFreeMultiStrsW(
    PWSTR* ppwszStrings
    );

VOID
RegFreeMemory(
    PVOID pMemory
    );

NTSTATUS
RegCopyValueBytes(
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

BOOLEAN
RegValidValueAttributes(
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes
    );


/*Range 40700 - 41200 is reserved for registry specific error*/
#define LWREG_ERROR_NO_SUCH_KEY_OR_VALUE                      40700
#define LWREG_ERROR_KEY_IS_ACTIVE                             40701
#define LWREG_ERROR_DUPLICATE_KEYVALUENAME                    40702
#define LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY                  40703
#define LWREG_ERROR_UNKNOWN_DATA_TYPE                         40704
#define LWREG_ERROR_BEYOUND_MAX_KEY_OR_VALUE_LENGTH           40705
#define LWREG_ERROR_NO_MORE_KEYS_OR_VALUES                    40706
#define LWREG_ERROR_INVALID_NAME                              40707
#define LWREG_ERROR_INVALID_CONTEXT                           40708
#define LWREG_ERROR_KEYNAME_EXIST                             40709
#define LWREG_ERROR_NO_SECURITY_ON_KEY                        40710
#define LWREG_ERROR_INVALID_SECURITY_DESCR                    40711
#define LWREG_ERROR_INVALID_ACCESS_TOKEN                      40712
#define LWREG_ERROR_DELETE_DEFAULT_VALUE_NOT_ALLOWED          40713
#define LWREG_ERROR_TOO_MANY_ENTRIES                          40714


#define LWREG_ERROR_INVALID_CACHE_PATH                        40720 //LWERROR VALUE 40001
#define LWREG_ERROR_INVALID_PREFIX_PATH                       40721 //40003
#define LWREG_ERROR_NOT_IMPLEMENTED                           40722 //40010
#define LWREG_ERROR_REGEX_COMPILE_FAILED                      40723 //40013
#define LWREG_ERROR_INVALID_LOG_LEVEL                         40724 //40112
#define LWREG_ERROR_NOT_HANDLED                               40725 //40017
#define LWREG_ERROR_UNEXPECTED_TOKEN                          40726 //40062
#define LWREG_ERROR_UNKNOWN                                   40727 //40188
#define LWREG_ERROR_SYNTAX                                    40728 
#define LWREG_ERROR_PARSE                                     40729

#endif /* __REG_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
