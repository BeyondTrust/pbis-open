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
 *        main.c
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
SvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
SvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

NTSTATUS
SvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = pthread_rwlock_init(&gUmnConfigLock, NULL);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = UmnSrvInitConfig(&gpAPIConfig);
    BAIL_ON_UMN_ERROR(dwError);

    // This function creates threads, so signals must be blocked first
    dwError = UmnSrvRefreshConfiguration();
    BAIL_ON_UMN_ERROR(dwError);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    BAIL_ON_UMN_ERROR(dwError);

    dwError = UmnSrvStartPollerThread();
    BAIL_ON_UMN_ERROR(dwError);

error:
    return dwError;
}

NTSTATUS
SvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    UMN_LOG_VERBOSE("Shutting down threads");

    UmnSrvStopPollerThread();

    UmnSrvFreeConfig(gpAPIConfig);
    LW_ASSERT(pthread_rwlock_destroy(&gUmnConfigLock) == 0);

    UMN_LOG_INFO("Usermonitor Service exiting...");

    return 0;
}

NTSTATUS
SvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
    DWORD dwError = 0;

    UMN_LOG_VERBOSE("Refreshing configuration");
    dwError = UmnSrvRefreshConfiguration();

    if (dwError) {
        UMN_LOG_WARNING("Failed refreshing configuration: error %d", dwError);
    } else {
        UMN_LOG_INFO("Succeeded refreshing configuration");
    }

    return dwError;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = SvcmInit,
    .Destroy = SvcmDestroy,
    .Start = SvcmStart,
    .Stop = SvcmStop,
    .Refresh = SvcmRefresh 
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(usermonitor)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}
