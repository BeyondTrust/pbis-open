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
 *        join.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
#define LSAJOIN_NO_PRE_COMPUTER_ACCOUNT              (0x00000200)

/* LDAP account flags - local copies of UF_* flags
   from NetAPI */

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
