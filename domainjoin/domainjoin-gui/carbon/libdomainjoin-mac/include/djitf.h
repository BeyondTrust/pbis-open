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

#ifndef __DJ_ITF_H__
#define __DJ_ITF_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __DOMAIN_JOIN_INFO
{
    char* pszName;
    char* pszDnsDomain;
    char* pszDomainName;      /* Null if not joined  */
    char* pszDomainShortName; /* Null if not joined  */
    char* pszLogFilePath;     /* Null if not logging */
    char* pszOU;              /* Null if not joined  */

} DOMAIN_JOIN_INFO, *PDOMAIN_JOIN_INFO;

typedef struct __DOMAIN_JOIN_ERROR
{

    int  code;
    char* pszShortError;
    char* pszLongError;

} DOMAIN_JOIN_ERROR, *PDOMAIN_JOIN_ERROR;

typedef int (*PFNJoinDomain)(
                const char*         pszDomainName,
                const char*         pszOU,
                const char*         pszUsername,
                const char*         pszPassword,
                const char*         pszUserDomainPrefix,
                short               bAssumeDefaultDomain,
                short               bNoHosts,
                PDOMAIN_JOIN_ERROR* ppError
                );

typedef int (*PFNLeaveDomain)(
                const char*         pszUsername,
                const char*         pszPassword,
                PDOMAIN_JOIN_ERROR* ppError
                );

typedef int (*PFNSetComputerName)(
                const char*         pszComputerName,
                const char*         pszDomainName,
                PDOMAIN_JOIN_ERROR* ppError
                );

typedef int (*PFNQueryInformation)(
                PDOMAIN_JOIN_INFO*  ppDomainJoinInfo,
                PDOMAIN_JOIN_ERROR* ppError
                );

typedef int (*PFNIsDomainNameResolvable)(
                const char*         pszDomainName,
                short*              pbIsResolvable,
                PDOMAIN_JOIN_ERROR* ppszError
                ); 

typedef void (*PFNFreeDomainJoinInfo)(
                PDOMAIN_JOIN_INFO pDomainJoinInfo
                );

typedef void (*PFNFreeDomainJoinError)(
                PDOMAIN_JOIN_ERROR pError
                );

#ifdef __cplusplus
}
#endif

typedef struct __DJ_API_FUNCTION_TABLE
{
    PFNJoinDomain             pfnJoinDomain;
    PFNLeaveDomain            pfnLeaveDomain;
    PFNSetComputerName        pfnSetComputerName;
    PFNQueryInformation       pfnQueryInformation;
    PFNIsDomainNameResolvable pfnIsDomainNameResolvable;
    PFNFreeDomainJoinInfo     pfnFreeDomainJoinInfo;
    PFNFreeDomainJoinError    pfnFreeDomainJoinError;

} DJ_API_FUNCTION_TABLE, *PDJ_API_FUNCTION_TABLE;

#define DJ_INITIALIZE_JOIN_INTERFACE "DJInitJoinInterface"

typedef int (*PFNInitJoinInterface)(
                 PDJ_API_FUNCTION_TABLE* ppFuncTable
                 );

int
DJInitJoinInterface(
    PDJ_API_FUNCTION_TABLE* ppFuncTable
    );

#define DJ_SHUTDOWN_JOIN_INTERFACE   "DJShutdownJoinInterface"

typedef void (*PFNShutdownJoinInterface)(
                PDJ_API_FUNCTION_TABLE pFuncTable
                );

void
DJShutdownJoinInterface(
    PDJ_API_FUNCTION_TABLE pFuncTable
    );

#endif /* __DJ_ITF_H__ */

