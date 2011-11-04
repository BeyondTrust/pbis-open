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
 *        lsaartefactinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NSSArtefact Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"

VOID
LsaFreeNSSArtefactInfoList(
    DWORD  dwLevel,
    PVOID* pNSSArtefactInfoList,
    DWORD  dwNumNSSArtefacts
    )
{
    DWORD iNSSArtefact = 0;
    for (;iNSSArtefact < dwNumNSSArtefacts; iNSSArtefact++) {
        PVOID pNSSArtefactInfo = *(pNSSArtefactInfoList+iNSSArtefact);
        if (pNSSArtefactInfo) {
           LsaFreeNSSArtefactInfo(dwLevel, pNSSArtefactInfo);
        }
    }
    LwFreeMemory(pNSSArtefactInfoList);
}

VOID
LsaFreeNSSArtefactInfo(
    DWORD  dwLevel,
    PVOID  pNSSArtefactInfo
    )
{
    switch(dwLevel)
    {
        case 0:
        {
            LsaFreeNSSArtefactInfo_0((PLSA_NSS_ARTEFACT_INFO_0)pNSSArtefactInfo);
            break;
        }
        default:
        {
            LSA_LOG_ERROR("Unsupported NSSArtefact Info Level [%u]", dwLevel);
        }
    }
}

VOID
LsaFreeNSSArtefactInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo
    )
{
    LW_SAFE_FREE_STRING(pNSSArtefactInfo->pszName);
    LW_SAFE_FREE_STRING(pNSSArtefactInfo->pszValue);
    LwFreeMemory(pNSSArtefactInfo);
}

void
LsaFreeIpcNssArtefactInfoList(
    PLSA_NSS_ARTEFACT_INFO_LIST pNssArtefactIpcInfoList
    )
{
    if (pNssArtefactIpcInfoList)
    {
        switch (pNssArtefactIpcInfoList->dwNssArtefactInfoLevel)
        {
            case 0:
                LsaFreeNSSArtefactInfoList(0, (PVOID*)pNssArtefactIpcInfoList->ppNssArtefactInfoList.ppInfoList0, pNssArtefactIpcInfoList->dwNumNssArtefacts);
                break;

            default:
            {
                LSA_LOG_ERROR("Unsupported Nss Artefact Info Level [%u]", pNssArtefactIpcInfoList->dwNssArtefactInfoLevel);
            }
        }
        LwFreeMemory(pNssArtefactIpcInfoList);
    }
}
