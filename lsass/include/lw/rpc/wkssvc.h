/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        wkssvc.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Workstation service (wkssvc) library
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef _RPC_WKSSVC_H_
#define _RPC_WKSSVC_H_

/*
 * Join domain flags
 */
#define NETSETUP_JOIN_DOMAIN                (0x00000001)
#define NETSETUP_ACCT_CREATE                (0x00000002)
#define NETSETUP_ACCT_DELETE                (0x00000004)
#define NETSETUP_WIN9X_UPGRADE              (0x00000010)
#define NETSETUP_DOMAIN_JOIN_IF_JOINED      (0x00000020)
#define NETSETUP_JOIN_UNSECURE              (0x00000040)
#define NETSETUP_MACHINE_PWD_PASSED         (0x00000080)
#define NETSETUP_DEFER_SPN_SET              (0x00000100)


typedef struct _WKSTA_INFO_100
{
    DWORD  wksta100_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta100_name;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta100_domain;
    DWORD  wksta100_version_major;
    DWORD  wksta100_version_minor;

} WKSTA_INFO_100, *PWKSTA_INFO_100;


typedef struct _WKSTA_INFO_101
{
    DWORD  wksta101_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta101_name;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta101_domain;
    DWORD  wksta101_version_major;
    DWORD  wksta101_version_minor;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta101_lan_root;

} WKSTA_INFO_101, *PWKSTA_INFO_101;


typedef struct _WKSTA_INFO_102
{
    DWORD  wksta102_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta102_name;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta102_domain;
    DWORD  wksta102_version_major;
    DWORD  wksta102_version_minor;

#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wksta102_lan_root;
    DWORD  wksta102_logged_users;

} WKSTA_INFO_102, *PWKSTA_INFO_102;


#ifndef _DCE_IDL_
typedef union _NETR_WKSTA_INFO
{
    WKSTA_INFO_100 *pInfo100;
    WKSTA_INFO_101 *pInfo101;
    WKSTA_INFO_102 *pInfo102;

} NETR_WKSTA_INFO, *PNETR_WKSTA_INFO;
#endif


typedef struct _NETR_WKSTA_USER_INFO_0
{
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wkui0_username;

} NETR_WKSTA_USER_INFO_0, *PNETR_WKSTA_USER_INFO_0;


typedef struct _NETR_WKSTA_USER_INFO_1
{
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wkui1_username;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wkui1_logon_domain;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wkui1_oth_domains;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR  wkui1_logon_server;

} NETR_WKSTA_USER_INFO_1, *PNETR_WKSTA_USER_INFO_1;


typedef struct _NETR_WKSTA_USER_INFO_CTR_0
{
    DWORD  dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    NETR_WKSTA_USER_INFO_0 *pInfo;

} NETR_WKSTA_USER_INFO_CTR_0, *PNETR_WKSTA_USER_INFO_CTR_0;


typedef struct _NETR_WKSTA_USER_INFO_CTR_1
{
    DWORD  dwCount;
#ifdef _DCE_IDL_
    [size_is(dwCount)]
#endif
    NETR_WKSTA_USER_INFO_1 *pInfo;

} NETR_WKSTA_USER_INFO_CTR_1, *PNETR_WKSTA_USER_INFO_CTR_1;


#ifndef _DCE_IDL_
typedef union _NETR_WKSTA_USER_INFO_CTR
{
    NETR_WKSTA_USER_INFO_CTR_0  *pInfo0;
    NETR_WKSTA_USER_INFO_CTR_1  *pInfo1;

} NETR_WKSTA_USER_INFO_CTR, *PNETR_WKSTA_USER_INFO_CTR;


typedef struct _NETR_WKSTA_USER_INFO
{
    DWORD  dwLevel;
    NETR_WKSTA_USER_INFO_CTR Ctr;

} NETR_WKSTA_USER_INFO, *PNETR_WKSTA_USER_INFO;
#endif


typedef struct _ENC_JOIN_PASSWORD_BUFFER
{
    BYTE data[524];

} ENC_JOIN_PASSWORD_BUFFER, *PENC_JOIN_PASSWORD_BUFFER;


#ifndef _DCE_IDL_

typedef
void* WKSS_BINDING;

typedef
WKSS_BINDING *PWKSS_BINDING;


WINERROR
WkssInitBindingDefault(
    OUT PWKSS_BINDING   phBinding,
    IN  PCWSTR          pwszHostname,
    IN  LW_PIO_CREDS    pCreds
    );


WINERROR
WkssInitBindingFull(
    OUT PWKSS_BINDING   phBinding,
    IN  PCWSTR          pwszProtSeq,
    IN  PCWSTR          pwszHostname,
    IN  PCWSTR          pwszEndpoint,
    IN  PCWSTR          pwszUuid,
    IN  PCWSTR          pwszOptions,
    IN  LW_PIO_CREDS    pCreds
    );


#define WkssInitBindingFromBindingString(binding_ptr, binding_str, creds_ptr) \
    RpcInitBindingFromBindingString((handle_t*)(binding_ptr),                 \
                                    (binding_str),                            \
                                    (creds_ptr));


VOID
WkssFreeBinding(
    IN  PWKSS_BINDING   phBinding
    );


WINERROR
NetrWkstaGetInfo(
    IN  WKSS_BINDING       hBinding,
    IN  PWSTR              pwszServerName,
    IN  DWORD              dwLevel,
    OUT PNETR_WKSTA_INFO   pInfo
    );


WINERROR
NetrWkstaUserEnum(
    IN  WKSS_BINDING    hBinding,
    IN  PWSTR           pwszServerName,
    IN  DWORD           dwLevel,
    IN  DWORD           dwPrefMaxLen,
    OUT PVOID          *ppInfo,
    OUT PDWORD          pdwSize,
    OUT PDWORD          pdwNumEntries,
    OUT PDWORD          pdwTotalNumEntries,
    IN OUT PDWORD       pdwResume
    );


WINERROR
NetrJoinDomain2(
    IN  WKSS_BINDING               hBinding,
    IN  PWSTR                      pwszServerName,
    IN  PWSTR                      pwszDomainName,
    IN  PWSTR                      pwszAccountOu,
    IN  PWSTR                      pwszAccountName,
    IN  PENC_JOIN_PASSWORD_BUFFER  pPassword,
    IN  DWORD                      dwJoinFlags
    );


WINERROR
NetrUnjoinDomain2(
    IN  WKSS_BINDING               hBinding,
    IN  PWSTR                      pwszServerName,
    IN  PWSTR                      pwszAccountName,
    IN  PENC_JOIN_PASSWORD_BUFFER  pPassword,
    IN  DWORD                      dwUnjoinFlags
    );


VOID
WkssFreeMemory(
    IN PVOID pPtr
    );


#endif /* _DCE_IDL_ */

#endif /* _RPC_WKSSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
