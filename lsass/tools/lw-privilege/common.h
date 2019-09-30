/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        common.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
