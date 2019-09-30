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
 *        batch_marshal.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef _BATCH_MARSHAL_H_
#define _BATCH_MARSHAL_H_

#include "batch_common.h"

DWORD
LsaAdBatchMarshalList(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    IN DWORD dwAvailableCount,
    OUT PLSA_SECURITY_OBJECT* ppObjects,
    OUT PDWORD pdwUsedCount
    );

DWORD
LsaAdBatchMarshal(
    IN PLSA_AD_PROVIDER_STATE pState,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
LsaAdBatchMarshalUserInfoAccountExpires(
    IN UINT64 AccountExpires,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszSamAccountName
    );

DWORD
LsaAdBatchMarshalUserInfoPasswordExpires(
    IN UINT64 PasswordExpires,
    IN OUT PLSA_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszSamAccountName
    );

#endif /* _BATCH_MARSHAL_H_ */
