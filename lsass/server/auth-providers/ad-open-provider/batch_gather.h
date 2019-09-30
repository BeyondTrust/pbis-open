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
 *        batch_gather.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        AD LDAP User/Group information gathering functions
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#ifndef _BATCH_GATHER_H_
#define _BATCH_GATHER_H_

#include "batch_common.h"

DWORD
LsaAdBatchGatherRpcObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT PSTR* ppszSid,
    IN OUT PSTR* ppszSamAccountName
    );

DWORD
LsaAdBatchGatherRealObjectInternal(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN OPTIONAL PDWORD pdwDirectoryMode,
    IN OPTIONAL ADConfigurationMode* pAdMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT OPTIONAL PSTR* ppszSid,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

DWORD
LsaAdBatchGatherRealObject(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT OPTIONAL PSTR* ppszSid,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

DWORD
LsaAdBatchGatherPseudoObjectDefaultSchema(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT OPTIONAL PSTR* ppszSid,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

DWORD
LsaAdBatchGatherPseudoObject(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

DWORD
LsaAdBatchGatherPseudoObjectSidFromGc(
    IN PAD_PROVIDER_DATA pProviderData,
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

#endif /* _BATCH_GATHER_H_ */
