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
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 *        Private Header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __JOIN_P_H__
#define __JOIN_P_H__

#include <lwio/lwio.h>

/* (un)join domain flags - local copies of NETSETUP_* flags
   from NetAPI */
#define LSAJOIN_JOIN_DOMAIN                          (0x00000001)
#define LSAJOIN_ACCT_CREATE                          (0x00000002)
#define LSAJOIN_ACCT_DELETE                          (0x00000004)
#define LSAJOIN_WIN9X_UPGRADE                        (0x00000010)
#define LSAJOIN_DOMAIN_JOIN_IF_JOINED                (0x00000020)
#define LSAJOIN_JOIN_UNSECURE                        (0x00000040)
#define LSAJOIN_MACHINE_PWD_PASSED                   (0x00000080)
#define LSAJOIN_DEFER_SPN_SET                        (0x00000100)

/* LDAP account flags - local copies of UF_* flags
   from NetAPI */
#define LSAJOIN_ACCOUNTDISABLE                       (0x00000002)
#define LSAJOIN_WORKSTATION_TRUST_ACCOUNT            (0x00001000)

DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    );

DWORD
LsaGetRwDcName(
    PCWSTR    pwszDnsDomainName,
    BOOLEAN   bForce,
    PWSTR    *ppwszDomainControllerName
    );

DWORD
LsaChangeDomainGroupMembership(
    IN  PCSTR    pszDomainName,
    IN  PCSTR    pszDomainSID,
    IN  BOOLEAN  bEnable
    );

#endif /* __JOIN_P_H__ */
