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

#ifndef _LMSHARE_H_
#define _LMSHARE_H_

#include <lw/types.h>

#ifndef SHARE_INFO_0_DEFINED
#define SHARE_INFO_0_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_0_DEFINED")
cpp_quote("#define SHARE_INFO_0_DEFINED 1")
#endif

typedef struct _SHARE_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi0_netname;
} SHARE_INFO_0, *PSHARE_INFO_0;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1_DEFINED
#define SHARE_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1_DEFINED")
cpp_quote("#define SHARE_INFO_1_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1_netname;
    UINT32 shi1_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1_remark;
} SHARE_INFO_1, *PSHARE_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_2_DEFINED
#define SHARE_INFO_2_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_2_DEFINED")
cpp_quote("#define SHARE_INFO_2_DEFINED 1")
#endif

typedef struct _SHARE_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_netname;
    UINT32 shi2_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_remark;
    UINT32 shi2_permissions;
    UINT32 shi2_max_uses;
    UINT32 shi2_current_uses;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_path;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_password;
} SHARE_INFO_2, *PSHARE_INFO_2;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_501_DEFINED
#define SHARE_INFO_501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_501_DEFINED")
cpp_quote("#define SHARE_INFO_501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_501 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi501_netname;
    UINT32 shi501_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi501_remark;
    UINT32 shi501_flags;
} SHARE_INFO_501, *PSHARE_INFO_501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_502_DEFINED
#define SHARE_INFO_502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_502_DEFINED")
cpp_quote("#define SHARE_INFO_502_DEFINED 1")
#endif

typedef struct _SHARE_INFO_502 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi502_netname;
    UINT32 shi502_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi502_remark;
    UINT32 shi502_permissions;
    UINT32 shi502_max_uses;
    UINT32 shi502_current_uses;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi502_path;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi502_password;
    UINT32 shi502_reserved;
#ifdef _DCE_IDL_
    [size_is(shi502_reserved)]
#endif
    PBYTE shi502_security_descriptor;
} SHARE_INFO_502, *PSHARE_INFO_502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_502_I_DEFINED
#define SHARE_INFO_502_I_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_502_I_DEFINED")
cpp_quote("#define SHARE_INFO_502_I_DEFINED 1")
#endif

typedef struct _SHARE_INFO_502_I {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  shi502_netname;
    UINT32 shi502_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  shi502_remark;
    UINT32 shi502_permissions;
    UINT32 shi502_max_uses;
    UINT32 shi502_current_uses;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  shi502_path;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  shi502_password;
    UINT32 shi502_reserved;
#ifdef _DCE_IDL_
    [size_is(shi502_reserved)]
#endif
    PBYTE  shi502_security_descriptor;
} SHARE_INFO_502_I, *PSHARE_INFO_502_I;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1004_DEFINED
#define SHARE_INFO_1004_DEFINED 1


#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1004_DEFINED")
cpp_quote("#define SHARE_INFO_1004_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1004 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1004_remark;
} SHARE_INFO_1004, *PSHARE_INFO_1004;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1005_DEFINED
#define SHARE_INFO_1005_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1005_DEFINED")
cpp_quote("#define SHARE_INFO_1005_DEFINED 1")
#endif

#define SHARE_INFO_FLAG_IN_DFS                  0x00000001
#define SHARE_INFO_FLAG_DFS_ROOT                0x00000002
#define SHARE_INFO_FLAG_CSC_POLICY_MASK         0x00000030
#define SHARE_INFO_FLAG_CSC_CACHE_MANUAL_REINT  0x00000000
#define SHARE_INFO_FLAG_CSC_CACHE_AUTO_REINT    0x00000010
#define SHARE_INFO_FLAG_CSC_CACHE_VDO           0x00000020
#define SHARE_INFO_FLAG_CSC_CACHE_NONE          0x00000030
#define SHARE_INFO_FLAG_RESTRICT_EXCLUSIVE_OPEN 0x00000100
#define SHARE_INFO_FLAG_FORCE_SHARED_DELETE     0x00000200
#define SHARE_INFO_FLAG_ALLOW_NAMESPACE_CACHE   0x00000400
#define SHARE_INFO_FLAG_ABE_ENABLED             0x00000800

typedef struct _SHARE_INFO_1005 {
    UINT32 shi1005_flags;
} SHARE_INFO_1005, *PSHARE_INFO_1005;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1006_DEFINED
#define SHARE_INFO_1006_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1006_DEFINED")
cpp_quote("#define SHARE_INFO_1006_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1006 {
    UINT32 shi1006_max_uses;
} SHARE_INFO_1006, *PSHARE_INFO_1006;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1501_DEFINED
#define SHARE_INFO_1501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1501_DEFINED")
cpp_quote("#define SHARE_INFO_1501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1501 {
    UINT32 shi1501_reserved;
#ifdef _DCE_IDL_
    [size_is(shi1501_reserved)]
#endif
    PBYTE shi1501_security_descriptor;
} SHARE_INFO_1501, *PSHARE_INFO_1501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1501_I_DEFINED
#define SHARE_INFO_1501_I_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1501_I_DEFINED")
cpp_quote("#define SHARE_INFO_1501_I_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1501_I {
    UINT32 shi1501_reserved;
#ifdef _DCE_IDL_
    [size_is(shi1501_reserved)]
#endif
    PBYTE  shi1501_security_descriptor;
} SHARE_INFO_1501_I, *PSHARE_INFO_1501_I;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef _DCE_IDL_

typedef union {
    SHARE_INFO_0 *info0;
    SHARE_INFO_1 *info1;
    SHARE_INFO_2 *info2;
    SHARE_INFO_501 *info501;
    SHARE_INFO_502_I *info502;
    SHARE_INFO_1004 *info1004;
    SHARE_INFO_1005 *info1005;
    SHARE_INFO_1006 *info1006;
    SHARE_INFO_1501_I *info1501;
} srvsvc_NetShareInfo;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SHARE_INFO_0 *array;
} srvsvc_NetShareCtr0;

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SHARE_INFO_1 *array;
} srvsvc_NetShareCtr1;

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SHARE_INFO_2 *array;
} srvsvc_NetShareCtr2;

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SHARE_INFO_501 *array;
} srvsvc_NetShareCtr501;

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SHARE_INFO_502_I *array;
} srvsvc_NetShareCtr502;

#ifndef _DCE_IDL_

typedef union {
    srvsvc_NetShareCtr0 *ctr0;
    srvsvc_NetShareCtr1 *ctr1;
    srvsvc_NetShareCtr2 *ctr2;
    srvsvc_NetShareCtr501 *ctr501;
    srvsvc_NetShareCtr502 *ctr502;
} srvsvc_NetShareCtr;

#endif


#endif /* _LMSHARE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
