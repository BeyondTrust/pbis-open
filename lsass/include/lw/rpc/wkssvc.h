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


#define WKSS_DEFAULT_PROT_SEQ   "ncacn_np"
#define WKSS_DEFAULT_ENDPOINT   "\\PIPE\\wkssvc"
#define WKSS_LOCAL_ENDPOINT     "/var/lib/likewise/rpc/lsass"


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
    DWORD  dwLoggedUsers;

} WKSTA_INFO_102, *PWKSTA_INFO_102;


#ifndef _DCE_IDL_
typedef union _NETR_WKSTA_INFO
{
    WKSTA_INFO_100 *pInfo100;
    WKSTA_INFO_101 *pInfo101;
    WKSTA_INFO_102 *pInfo102;

} NETR_WKSTA_INFO, *PNETR_WKSTA_INFO;
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


VOID
WkssFreeBinding(
    IN  PWKSS_BINDING   phBinding
    );


WINERROR
NetrWkstaGetInfo(
    IN WKSS_BINDING       hBinding,
    IN PWSTR              pwszServerName,
    IN DWORD              dwLevel,
    OUT PNETR_WKSTA_INFO  pInfo
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
