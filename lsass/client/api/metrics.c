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
 *        groups.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Performance Metrics API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaGetMetrics(
    HANDLE  hLsaConnection,
    DWORD   dwInfoLevel,
    PVOID*  ppMetricPack
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hLsaConnection;

    PLSA_METRIC_PACK pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    request.tag = LSA_Q_GET_METRICS;
    request.object = &dwInfoLevel;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GET_METRICS_SUCCESS:
            pResult = (PLSA_METRIC_PACK)response.object;
            switch (pResult->dwInfoLevel)
            {
                case 0:
                    *ppMetricPack = (PVOID*)pResult->pMetricPack.pMetricPack0;
                    pResult->pMetricPack.pMetricPack0 = NULL;
                    break;
                case 1:
                    *ppMetricPack = (PVOID*)pResult->pMetricPack.pMetricPack1;
                    pResult->pMetricPack.pMetricPack1 = NULL;
                    break;
                default:
                   dwError = LW_ERROR_INVALID_PARAMETER;
                   BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_GET_METRICS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    *ppMetricPack  = NULL;

    goto cleanup;
}


