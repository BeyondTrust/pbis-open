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
 *        session.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Login Session API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvOpenSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    BAIL_ON_INVALID_STRING(pszLoginId);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnOpenSession(
                                        hProvider,
                                        pszLoginId);
        if (!dwError) {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginSuccessEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId,
                    NULL,
                    0,
                    LSASS_EVENT_LOGON_PHASE_CREATE_SESSION,
                    dwError);
            }
           break;

        } else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_USER)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;

        } else {
            if (LsaSrvEventlogEnabled())
            {
                LsaSrvWriteLoginFailedEvent(
                    hServer,
                    pProvider->pszName,
                    pszLoginId,
                    NULL,
                    0,
                    LSASS_EVENT_LOGON_PHASE_CREATE_SESSION,
                    dwError);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulOpenSession);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedOpenSession);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "open session for user (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));

    goto cleanup;
}

DWORD
LsaSrvCloseSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_AUTHENTICATION};
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(
                      hServer,
                      pProvider,
                      NULL,
                      &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnCloseSession(
                                hProvider,
                                pszLoginId);
        if (!dwError) {

            if (LsaSrvEventlogEnabled())
            {
                    LsaSrvWriteLogoutSuccessEvent(hServer,
                                                  pProvider->pszName,
                                                  LSASS_EVENT_LOGON_PHASE_CREATE_SESSION,
                                                  pszLoginId);
            }

           break;

        } else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_USER)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;

        } else {

            BAIL_ON_LSA_ERROR(dwError);

        }
    }

cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulCloseSession);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedCloseSession);
    }

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:
    LSA_LOG_ERROR_API_FAILED(hServer, dwError, "close session for user (name = '%s')", LSA_SAFE_LOG_STRING(pszLoginId));

    goto cleanup;
}
