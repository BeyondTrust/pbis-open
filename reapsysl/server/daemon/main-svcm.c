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
 *        main.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
RSysSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

static
VOID
RSysSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

static
NTSTATUS
RSysSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = RSysSrvInitialize();
    BAIL_ON_RSYS_ERROR(dwError);

    RSYS_LOG_VERBOSE("Logging started");

    dwError = RSysSrvStartReaderThread();
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:

    return LwWin32ErrorToNtStatus(dwError);

error:

    goto cleanup;
}

static
NTSTATUS
RSysSvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return LwWin32ErrorToNtStatus(RSysSrvRefreshConfiguration());
}

static
NTSTATUS
RSysSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    RSYS_LOG_VERBOSE("Shutting down threads");

    RSysSrvStopReaderThread();

    RSysSrvApiShutdown();

    RSYS_LOG_INFO("Reapsysl Service exiting...");

    return STATUS_SUCCESS;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = RSysSvcmInit,
    .Destroy = RSysSvcmDestroy,
    .Start = RSysSvcmStart,
    .Stop = RSysSvcmStop,
    .Refresh = RSysSvcmRefresh,
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(reapsysl)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}

DWORD
RSysSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    
    dwError = RSysSrvApiInit();
    BAIL_ON_RSYS_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}
