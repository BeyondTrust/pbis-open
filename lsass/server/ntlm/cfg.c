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
 *        cfg.c
 *
 * Abstract:
 *
 *        NTLM Security registry settings
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */

#include "ntlmsrvapi.h"
#include "lsasrvutils.h"

DWORD
NtlmReadRegistry(
    OUT PNTLM_CONFIG pConfig
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    pConfig->bSendNTLMv2 = FALSE;
    pConfig->bSupportUnicode = TRUE;
    pConfig->bSupportNTLM2SessionSecurity = TRUE;
    pConfig->bSupportKeyExchange = TRUE;
    pConfig->bSupport56bit = TRUE;
    pConfig->bSupport128bit = TRUE;

    LWREG_CONFIG_ITEM configItems[] =
    {
        {
            "SendNTLMv2",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSendNTLMv2,
            NULL
        },
        {
            "SupportUnicode",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSupportUnicode,
            NULL
        },
        {
            "SupportNTLM2SessionSecurity",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSupportNTLM2SessionSecurity,
            NULL
        },
        {
            "SupportKeyExchange",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSupportKeyExchange,
            NULL
        },
        {
            "Support56bit",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSupport56bit,
            NULL
        },
        {
            "Support128bit",
            TRUE,
            LwRegTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &pConfig->bSupport128bit,
            NULL
        },
    };

    dwError = RegProcessConfig(
                "Services\\lsass\\Parameters\\NTLM",
                "Policy\\Services\\lsass\\Parameters\\NTLM",
                configItems,
                sizeof(configItems)/sizeof(configItems[0]));
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}
