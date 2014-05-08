/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        usermonitor.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Public header
 *
 * Authors: Kyle Stemen <kstemen@beyondtrust.com>
 * 
 */
#ifndef __USERMONITOR_H__
#define __USERMONITOR_H__

#if defined(_DCE_IDL_) || defined(__midl)
cpp_quote("#include <usermonitor-encoding.h>")
cpp_quote("#if 0")
#endif

#ifdef __cplusplus_cli
#define STRUCT value struct
#else
#define STRUCT struct
#endif

typedef STRUCT _USER_MONITOR_PASSWD
{
    PSTR pw_name;
    PSTR pw_passwd;
    DWORD pw_uid;
    DWORD pw_gid;
    PSTR pw_gecos;
    PSTR pw_dir;
    PSTR pw_shell;
    PSTR pDisplayName;
    DWORD LastUpdated;
} USER_MONITOR_PASSWD, *PUSER_MONITOR_PASSWD;

typedef STRUCT USER_MONITOR_GROUP
{
    PSTR gr_name;
    PSTR gr_passwd;
    DWORD gr_gid;
    DWORD LastUpdated;
} USER_MONITOR_GROUP, *PUSER_MONITOR_GROUP;

typedef STRUCT _USER_CHANGE
{
    USER_MONITOR_PASSWD OldValue;
    USER_MONITOR_PASSWD NewValue;
} USER_CHANGE, *PUSER_CHANGE;

typedef STRUCT _GROUP_CHANGE
{
    USER_MONITOR_GROUP OldValue;
    USER_MONITOR_GROUP NewValue;
} GROUP_CHANGE, *PGROUP_CHANGE;

typedef STRUCT _GROUP_MEMBERSHIP_CHANGE
{
    BOOL Added;
    BOOL OnlyGidChange;
    PSTR pUserName;
    DWORD Gid;
    PSTR pGroupName;
} GROUP_MEMBERSHIP_CHANGE, *PGROUP_MEMBERSHIP_CHANGE;

#if !defined(_DCE_IDL_) && !defined(__midl)

#ifdef _WIN32
#ifndef LW_USERMONITORLIB_API
#define LW_USERMONITORLIB_API __declspec(dllimport) __stdcall
#endif
#else
#define LW_USERMONITORLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

DWORD
LW_USERMONITORLIB_API
DecodeUserChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PUSER_CHANGE* ppValue
    );

DWORD
LW_USERMONITORLIB_API
EncodeUserChange(
    IN PUSER_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    );

VOID
LW_USERMONITORLIB_API
FreeUserChange(
    PUSER_CHANGE pValue
    );

DWORD
LW_USERMONITORLIB_API
DecodeGroupChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PGROUP_CHANGE* ppValue
    );

DWORD
LW_USERMONITORLIB_API
EncodeGroupChange(
    IN PGROUP_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    );

VOID
LW_USERMONITORLIB_API
FreeGroupChange(
    PGROUP_CHANGE pValue
    );

DWORD
LW_USERMONITORLIB_API
DecodeGroupMembershipChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PGROUP_MEMBERSHIP_CHANGE* ppValue
    );

DWORD
LW_USERMONITORLIB_API
EncodeGroupMembershipChange(
    IN PGROUP_MEMBERSHIP_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    );

VOID
LW_USERMONITORLIB_API
FreeGroupMembershipChange(
    PGROUP_MEMBERSHIP_CHANGE pValue
    );

#ifdef __cplusplus
}
#endif

#endif

#if defined(_DCE_IDL_) || defined(__midl)
cpp_quote("#endif")
#endif

#endif /* __USERMONITOR_H__ */
