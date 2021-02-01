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
 *        state_store.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __STATE_STORE_H__
#define __STATE_STORE_H__

DWORD
ADState_EmptyDb(
    IN PCSTR pszDomainName
    );

DWORD
ADState_GetProviderData(
    IN PCSTR pszDomainName,
    OUT PAD_PROVIDER_DATA* ppProvider
    );

DWORD
ADState_StoreProviderData(
    IN PCSTR pszDomainName,
    IN PAD_PROVIDER_DATA pProvider
    );

DWORD
ADState_GetDomainTrustList(
    IN PCSTR pszDomainName,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PLW_DLINKED_LIST* ppList
    );

DWORD
ADState_StoreDomainTrustList(
    IN PCSTR pszDomainName,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    );

DWORD
ADState_AddDomainTrust(
    IN PCSTR pszDomainName,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

VOID
ADState_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PLW_DLINKED_LIST pList
    );

#endif /* __STATE_STORE_H__ */
