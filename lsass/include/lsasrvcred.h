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
 *        lsasrvcred.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __LSASRVCRED_H__
#define __LSASRVCRED_H__

#include <lw/base.h>
#include <lwdef.h>

struct _LSA_CREDENTIALS;
typedef struct _LSA_CREDENTIALS *LSA_CRED_HANDLE, **PLSA_CRED_HANDLE;

DWORD
LsaAddCredential(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const uid_t* pUid,
    OUT PLSA_CRED_HANDLE phCredential
    );

VOID
LsaReferenceCredential(
    IN LSA_CRED_HANDLE hCredential
    );

VOID
LsaReleaseCredential(
    IN PLSA_CRED_HANDLE hCredential
    );

LSA_CRED_HANDLE
LsaGetCredential(
    IN uid_t Uid
    );

VOID
LsaGetCredentialInfo(
    IN LSA_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL uid_t* pUid
    );

#endif /* __LSASRVCRED_H__ */
