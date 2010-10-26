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

#ifndef _LMCONNECTION_H_
#define _LMCONNECTION_H_

#include <lw/types.h>

typedef struct _CONNECTION_INFO_0 {
    UINT32 coni0_id;
} CONNECTION_INFO_0, *PCONNECTION_INFO_0;


typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    CONNECTION_INFO_0 *array;
} srvsvc_NetConnCtr0;


typedef struct _CONNECTION_INFO_1 {
    UINT32 coni1_id;
    UINT32 coni1_type;
    UINT32 coni1_num_opens;
    UINT32 coni1_num_users;
    UINT32 coni1_time;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_username;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_netname;
} CONNECTION_INFO_1, *PCONNECTION_INFO_1;


typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    CONNECTION_INFO_1 *array;
} srvsvc_NetConnCtr1;


#ifndef _DCE_IDL_

typedef union {
    srvsvc_NetConnCtr0 *ctr0;
    srvsvc_NetConnCtr1 *ctr1;
} srvsvc_NetConnCtr;

#endif

#endif /* _LMCONNECTION_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
