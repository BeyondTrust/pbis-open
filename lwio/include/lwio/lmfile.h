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

#ifndef _LMFILE_H_
#define _LMFILE_H_

#include <lw/types.h>

#ifndef FILE_INFO_2_DEFINED
#define FILE_INFO_2_DEFINED 1

typedef struct _FILE_INFO_2 {
    UINT32 fi2_id;
} FILE_INFO_2, *PFILE_INFO_2;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    FILE_INFO_2 *array;
} srvsvc_NetFileCtr2;


#ifndef FILE_INFO_3_DEFINED
#define FILE_INFO_3_DEFINED 1

typedef struct _FILE_INFO_3 {
    UINT32 fi3_idd;
    UINT32 fi3_permissions;
    UINT32 fi3_num_locks;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_path_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_username;
} FILE_INFO_3, *PFILE_INFO_3;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    FILE_INFO_3 *array;
} srvsvc_NetFileCtr3;


#ifndef _DCE_IDL_

typedef union {
    srvsvc_NetFileCtr2 *ctr2;
    srvsvc_NetFileCtr3 *ctr3;
} srvsvc_NetFileCtr;


typedef union {
    FILE_INFO_2 *info2;
    FILE_INFO_3 *info3;
} srvsvc_NetFileInfo;

#endif

#endif /* _LMFILE_H_ */
