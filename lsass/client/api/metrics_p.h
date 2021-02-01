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
 *        groups_p.h
 *
 * Abstract:
 * 
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Private Header (Library)
 * 
 *        Metric Lookup and Management API (Client)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __ARTEFACTS_P_H__
#define __ARTEFACTS_P_H__

typedef struct __LSA_ENUM_PERF_METRICS_INFO
{
    DWORD dwMetricInfoLevel;
    DWORD dwNumMaxMetrics;
    PSTR  pszGUID;
} LSA_ENUM_PERF_METRICS_INFO, *PLSA_ENUM_PERF_METRICS_INFO;

LSASS_API
DWORD
LsaBeginEnumMetrics(
    HANDLE  hLsaConnection,
    DWORD   dwMetricInfoLevel,
    DWORD   dwMaxNumMetrics,
    PHANDLE phResume
    );

LSASS_API
DWORD
LsaEnumMetrics(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumMetricsFound,
    PVOID** pppMetricInfoList
    );

LSASS_API
DWORD
LsaEndEnumMetrics(
    HANDLE hLsaConnection,
    HANDLE hResume
    );

DWORD
LsaValidateMetricInfoLevel(
    DWORD dwMetricInfoLevel
    );

VOID
LsaFreeEnumMetricsInfo(
    PLSA_ENUM_PERF_METRICS_INFO pInfo
    );

#endif /* __PERF_METRICS_P_H__ */

