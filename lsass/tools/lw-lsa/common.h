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
 * Module Name:
 *
 *        common.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Common functions for tools
 *
 * Authors: Brian Koropoff(bkoropoff@likewise.com)
 */

#ifndef __TOOLS_COMMON_H__
#define __TOOLS_COMMON_H__

#include <lsa/lsa.h>
#include "lwargvcursor.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

#define PB_MODE_LSA  0x00001
#define PB_MODE_PBIS 0x00002

VOID
PrintSecurityObject(
    PLSA_SECURITY_OBJECT pObject,
    DWORD dwObjectNumber,
    DWORD dwObjectTotal,
    BOOLEAN bPBOutputMode
    );

PCSTR
Basename(
    PCSTR pszPath
    );

VOID
PrintErrorMessage(
    IN DWORD ErrorCode
    );

BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

#endif
