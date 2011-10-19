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

#ifndef _LMSESSION_H_
#define _LMSESSION_H_

#include <lw/types.h>

#ifndef SESSION_INFO_0_DEFINED
#define SESSION_INFO_0_DEFINED 1

typedef struct _SESSION_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi0_cname;
} SESSION_INFO_0, *PSESSION_INFO_0;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_0 *array;
} srvsvc_NetSessCtr0;


#ifndef SESSION_INFO_1_DEFINED
#define SESSION_INFO_1_DEFINED 1

typedef struct _SESSION_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_username;
    UINT32 sesi1_num_opens;
    UINT32 sesi1_time;
    UINT32 sesi1_idle_time;
    UINT32 sesi1_user_flags;
} SESSION_INFO_1, *PSESSION_INFO_1;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_1 *array;
} srvsvc_NetSessCtr1;


#ifndef SESSION_INFO_2_DEFINED
#define SESSION_INFO_2_DEFINED 1


typedef struct _SESSION_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_username;
    UINT32 sesi2_num_opens;
    UINT32 sesi2_time;
    UINT32 sesi2_idle_time;
    UINT32 sesi2_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cltype_name;
} SESSION_INFO_2, *PSESSION_INFO_2;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_2 *array;
} srvsvc_NetSessCtr2;


#ifndef SESSION_INFO_10_DEFINED
#define SESSION_INFO_10_DEFINED 1

typedef struct _SESSION_INFO_10 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_username;
    UINT32 sesi10_time;
    UINT32 sesi10_idle_time;
} SESSION_INFO_10, *PSESSION_INFO_10;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_10 *array;
} srvsvc_NetSessCtr10;


#ifndef SESSION_INFO_502_DEFINED
#define SESSION_INFO_502_DEFINED 1

typedef struct _SESSION_INFO_502 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_username;
    UINT32 sesi502_num_opens;
    UINT32 sesi502_time;
    UINT32 sesi502_idle_time;
    UINT32 sesi502_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cltype_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_transport;
} SESSION_INFO_502, *PSESSION_INFO_502;

#endif

typedef struct {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SESSION_INFO_502 *array;
} srvsvc_NetSessCtr502;


#ifndef _DCE_IDL_

typedef union {
    srvsvc_NetSessCtr0 *ctr0;
    srvsvc_NetSessCtr1 *ctr1;
    srvsvc_NetSessCtr2 *ctr2;
    srvsvc_NetSessCtr10 *ctr10;
    srvsvc_NetSessCtr502 *ctr502;
} srvsvc_NetSessCtr;

#endif

#endif /* _LMSESSION_H_ */

