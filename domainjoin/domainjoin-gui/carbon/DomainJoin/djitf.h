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

