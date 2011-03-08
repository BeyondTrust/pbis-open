/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        common.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LSA Privileges, System Access Rights and Account Rights manager
 *        (rpc client utility).
 *        Common routines.
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _COMMON_H_
#define _COMMON_H_


typedef enum _CREDS_TYPE {
    CredsUnknown = 0,
    CredsKrb5,
    CredsNtlm,
} CREDS_TYPE;


typedef struct _RPC_PARAMETERS {
    PSTR pszHost;
    PSTR pszBindingString;

    CREDS_TYPE Credentials;

    union _CREDS {
        struct _KRB5 {
            PSTR pszPrincipal;
            PSTR pszCache;
        } Krb5;
        struct _NTLM {
            PSTR pszDomainName;
            PSTR pszUserName;
            PSTR pszPassword;
        } Ntlm;
    } Creds;

} RPC_PARAMETERS, *PRPC_PARAMETERS;


DWORD
ProcessRpcParameters(
    IN const DWORD Argc,
    IN PCSTR* Argv,
    OUT PRPC_PARAMETERS pParams
    );

VOID
ReleaseRpcParameters(
    IN PRPC_PARAMETERS pParams
    );

DWORD
CreateRpcCredentials(
    IN PRPC_PARAMETERS pRpcParams,
    OUT LW_PIO_CREDS *ppCreds
    );

DWORD
CreateLsaRpcBinding(
    IN PRPC_PARAMETERS pRpcParams,
    IN LW_PIO_CREDS pCreds,
    OUT LSA_BINDING *phLsa
    );

DWORD
ResolveAccountNameToSid(
    IN LSA_BINDING hLsa,
    IN PSTR pszAccountName,
    OUT PSID *ppAccountSid
    );


#define SEPARATOR_CHAR  ','

DWORD
GetStringListFromString(
    PSTR pszStr,
    CHAR Separator,
    PSTR **pppszList,
    PDWORD pNumElements
    );


#define LUID_SEPARATOR_CHAR  ';'

DWORD
GetLuidListFromString(
    IN PSTR pszStr,
    PLUID *ppLuidList,
    PDWORD pNumElements
    );


#endif /* _PRIVILEGES_H_ */
