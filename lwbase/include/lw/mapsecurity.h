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

#ifndef __LW_MAP_SECURITY_H__
#define __LW_MAP_SECURITY_H__

#include <lw/security-types.h>
#include <lw/ntstatus.h>
#include <lw/mapsecurity-types.h>

LW_BEGIN_EXTERN_C

typedef struct _LW_MAP_SECURITY_CONTEXT *PLW_MAP_SECURITY_CONTEXT;

typedef struct gss_ctx_id_struct *LW_MAP_SECURITY_GSS_CONTEXT;

//
// Every successful call to LwMapSecurityInitialize() must have
// exactly one corresponding call to LwMapSecurityCleanup().
// However, it is not necessary to call LwMapSecurityInitialize()
// before calling LwMapSecurityCreateContext().  Also, the library
// may return references to existing contexts and just do reference
// counting.
//

NTSTATUS
LwMapSecurityInitialize(
    VOID
    );

VOID
LwMapSecurityCleanup(
    VOID
    );

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    );

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    );

NTSTATUS
LwMapSecurityGetSidFromName(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN PCSTR Name
    );

NTSTATUS
LwMapSecurityGetNameFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSTR* Domain,
    OUT PSTR* Name,
    OUT PBOOLEAN IsUser,
    IN PSID Sid
    );

VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    );

VOID
LwMapSecurityFreeCString(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSTR* String
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN ULONG Uid,
    IN ULONG Gid
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PUNICODE_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PANSI_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCWSTR Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCSTR Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromGssContext(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN LW_MAP_SECURITY_GSS_CONTEXT GssContext
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromNtlmLogon(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* pAccessToken,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmInfo,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    );

VOID
LwMapSecurityFreeNtlmLogonResult(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* pNtlmResult
    );

NTSTATUS
LwMapSecurityGetLocalGuestAccountSid(
    IN PLW_MAP_SECURITY_CONTEXT pContext,
    OUT PSID* ppSid
    );

LW_END_EXTERN_C

#endif /* __LW_MAP_SECURITY_H__ */
