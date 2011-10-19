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
 *        misc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Miscellaneous Utilities
 *
 */

#include "includes.h"

VOID
LwFreeDomainInfoRequest(
    PLW_DOMAIN_INFO_REQUEST pRequestInfo
    )
{
    LwFreeDomainInfoRequestContents(pRequestInfo);

    LwFreeMemory(pRequestInfo);
}

VOID
LwFreeDomainInfoRequestContents(
    PLW_DOMAIN_INFO_REQUEST pRequestInfo
    )
{
    switch (pRequestInfo->taskType)
    {
        case LW_DOMAIN_TASK_TYPE_JOIN:

            if (pRequestInfo->args.joinArgs.pszDomainName)
            {
                LwFreeString(pRequestInfo->args.joinArgs.pszDomainName);
            }
            if (pRequestInfo->args.joinArgs.pszOU)
            {
                LwFreeString(pRequestInfo->args.joinArgs.pszOU);
            }
            if (pRequestInfo->args.joinArgs.pszPassword)
            {
                LwFreeString(pRequestInfo->args.joinArgs.pszPassword);
            }
            if (pRequestInfo->args.joinArgs.pszUsername)
            {
                LwFreeString(pRequestInfo->args.joinArgs.pszUsername);
            }

            break;

        case LW_DOMAIN_TASK_TYPE_LEAVE:

            if (pRequestInfo->args.leaveArgs.pszPassword)
            {
                LwFreeString(pRequestInfo->args.leaveArgs.pszPassword);
            }
            if (pRequestInfo->args.leaveArgs.pszUsername)
            {
                LwFreeString(pRequestInfo->args.leaveArgs.pszUsername);
            }

            break;

        default:

            break;
    }
}
