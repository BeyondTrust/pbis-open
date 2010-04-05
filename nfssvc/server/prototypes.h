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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWNFSSVC)
 *
 * Server Main
 *
 * Function Prototypes
 *
 */

// config.c

DWORD
NfsSvcReadConfigSettings(
    VOID
    );

DWORD
NfsSvcConfigGetLsaLpcSocketPath(
    PSTR* ppszPath
    );

// main.c

VOID
NfsSvcSetProcessShouldExit(
    BOOLEAN val
    );

// signalhandler.c

VOID
NfsSvcBlockSelectedSignals(
    VOID
    );

DWORD
NfsSvcHandleSignals(
    VOID
    );

// nfssvc.c

NET_API_STATUS
NfsSvcInitSecurity(
    void
    );

NET_API_STATUS
NfsSvcNetShareAdd(
    handle_t IDL_handle,
    wchar16_t *server_name,
    UINT32 level,
    nfssvc_NetShareInfo info,
    UINT32 *parm_error
    );


NET_API_STATUS
NfsSvcNetShareEnum(
    handle_t IDL_handle,
    wchar16_t *server_name,
    UINT32 *level,
    nfssvc_NetShareCtr *ctr,
    UINT32 preferred_maximum_length,
    UINT32 *total_entries,
    UINT32 *resume_handle
    );


NET_API_STATUS
NfsSvcNetShareGetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 level,
    nfssvc_NetShareInfo *info
    );


NET_API_STATUS
NfsSvcNetShareSetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 level,
    nfssvc_NetShareInfo info,
    UINT32 *parm_error
    );


NET_API_STATUS
NfsSvcNetShareDel(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 reserved
    );


NET_API_STATUS
NfsSvcNetrServerGetInfo(
    handle_t b,
    wchar16_t *server_name,
    UINT32 level,
    nfssvc_NetNfsInfo *info
    );


NET_API_STATUS
NfsSvcNetNameValidate(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *name,
    UINT32 type,
    UINT32 flags
    );

// utils.c

DWORD
NfsSvcNfsAllocateWC16StringFromUnicodeStringEx(
    OUT PWSTR            *ppwszOut,
    IN  UnicodeStringEx  *pIn
    );

DWORD
NfsSvcNfsAllocateWC16String(
    OUT PWSTR  *ppwszOut,
    IN  PCWSTR  pwszIn
    );

DWORD
NfsSvcNfsAllocateWC16StringFromCString(
    OUT PWSTR  *ppwszOut,
    IN  PCSTR   pszIn
    );
