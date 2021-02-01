/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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

/*
 * UNIX struct passwd and related AD RFC2307 attributes
 */
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

/* used when processing to discriminate between format changes */
#define AD_USER_CHANGE_VERSION  "(v2)"
#define AD_USER_INFO_VERSION 0

/*
 * AD user attributes
 */
typedef STRUCT _AD_USER_INFO
{
    // the version of this struct
    DWORD version;

    // This is a subset of LSA_SECURITY_OBJECT,
    // and sub LSA_SECURITY_OBJECT_USER_INFO
    // struct

    PSTR pszDN;
    PSTR pszObjectSid;
    BOOL enabled;
    BOOL bIsLocal;
    PSTR pszNetbiosDomainName;
    PSTR pszSamAccountName;

    // the LSA_SECURITY_OBJECT_USER_INFO
    PSTR pszPrimaryGroupSid;
    PSTR pszUPN;
    PSTR pszAliasName;

    // NT time values
    UINT64 qwPwdLastSet;
    UINT64 qwMaxPwdAge;
    UINT64 qwPwdExpires;
    UINT64 qwAccountExpires;

    BOOL bIsGeneratedUPN;
    BOOL bIsAccountInfoKnown;
    BOOL bPasswordExpired;
    BOOL bPasswordNeverExpires;
    BOOL bPromptPasswordChange;
    BOOL bUserCanChangePassword;
    BOOL bAccountDisabled;
    BOOL bAccountExpired;
    BOOL bAccountLocked;

    // the UNIX attributes
    DWORD pw_uid;
    DWORD pw_gid;
    PSTR pw_name;
    PSTR pw_passwd;
    PSTR pw_gecos;
    PSTR pw_shell;
    PSTR pw_dir;

    PSTR pDisplayName;

    PSTR pszWindowsHomeFolder;
    PSTR pszLocalWindowsHomeFolder;

    DWORD LastUpdated;
} AD_USER_INFO, *PAD_USER_INFO;

typedef STRUCT USER_MONITOR_GROUP
{
    PSTR gr_name;
    PSTR gr_passwd;
    DWORD gr_gid;
    DWORD LastUpdated;
} USER_MONITOR_GROUP, *PUSER_MONITOR_GROUP;

/* new, changed and deleted local users */
typedef STRUCT _USER_CHANGE
{
    // unix passwd like attributes
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

/* new, changed and deleted AD users */
typedef STRUCT _AD_USER_CHANGE
{
    AD_USER_INFO OldValue;
    AD_USER_INFO ADNewValue;
} AD_USER_CHANGE, *PAD_USER_CHANGE;


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

VOID
LW_USERMONITORLIB_API
FreeADUserChange(
    PAD_USER_CHANGE pValue
    );

DWORD
LW_USERMONITORLIB_API
DecodeADUserChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PAD_USER_CHANGE* ppValue
    );

DWORD
LW_USERMONITORLIB_API
EncodeADUserChange(
    IN PAD_USER_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    );
#ifdef __cplusplus
}
#endif

#endif

#if defined(_DCE_IDL_) || defined(__midl)
cpp_quote("#endif")
#endif

#endif /* __USERMONITOR_H__ */
