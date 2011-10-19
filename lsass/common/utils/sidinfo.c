/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sidinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Object (User/Group/Other) Info by Security Identifier
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

VOID
LsaFreeSIDInfoList(
    PLSA_SID_INFO pSIDInfoList,
    size_t        stNumSID
    )
{
    size_t iSID = 0;
    for (; iSID < stNumSID; iSID++)
    {
        LW_SAFE_FREE_STRING(pSIDInfoList[iSID].pszDomainName);
        LW_SAFE_FREE_STRING(pSIDInfoList[iSID].pszSamAccountName);
    }
    LwFreeMemory(pSIDInfoList);
}

VOID
LsaFreeSIDInfo(
    PLSA_SID_INFO pSIDInfo
    )
{
    LW_SAFE_FREE_STRING(pSIDInfo->pszSamAccountName);
    LW_SAFE_FREE_STRING(pSIDInfo->pszDomainName);
    LwFreeMemory(pSIDInfo);
}

void
LsaFreeIpcNameSidsList(
    PLSA_FIND_NAMES_BY_SIDS pNameSidsList
    )
{
    if (pNameSidsList)
    {
        LsaFreeSIDInfoList(pNameSidsList->pSIDInfoList, pNameSidsList->sCount);
        LwFreeMemory(pNameSidsList);
    }
}
