/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lsadmengine.c
 *
 * @brief
 *
 *     LSASS Special Domain Name/SID Utilities
 *
 * @details
 *
 *     This module has some helper routines for dealing with
 *     special domain names and SIDs.
 *
 * @author Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "specialdomain.h"
#include <strings.h>

BOOLEAN
AdIsSpecialDomainName(
    IN PCSTR pszDomainName
    )
{
    return (!strcasecmp(pszDomainName, "BUILTIN") ||
            !strcasecmp(pszDomainName, "NT AUTHORITY"));
}

// Prefix for standard (not system, built-in, etc) accounts.
#define NT_NON_UNIQUE_SID_PREFIX "S-1-5-21-"

BOOLEAN
AdIsSpecialDomainSidPrefix(
    IN PCSTR pszObjectSid
    )
{
    // The NT non-unique SID prefix (S-1-5-21) is the prefix used
    // for standard accounts.
    return !strncasecmp(pszObjectSid, NT_NON_UNIQUE_SID_PREFIX, sizeof(NT_NON_UNIQUE_SID_PREFIX)-1) ? FALSE : TRUE;
}
