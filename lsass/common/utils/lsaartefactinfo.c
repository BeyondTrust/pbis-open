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
 *        lsaartefactinfo.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
