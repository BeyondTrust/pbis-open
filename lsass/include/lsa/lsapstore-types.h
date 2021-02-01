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
 *        lsapstore-types.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Password Store Types
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LSA_PSTORE_TYPES_H__
#define __LSA_PSTORE_TYPES_H__

#include <lw/types.h>

typedef LW_DWORD LSA_MACHINE_ACCOUNT_FLAGS, *PLSA_MACHINE_ACCOUNT_FLAGS;

#define LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION 1 // Schannel Type 2
#define LSA_MACHINE_ACCOUNT_TYPE_DC          2 // Schannel Type 4
#define LSA_MACHINE_ACCOUNT_TYPE_BDC         3 // Schannel Type 6

// Reserve extra bits for potential future expansion
#define _LSA_MACHINE_ACCOUNT_TYPE_MASK  ((LSA_MACHINE_ACCOUNT_FLAGS)0xFF)

#define LSA_GET_MACHINE_ACCOUNT_TYPE(Flags) \
    ((Flags) & _LSA_MACHINE_ACCOUNT_TYPE_MASK)

static
inline
LW_BOOLEAN
LSA_IS_VALID_MACHINE_ACCOUNT_FLAGS(
    LSA_MACHINE_ACCOUNT_FLAGS AccountFlags
    )
{
    switch (LSA_GET_MACHINE_ACCOUNT_TYPE(AccountFlags))
    {
        case LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION:
        case LSA_MACHINE_ACCOUNT_TYPE_DC:
        case LSA_MACHINE_ACCOUNT_TYPE_BDC:
            break;
        default:
            return LW_FALSE;
    }
    // No non-type flags currently defined
    if (AccountFlags & ~_LSA_MACHINE_ACCOUNT_TYPE_MASK)
    {
        return LW_FALSE;
    }
    return LW_TRUE;
}

typedef struct _LSA_MACHINE_ACCOUNT_INFO_W {
    // Basic Information
    LW_PWSTR DnsDomainName;
    LW_PWSTR NetbiosDomainName;
    LW_PWSTR DomainSid;
    LW_PWSTR SamAccountName;
    LSA_MACHINE_ACCOUNT_FLAGS AccountFlags;
    LW_DWORD KeyVersionNumber;
    LW_PWSTR Fqdn;
    // Windows Time Format
    LW_LONG64 LastChangeTime;
} LSA_MACHINE_ACCOUNT_INFO_W, *PLSA_MACHINE_ACCOUNT_INFO_W;

typedef struct _LSA_MACHINE_PASSWORD_INFO_W {
    LSA_MACHINE_ACCOUNT_INFO_W Account;
    LW_PWSTR Password;
} LSA_MACHINE_PASSWORD_INFO_W, *PLSA_MACHINE_PASSWORD_INFO_W;

typedef struct _LSA_MACHINE_ACCOUNT_INFO_A {
    // Basic Information
    LW_PSTR DnsDomainName;
    LW_PSTR NetbiosDomainName;
    LW_PSTR DomainSid;
    LW_PSTR SamAccountName;
    LSA_MACHINE_ACCOUNT_FLAGS AccountFlags;
    LW_DWORD KeyVersionNumber;
    LW_PSTR Fqdn;
    // Windows Time Format
    LW_LONG64 LastChangeTime;
} LSA_MACHINE_ACCOUNT_INFO_A, *PLSA_MACHINE_ACCOUNT_INFO_A;

typedef struct _LSA_MACHINE_PASSWORD_INFO_A {
    LSA_MACHINE_ACCOUNT_INFO_A Account;
    LW_PSTR Password;
} LSA_MACHINE_PASSWORD_INFO_A, *PLSA_MACHINE_PASSWORD_INFO_A;

#endif /* __LSA_PSTORE_TYPES_H__ */
