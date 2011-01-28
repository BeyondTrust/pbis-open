/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Registry Interprocess Communication
 *
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ipc.h"

#define LWMSG_MEMBER_PBYTE(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT8(BYTE),                              \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ENCODING("hex+ascii")

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(WCHAR),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING("utf-16")

#define LWMSG_PWSTR \
    LWMSG_POINTER_BEGIN, \
    LWMSG_UINT16(WCHAR), \
    LWMSG_POINTER_END, \
    LWMSG_ATTR_ZERO_TERMINATED, \
    LWMSG_ATTR_ENCODING("utf-16")

/******************************************************************************/

// This one is odd because Windows has decided that a security descriptor
// (that's not a typo... descriptor) changes too much, so the nLength member
// actually is the size of SECURITY_ATTRIBUTE... so this puts us in a bit of
// a bind not knowing how big the pSecurityDescriptor member really is.  What
// saves us (a little) is that NULL is usually passed in that member, so we may
// have to add that as a limitation.
//
// In the mean time, we won't use the nLength as a LWMSG_ATTR_LENGTH_MEMBER
static LWMsgTypeSpec gRegSecAttrSpec[] =
{
    // DWORD  nLength;
    // PVOID  pSecurityDescriptor;
    // BOOL   bInheritHandle;

    LWMSG_STRUCT_BEGIN(SECURITY_ATTRIBUTES),

    LWMSG_MEMBER_UINT32(SECURITY_ATTRIBUTES, nLength),

    LWMSG_MEMBER_POINTER_BEGIN(SECURITY_ATTRIBUTES, pSecurityDescriptor),
    LWMSG_INT64(VOID),
    LWMSG_POINTER_END,
    //LWMSG_ATTR_LENGTH_MEMBER(SECURITY_ATTRIBUTES, nLength), see above comment

    LWMSG_MEMBER_UINT32(SECURITY_ATTRIBUTES, bInheritHandle),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegIPCErrorSpec[] =
{
    //NTSTATUS status;

    LWMSG_STRUCT_BEGIN(REG_IPC_STATUS),
    LWMSG_MEMBER_UINT32(REG_IPC_STATUS, status),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegCreateKeyExSpec[] =
{
    // IN HKEY hKey,
    // IN PCWSTR pSubKey,
    // IN DWORD Reserved,
    // IN OPTIONAL PWSTR pClass,
    // IN DWORD dwOptions,
    // IN ACCESS_MASK AccessDesired,
    // IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    // ULONG ulSecDescLen;

    LWMSG_STRUCT_BEGIN(REG_IPC_CREATE_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_CREATE_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_CREATE_KEY_EX_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_CREATE_KEY_EX_REQ, pClass),

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_REQ, dwOptions),

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_REQ, AccessDesired),

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_REQ, ulSecDescLen),
    LWMSG_MEMBER_PBYTE(REG_IPC_CREATE_KEY_EX_REQ, pSecDescRel),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_CREATE_KEY_EX_REQ, ulSecDescLen),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegCreateKeyExRespSpec[] =
{
    // OUT HKEY hkResult,
    // OUT OPTIONAL DWORD dwDisposition

    LWMSG_STRUCT_BEGIN(REG_IPC_CREATE_KEY_EX_RESPONSE),

    LWMSG_MEMBER_HANDLE(REG_IPC_CREATE_KEY_EX_RESPONSE, hkResult, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_RESPONSE, dwDisposition),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumRootKeysRespSpec[] =
{
    //PWSTR* ppwszRootKeyNames;
    //DWORD dwNumRootKeys;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_ROOTKEYS_RESPONSE),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_ROOTKEYS_RESPONSE, dwNumRootKeys),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_ENUM_ROOTKEYS_RESPONSE, ppwszRootKeyNames),
    LWMSG_PWSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_ROOTKEYS_RESPONSE, dwNumRootKeys),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegCloseKeySpec[] =
{
    // HKEY hKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_CLOSE_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_CLOSE_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteKeySpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_REQ, pSubKey),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteKeyValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // PCTSTR pValueName;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_KEY_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_KEY_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_VALUE_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_VALUE_REQ, pValueName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteTreeSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_TREE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_TREE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_TREE_REQ, pSubKey),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pValueName;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_VALUE_REQ, pValueName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegEnumKeyExSpec[] =
{
    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_ENUM_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_REQ, dwIndex),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_REQ, cName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_REQ, cClass),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumKeyExRespSpec[] =
{
    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_KEY_EX_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_KEY_EX_RESPONSE, pName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_RESPONSE, cName),

    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_KEY_EX_RESPONSE, pClass),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_RESPONSE, cClass),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegEnumValueSpec[] =
{
    //HKEY hKey;
    //DWORD dwIndex;
    //DWORD cName;
    //REG_DATA_TYPE type;
    //DWORD cValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_ENUM_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, dwIndex),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, cName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, cValue),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumValueRespSpec[] =
{
    //PWSTR pName;
    //DWORD cName;
    //PBYTE pValue;
    //DWORD cValue;
    //REG_DATA_TYPE type;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_VALUE_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_VALUE_RESPONSE, pName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, cName),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_RESPONSE, cName),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, type),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, cValue),
    LWMSG_MEMBER_PBYTE(REG_IPC_ENUM_VALUE_RESPONSE, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_RESPONSE, cValue),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegGetValueSpec[] =
{
    //HKEY hKey;
    //PCWSTR pSubKey;
    //PCWSTR pValue;
    //REG_DATA_TYPE_FLAGS Flags;
    //DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_GET_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_REQ, pSubKey),
    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_REQ, pValue),
    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_REQ, Flags),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegGetValueRespSpec[] =
{
    //DWORD dwType;
    //PBYTE pvData;
    //DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_RESPONSE),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_RESPONSE, dwType),
    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_RESPONSE, cbData),
    LWMSG_MEMBER_PBYTE(REG_IPC_GET_VALUE_RESPONSE, pvData),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_GET_VALUE_RESPONSE, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/
static LWMsgTypeSpec gRegOpenKeyExSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // ACCESS_MASK AccessDesired;

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_OPEN_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_OPEN_KEY_EX_REQ, pSubKey),

    LWMSG_MEMBER_UINT32(REG_IPC_OPEN_KEY_EX_REQ, AccessDesired),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegOpenKeyExRespSpec[] =
{
    // OUT HKEY hkResult;

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_KEY_EX_RESPONSE),

    LWMSG_MEMBER_HANDLE(REG_IPC_OPEN_KEY_EX_RESPONSE, hkResult, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegValueEntSpec[] =
{
    // PWSTR     ve_valuename;
    // DWORD     ve_valuelen;
    // PDWORD    ve_valueptr;
    // DWORD     ve_type;

    LWMSG_STRUCT_BEGIN(VALENT),

    LWMSG_MEMBER_PWSTR(VALENT, ve_valuename),
    LWMSG_MEMBER_UINT32(VALENT, ve_valuelen),
    LWMSG_MEMBER_POINTER(VALENT, ve_valueptr, LWMSG_UINT32(DWORD)),
    LWMSG_MEMBER_UINT32(VALENT, ve_type),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegQueryMultipleValuesSpec[] =
{
    // HKEY hKey;
    // DWORD num_vals;
    // PVALENT val_list;
    // DWORD dwTotalsize;
    // PWSTR pValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, num_vals),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, val_list),
    LWMSG_TYPESPEC(gRegValueEntSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, num_vals),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, dwTotalsize),
    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, dwTotalsize),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegQueryMultipleValuesRespSpec[] =
{
    // DWORD num_vals;
    // PVALENT val_list;
    // DWORD dwTotalsize;
    // PWSTR pValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, num_vals),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, val_list),
    LWMSG_TYPESPEC(gRegValueEntSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, num_vals),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, dwTotalsize),
    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, dwTotalsize),


    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegQueryInfoKeySpec[] =
{
    // HKEY hKey;
    // PDWORD pcClass;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_INFO_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_QUERY_INFO_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_POINTER(REG_IPC_QUERY_INFO_KEY_REQ, pcClass, LWMSG_UINT32(DWORD)),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegQueryInfoKeyRespSpec[] =
{
     //DWORD cSubKeys;
     //DWORD cMaxSubKeyLen;

     //DWORD cValues;
     //DWORD cMaxValueNameLen;
     //DWORD cMaxValueLen;
	 //DWORD cSecurityDescriptor;


    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_INFO_KEY_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_INFO_KEY_RESPONSE, pClass),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cSubKeys),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxSubKeyLen),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cValues),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxValueNameLen),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxValueLen),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cSecurityDescriptor),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegSetKeyValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // PCTSTR pValueName;
    // DWORD dwType;
    // PCVOID pData;
    // DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_KEY_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_KEY_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_KEY_VALUE_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_KEY_VALUE_REQ, pValueName),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_VALUE_REQ, dwType),
    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_VALUE_REQ, cbData),
    LWMSG_MEMBER_POINTER(REG_IPC_SET_KEY_VALUE_REQ, pData, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_SET_KEY_VALUE_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegSetValueExSpec[] =
{
    // HKEY hKey;
    // PCTSTR pValueName;
    //DWORD dwType;
    // const PBYTE pData;
    // DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_VALUE_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_VALUE_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_VALUE_EX_REQ, pValueName),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_VALUE_EX_REQ, dwType),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_VALUE_EX_REQ, cbData),

    LWMSG_MEMBER_POINTER(REG_IPC_SET_VALUE_EX_REQ, pData, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_SET_VALUE_EX_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegSetKeySecuritySpec[] =
{
    //HKEY hKey;
    //SECURITY_INFORMATION SecurityInformation;
    //PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    //ULONG Length;

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_KEY_SECURITY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_KEY_SECURITY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_SECURITY_REQ, SecurityInformation),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_SECURITY_REQ, Length),
    LWMSG_MEMBER_PBYTE(REG_IPC_SET_KEY_SECURITY_REQ, SecurityDescriptor),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_SET_KEY_SECURITY_REQ, Length),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegGetKeySecuritySpec[] =
{
    //HKEY hKey;
    //SECURITY_INFORMATION SecurityInformation;
    //ULONG Length;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_KEY_SECURITY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_GET_KEY_SECURITY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_GET_KEY_SECURITY_REQ, SecurityInformation),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_KEY_SECURITY_REQ, Length),

    LWMSG_MEMBER_UINT8(REG_IPC_GET_KEY_SECURITY_REQ, bRetSecurityDescriptor),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegGetKeySecurityResp[] =
{
    //PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor;
    //ULONG Length;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_KEY_SECURITY_RES),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_KEY_SECURITY_RES, Length),
    LWMSG_MEMBER_PBYTE(REG_IPC_GET_KEY_SECURITY_RES, SecurityDescriptor),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_GET_KEY_SECURITY_RES, Length),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegValueRangeInteger[] =
{
        LWMSG_STRUCT_BEGIN(LWREG_RANGE_INTEGER),

        LWMSG_MEMBER_UINT32(LWREG_RANGE_INTEGER, Min),
        LWMSG_MEMBER_UINT32(LWREG_RANGE_INTEGER, Max),

        LWMSG_STRUCT_END,
        LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegValueAttributesSpec[] =
{
    LWMSG_STRUCT_BEGIN(LWREG_VALUE_ATTRIBUTES),

    LWMSG_MEMBER_UINT32(LWREG_VALUE_ATTRIBUTES, ValueType),

    LWMSG_MEMBER_UINT32(LWREG_VALUE_ATTRIBUTES, DefaultValueLen),
    LWMSG_MEMBER_PBYTE(LWREG_VALUE_ATTRIBUTES, pDefaultValue),
    LWMSG_ATTR_LENGTH_MEMBER(LWREG_VALUE_ATTRIBUTES, DefaultValueLen),

    LWMSG_MEMBER_PWSTR(LWREG_VALUE_ATTRIBUTES, pwszDocString),

    LWMSG_MEMBER_UINT8(LWREG_VALUE_ATTRIBUTES, RangeType),
    LWMSG_MEMBER_UINT8(LWREG_VALUE_ATTRIBUTES, Hint),

    // range union
    LWMSG_MEMBER_UNION_BEGIN(LWREG_VALUE_ATTRIBUTES, Range),
    LWMSG_MEMBER_TYPESPEC(union _LWREG_RANGE, RangeInteger, gRegValueRangeInteger),
    LWMSG_ATTR_TAG(LWREG_VALUE_RANGE_TYPE_INTEGER),
    LWMSG_MEMBER_POINTER_BEGIN(union _LWREG_RANGE, ppwszRangeEnumStrings),
    LWMSG_PWSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_ATTR_TAG(LWREG_VALUE_RANGE_TYPE_ENUM),
    LWMSG_MEMBER_VOID(union _LWREG_RANGE, bool_empty), // can be any arbitrary name
    LWMSG_ATTR_TAG(LWREG_VALUE_RANGE_TYPE_BOOLEAN),
    LWMSG_MEMBER_VOID(union _LWREG_RANGE, empty), // can be any arbitrary name
    LWMSG_ATTR_TAG(LWREG_VALUE_RANGE_TYPE_NONE),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(LWREG_VALUE_ATTRIBUTES, RangeType),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegSetValueAttrsSpec[] =
{
    // HKEY hKey
    // PCWSTR pSubKey
    // PCWSTR pValueName
    // PLWREG_VALUE_ATTRIBUTES pValueAttributes

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_VALUE_ATTRS_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_VALUE_ATTRS_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_VALUE_ATTRS_REQ, pSubKey),
    LWMSG_MEMBER_PWSTR(REG_IPC_SET_VALUE_ATTRS_REQ, pValueName),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_SET_VALUE_ATTRS_REQ, pValueAttributes),
    LWMSG_TYPESPEC(gRegValueAttributesSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gRegDeleteValueAttrsSpec[] =
{
    // HKEY hKey
    // PCWSTR pSubKey
    // PCWSTR pValueName

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_VALUE_ATTRS_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_VALUE_ATTRS_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_VALUE_ATTRS_REQ, pSubKey),
    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_VALUE_ATTRS_REQ, pValueName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gRegGetValueAttrsSpec[] =
{
    // HKEY hKey
    // PCWSTR pSubKey
    // PCWSTR pValueName
    // BOOLEAN bRetCurrentValue;
    // BOOLEAN bRetValueAttributes

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_ATTRS_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_GET_VALUE_ATTRS_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_ATTRS_REQ, pSubKey),
    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_ATTRS_REQ, pValueName),
    LWMSG_MEMBER_UINT8(REG_IPC_GET_VALUE_ATTRS_REQ, bRetCurrentValue),
    LWMSG_MEMBER_UINT8(REG_IPC_GET_VALUE_ATTRS_REQ, bRetValueAttributes),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gRegCurrentValueInfo[] =
{
    // DWORD dwType
    // PVOID pvData
    // DWORD cbData

    LWMSG_STRUCT_BEGIN(LWREG_CURRENT_VALUEINFO),

    LWMSG_MEMBER_UINT32(LWREG_CURRENT_VALUEINFO, dwType),

    LWMSG_MEMBER_UINT32(LWREG_CURRENT_VALUEINFO, cbData),
    LWMSG_MEMBER_PBYTE(LWREG_CURRENT_VALUEINFO, pvData),
    LWMSG_ATTR_LENGTH_MEMBER(LWREG_CURRENT_VALUEINFO, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gRegGetValueAttrsResp[] =
{
    // PLWREG_CURRENT_VALUEINFO pCurrentValue
    // PLWREG_VALUE_ATTRIBUTES pValueAttributes

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_ATTRS_RESPONSE),

    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_GET_VALUE_ATTRS_RESPONSE, pCurrentValue),
    LWMSG_TYPESPEC(gRegCurrentValueInfo),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_GET_VALUE_ATTRS_RESPONSE, pValueAttributes),
    LWMSG_TYPESPEC(gRegValueAttributesSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgProtocolSpec gRegIPCSpec[] =
{
    /*Key Operation APIS*/
    LWMSG_MESSAGE(REG_R_ERROR, gRegIPCErrorSpec), // REG_IPC_ERROR
    LWMSG_MESSAGE(REG_Q_ENUM_ROOT_KEYSW, NULL),
    LWMSG_MESSAGE(REG_R_ENUM_ROOT_KEYSW, gRegEnumRootKeysRespSpec),
    LWMSG_MESSAGE(REG_Q_OPEN_KEYW_EX, gRegOpenKeyExSpec),
    LWMSG_MESSAGE(REG_R_OPEN_KEYW_EX, gRegOpenKeyExRespSpec),
    LWMSG_MESSAGE(REG_Q_CREATE_KEY_EX, gRegCreateKeyExSpec),
    LWMSG_MESSAGE(REG_R_CREATE_KEY_EX, gRegCreateKeyExRespSpec),
    LWMSG_MESSAGE(REG_Q_CLOSE_KEY, gRegCloseKeySpec),
    LWMSG_MESSAGE(REG_R_CLOSE_KEY, NULL),
    LWMSG_MESSAGE(REG_Q_DELETE_KEY, gRegDeleteKeySpec),
    LWMSG_MESSAGE(REG_R_DELETE_KEY, NULL),
    LWMSG_MESSAGE(REG_Q_QUERY_INFO_KEYW, gRegQueryInfoKeySpec),
    LWMSG_MESSAGE(REG_R_QUERY_INFO_KEYW, gRegQueryInfoKeyRespSpec),
    LWMSG_MESSAGE(REG_Q_ENUM_KEYW_EX, gRegEnumKeyExSpec),
    LWMSG_MESSAGE(REG_R_ENUM_KEYW_EX, gRegEnumKeyExRespSpec),
    /*Value Operation APIs*/
    LWMSG_MESSAGE(REG_Q_SET_VALUEW_EX, gRegSetValueExSpec),
    LWMSG_MESSAGE(REG_R_SET_VALUEW_EX, NULL),
    LWMSG_MESSAGE(REG_Q_GET_VALUEW, gRegGetValueSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEW, gRegGetValueRespSpec),
    LWMSG_MESSAGE(REG_Q_DELETE_KEY_VALUE, gRegDeleteKeyValueSpec),
    LWMSG_MESSAGE(REG_R_DELETE_KEY_VALUE, NULL),
    LWMSG_MESSAGE(REG_Q_DELETE_TREE, gRegDeleteTreeSpec),
    LWMSG_MESSAGE(REG_R_DELETE_TREE, NULL),
    LWMSG_MESSAGE(REG_Q_DELETE_VALUE, gRegDeleteValueSpec),
    LWMSG_MESSAGE(REG_R_DELETE_VALUE, NULL),
    LWMSG_MESSAGE(REG_Q_ENUM_VALUEW, gRegEnumValueSpec),
    LWMSG_MESSAGE(REG_R_ENUM_VALUEW, gRegEnumValueRespSpec),
    LWMSG_MESSAGE(REG_Q_QUERY_MULTIPLE_VALUES, gRegQueryMultipleValuesSpec),
    LWMSG_MESSAGE(REG_R_QUERY_MULTIPLE_VALUES, gRegQueryMultipleValuesRespSpec),
    LWMSG_MESSAGE(REG_Q_SET_KEY_VALUE, gRegSetKeyValueSpec),
    LWMSG_MESSAGE(REG_R_SET_KEY_VALUE, NULL),
    /*Key Security Operation APIs*/
    LWMSG_MESSAGE(REG_Q_SET_KEY_SECURITY, gRegSetKeySecuritySpec),
    LWMSG_MESSAGE(REG_R_SET_KEY_SECURITY, NULL),
    LWMSG_MESSAGE(REG_Q_GET_KEY_SECURITY, gRegGetKeySecuritySpec),
    LWMSG_MESSAGE(REG_R_GET_KEY_SECURITY, gRegGetKeySecurityResp),
    /*Value Attributes Operation APIs*/
    LWMSG_MESSAGE(REG_Q_SET_VALUEW_ATTRIBUTES, gRegSetValueAttrsSpec),
    LWMSG_MESSAGE(REG_R_SET_VALUEW_ATTRIBUTES, NULL),
    LWMSG_MESSAGE(REG_Q_GET_VALUEW_ATTRIBUTES, gRegGetValueAttrsSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEW_ATTRIBUTES, gRegGetValueAttrsResp),
    LWMSG_MESSAGE(REG_Q_DELETE_VALUEW_ATTRIBUTES, gRegDeleteValueAttrsSpec),
    LWMSG_MESSAGE(REG_R_DELETE_VALUEW_ATTRIBUTES, NULL),

    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
RegIPCGetProtocolSpec(
    void
    )
{
    return gRegIPCSpec;
}

NTSTATUS
RegMapLwmsgStatus(
    LWMsgStatus status
    )
{
    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        return STATUS_SUCCESS;

    case LWMSG_STATUS_ERROR:
    case LWMSG_STATUS_SYSTEM:
        return STATUS_INTERNAL_ERROR;

    case LWMSG_STATUS_MEMORY:
        return STATUS_NO_MEMORY;

    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_EOF:
        return STATUS_INVALID_NETWORK_RESPONSE;

    case LWMSG_STATUS_INVALID_PARAMETER:
    case LWMSG_STATUS_INVALID_STATE:
        return STATUS_INVALID_PARAMETER;

    case LWMSG_STATUS_UNIMPLEMENTED:
        return STATUS_NOT_IMPLEMENTED;

    case LWMSG_STATUS_SECURITY:
        return STATUS_ACCESS_DENIED;

    case LWMSG_STATUS_CANCELLED:
        return STATUS_MORE_PROCESSING_REQUIRED;

    case LWMSG_STATUS_FILE_NOT_FOUND:
        return LwErrnoToNtStatus(ENOENT);

    case LWMSG_STATUS_CONNECTION_REFUSED:
        return LwErrnoToNtStatus(ECONNREFUSED);

    case LWMSG_STATUS_PEER_RESET:
        return LwErrnoToNtStatus(ECONNRESET);

    case LWMSG_STATUS_PEER_ABORT:
        return LwErrnoToNtStatus(ECONNABORTED);

    case LWMSG_STATUS_PEER_CLOSE:
        return LwErrnoToNtStatus(EPIPE);

    case LWMSG_STATUS_SESSION_LOST:
        return LwErrnoToNtStatus(EPIPE);

    case LWMSG_STATUS_NOT_FOUND:
        return STATUS_NOT_FOUND;

    default:
        return STATUS_INTERNAL_ERROR;
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
