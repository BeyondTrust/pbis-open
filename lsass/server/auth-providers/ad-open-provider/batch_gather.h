/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        batch_gather.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
