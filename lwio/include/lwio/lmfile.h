/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
