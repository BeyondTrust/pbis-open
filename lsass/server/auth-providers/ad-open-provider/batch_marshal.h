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
 *        batch_marshal.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
