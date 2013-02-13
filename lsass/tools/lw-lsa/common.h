/*
 * Copyright Likewise Software
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
