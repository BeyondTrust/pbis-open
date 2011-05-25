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

#include "domainjoin.h"
#include "djitf.h"
#include "djitf_p.h"
#include "externs.h"

int
DJInitJoinInterface(
    PDJ_API_FUNCTION_TABLE* ppFuncTable
    )
{
    PSTR pszLogFilePath = "/tmp/lwidentity.join.log";
    DWORD ceError = 0;

    ceError = dj_init_logging_to_file(LOG_LEVEL_VERBOSE, pszLogFilePath);

    *ppFuncTable = gpDJApiFunctionTable;

    return ceError;
}

void
DJShutdownJoinInterface(
    PDJ_API_FUNCTION_TABLE pFuncTable
    )
{
    dj_close_log();
}

int
DJItfJoinDomain(
    const char*         pszDomainName,
    const char*         pszOU,
    const char*         pszUsername,
    const char*         pszPassword,
    const char*         pszUserDomainPrefix,
    short               bAssumeDefaultDomain,
    short               bNoHosts,
    PDOMAIN_JOIN_ERROR* ppError
    )
{
    DWORD ceError = ERROR_SUCCESS;
    JoinProcessOptions options;
    DynamicArray enableModules, disableModules;
    DynamicArray detailModules;
    LWException* pException = NULL;
    size_t i;
    
    DJZeroJoinProcessOptions(&options);
    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&detailModules, 0, sizeof(detailModules));
    
    options.joiningDomain = TRUE;
    
    if (bNoHosts) {
    
        PCSTR pszModule = "hostname";
        
        LW_CLEANUP_CTERR(&pException, CTArrayAppend(&disableModules, sizeof(PCSTR), &pszModule, 1));
        
    }
        
    if (!IsNullOrEmptyString(pszDomainName)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszDomainName, &options.domainName));
    }
    
    if (!IsNullOrEmptyString(pszOU)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszOU, &options.ouName));
    }
    
    if (!IsNullOrEmptyString(pszUsername)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszUsername, &options.username));
    }
    
    if (!IsNullOrEmptyString(pszPassword)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszPassword, &options.password));
    }

    // Need to set the value per the UI setting
    options.setAssumeDefaultDomain = TRUE;

    if (bAssumeDefaultDomain)
    {
        options.assumeDefaultDomain = TRUE;
        if (pszUserDomainPrefix)
        {
            LW_CLEANUP_CTERR(&pException, CTStrdup(pszUserDomainPrefix, &options.userDomainPrefix));
        }
    }

    LW_CLEANUP_CTERR(&pException, DJGetComputerName(&options.computerName));

    LW_TRY(&pException, DJInitModuleStates(&options, &LW_EXC));

    for(i = 0; i < enableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &enableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(&pException,
						ERROR_INVALID_PARAMETER,
						"Module already specified",
						"The module '%s' is listed as being disabled and enabled",
						module);
            goto cleanup;
        }
        LW_TRY(&pException, DJEnableModule(&options, module, TRUE, &LW_EXC));
    }

    for(i = 0; i < disableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &disableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&enableModules, module) != -1)
        {
            LW_RAISE_EX(&pException,
						ERROR_INVALID_PARAMETER,
						"Module already specified",
						"The module '%s' is listed as being disabled and enabled",
						module);
            goto cleanup;
        }
        LW_TRY(&pException, DJEnableModule(&options, module, FALSE, &LW_EXC));
    }

    LW_TRY(&pException, DJCheckRequiredEnabled(&options, &LW_EXC));
    
    LW_TRY(&pException, DJRunJoinProcess(&options, &LW_EXC));
    
cleanup:

    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&detailModules);
    
    if (pException) {
    
       PDOMAIN_JOIN_ERROR pError = NULL;
       DWORD ceError_2 = ERROR_SUCCESS;
       
       ceError_2 = DJItfBuildDomainJoinError(pException, &pError);
       if (!ceError_2) {
          *ppError = pError;
       }
       
       ceError = pException->code;
       
    } else {
    
       *ppError = NULL;
       
    }

    return ceError;
}

int
DJItfLeaveDomain(
    const char*         pszUsername,
    const char*         pszPassword,
    PDOMAIN_JOIN_ERROR* ppError
    )
{
    DWORD ceError = ERROR_SUCCESS;
    LWException* pException = NULL;
    JoinProcessOptions options;
    DynamicArray enableModules, disableModules;
    DynamicArray detailModules;
    size_t i;
    
    DJZeroJoinProcessOptions(&options);
    memset(&enableModules, 0, sizeof(enableModules));
    memset(&disableModules, 0, sizeof(disableModules));
    memset(&detailModules, 0, sizeof(detailModules));
    
    options.joiningDomain = FALSE;
    
    if (!IsNullOrEmptyString(pszUsername)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszUsername, &options.username));
    }
    
    if (!IsNullOrEmptyString(pszPassword)) {
        LW_CLEANUP_CTERR(&pException, CTStrdup(pszPassword, &options.password));
    }

    LW_CLEANUP_CTERR(&pException, DJGetComputerName(&options.computerName));

    LW_TRY(&pException, DJInitModuleStates(&options, &LW_EXC));

    for(i = 0; i < enableModules.size; i++)
    {
        PCSTR module = *(PCSTR *)CTArrayGetItem(
                    &enableModules, i, sizeof(PCSTR));
        if(CTArrayFindString(&disableModules, module) != -1)
        {
            LW_RAISE_EX(&pException,
						ERROR_INVALID_PARAMETER,
						"Module already specified",
						"The module '%s' is listed as being disabled and enabled",
						module);
            goto cleanup;
        }
        LW_TRY(&pException, DJEnableModule(&options, module, TRUE, &LW_EXC));
    }
    
    LW_TRY(&pException, DJCheckRequiredEnabled(&options, &LW_EXC));
    
    LW_TRY(&pException, DJRunJoinProcess(&options, &LW_EXC));

cleanup:

    DJFreeJoinProcessOptions(&options);
    CTArrayFree(&enableModules);
    CTArrayFree(&disableModules);
    CTArrayFree(&detailModules);

    if (pException) {
    
       PDOMAIN_JOIN_ERROR pError = NULL;
       DWORD ceError_2 = ERROR_SUCCESS;
       
       ceError_2 = DJItfBuildDomainJoinError(pException, &pError);
       if (!ceError_2) {
          *ppError = pError;
       }
       
       ceError = pException->code;
       
    } else {
    
       *ppError = NULL;
       
    }

    return ceError;
}

int
DJItfSetComputerName(
    const char*         pszComputerName,
    const char*         pszDomainName,
    PDOMAIN_JOIN_ERROR* ppError
    )
{
    DWORD ceError = ERROR_SUCCESS;
    LWException* pException = NULL;
    
    LW_TRY(&pException, DJSetComputerName(pszComputerName, pszDomainName, &LW_EXC));
    
cleanup:

    if (pException) {
    
       PDOMAIN_JOIN_ERROR pError = NULL;
       DWORD ceError_2 = ERROR_SUCCESS;
       
       ceError_2 = DJItfBuildDomainJoinError(pException, &pError);
       if (!ceError_2) {
          *ppError = pError;
       }
       
       ceError = pException->code;
       
    } else {
    
       *ppError = NULL;
       
    }
    
    return ceError;
}

int
DJItfQueryInformation(
    PDOMAIN_JOIN_INFO*  ppDomainJoinInfo,
    PDOMAIN_JOIN_ERROR* ppError
    )
{
    DWORD ceError = ERROR_SUCCESS;
    LWException* pException = NULL;
    PDOMAINJOININFO pJoinInfo = NULL;
    PDOMAIN_JOIN_INFO pJoinInfo_client = NULL;
    char* pszOU = NULL;
    
    LW_TRY(&pException, QueryInformation(&pJoinInfo, &LW_EXC));
    
    ceError = DJItfConvertDomainJoinInfo(pJoinInfo, &pJoinInfo_client);
    GOTO_CLEANUP_ON_DWORD(ceError);

    if (pJoinInfo_client->pszDomainName)
    {
        LW_TRY(&pException, DJGetComputerDN(&pszOU, &LW_EXC));
        pJoinInfo_client->pszOU = pszOU;
        pszOU = NULL;
    }
    
    *ppDomainJoinInfo = pJoinInfo_client;
    pJoinInfo_client = NULL;
    
cleanup:

    if (pJoinInfo) {
        FreeDomainJoinInfo(pJoinInfo);
    }
    
    if (pJoinInfo_client) {
        DJItfFreeDomainJoinInfo(pJoinInfo_client);
    }

    if (pszOU) {
        CT_SAFE_FREE_STRING(pszOU);
    }

    if (pException) {
    
       PDOMAIN_JOIN_ERROR pError = NULL;
       DWORD ceError_2 = ERROR_SUCCESS;
       
       ceError_2 = DJItfBuildDomainJoinError(pException, &pError);
       if (!ceError_2) {
          *ppError = pError;
       }
       
       ceError = pException->code;
       
    } else {
    
       *ppError = NULL;
       
    }

    return ceError;
}

int
DJItfIsDomainNameResolvable(
    const char*         pszDomainName,
    short*              pbIsResolvable,
    PDOMAIN_JOIN_ERROR* ppError
    ) 
{
    DWORD ceError = ERROR_SUCCESS;
    LWException* pException = NULL;
    BOOLEAN bResolvable = FALSE;
    
    LW_CLEANUP_CTERR(&pException, DJIsDomainNameResolvable(pszDomainName, &bResolvable));
    
    *pbIsResolvable = (bResolvable ? TRUE : FALSE);
    
cleanup:

    if (pException) {
    
       PDOMAIN_JOIN_ERROR pError = NULL;
       DWORD ceError_2 = ERROR_SUCCESS;
       
       ceError_2 = DJItfBuildDomainJoinError(pException, &pError);
       if (!ceError_2) {
          *ppError = pError;
       }
       
       ceError = pException->code;
       
    } else {
    
       *ppError = NULL;
       
    }
    
    if (ceError) {
       *pbIsResolvable = FALSE;
    }

    return ceError;
}

void
DJItfFreeDomainJoinInfo(
    PDOMAIN_JOIN_INFO pDomainJoinInfo
    )
{
    if (pDomainJoinInfo) {
    
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszName);
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszDnsDomain);
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszDomainName);
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszDomainShortName);
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszLogFilePath);
       CT_SAFE_FREE_STRING(pDomainJoinInfo->pszOU);
    }
}

void
DJItfFreeDomainJoinError(
    PDOMAIN_JOIN_ERROR pError
    )
{
    if (pError) {
       CT_SAFE_FREE_STRING(pError->pszShortError);
       CT_SAFE_FREE_STRING(pError->pszLongError);
       CTFreeMemory(pError);
    }
}

int
DJItfConvertDomainJoinInfo(
    PDOMAINJOININFO    pJoinInfo,
    PDOMAIN_JOIN_INFO* ppJoinInfo
    )
{
    DWORD ctError = ERROR_SUCCESS;
    PDOMAIN_JOIN_INFO pNewInfo = NULL;
    
    ctError = CTAllocateMemory(
                    sizeof(DOMAIN_JOIN_INFO),
                    (PVOID*)&pNewInfo);
    GOTO_CLEANUP_ON_DWORD(ctError);
    
    if (!IsNullOrEmptyString(pJoinInfo->pszName)) {
        ctError = CTAllocateString(
                        pJoinInfo->pszName,
                        &pNewInfo->pszName);
        GOTO_CLEANUP_ON_DWORD(ctError);
    }

    if (!IsNullOrEmptyString(pJoinInfo->pszDnsDomain)) {
        ctError = CTAllocateString(
                        pJoinInfo->pszDnsDomain,
                        &pNewInfo->pszDnsDomain
                        );
        GOTO_CLEANUP_ON_DWORD(ctError);
    }
    
    if (!IsNullOrEmptyString(pJoinInfo->pszDomainName)) {
        ctError = CTAllocateString(
                        pJoinInfo->pszDomainName,
                        &pNewInfo->pszDomainName
                        );
        GOTO_CLEANUP_ON_DWORD(ctError);
    }
    
    if (!IsNullOrEmptyString(pJoinInfo->pszDomainShortName)) {
        ctError = CTAllocateString(
                        pJoinInfo->pszDomainShortName,
                        &pNewInfo->pszDomainShortName);
        GOTO_CLEANUP_ON_DWORD(ctError);
    }

    if (!IsNullOrEmptyString(pJoinInfo->pszLogFilePath)) {
        ctError = CTAllocateString(
                        pJoinInfo->pszLogFilePath,
                        &pNewInfo->pszLogFilePath);
        GOTO_CLEANUP_ON_DWORD(ctError);
    }
    
    *ppJoinInfo = pNewInfo;
    pNewInfo = NULL;
    
cleanup:

    if (pNewInfo) {
        DJItfFreeDomainJoinInfo(pNewInfo);
    }

    return ctError;
}

int
DJItfBuildDomainJoinError(
    LWException* pException,
    PDOMAIN_JOIN_ERROR* ppError
    )
{
    DWORD ctError = ERROR_SUCCESS;
    PDOMAIN_JOIN_ERROR pError = NULL;
    PSTR pszShortErrorMsg = NULL;
    PSTR pszLongErrorMsg = NULL;
    
    ctError = CTAllocateMemory(
                    sizeof(DOMAIN_JOIN_ERROR),
                    (PVOID*)&pError);
    GOTO_CLEANUP_ON_DWORD(ctError);
    
    pError->code = pException->code;
    
    if (!IsNullOrEmptyString(pException->shortMsg)) {
       ctError = CTAllocateString(
                        pException->shortMsg,
                        &pszShortErrorMsg);
       GOTO_CLEANUP_ON_DWORD(ctError);
    }
    
    if (!IsNullOrEmptyString(pException->longMsg)) {
       ctError = CTAllocateString(
                        pException->longMsg,
                        &pszLongErrorMsg);
       GOTO_CLEANUP_ON_DWORD(ctError);
    }
    
    pError->pszShortError = pszShortErrorMsg;
    pszShortErrorMsg = NULL;
    
    pError->pszLongError = pszLongErrorMsg;
    pszLongErrorMsg = NULL;
    
    *ppError = pError;
    pError = NULL;
    
cleanup:

    CT_SAFE_FREE_STRING(pszShortErrorMsg);
    CT_SAFE_FREE_STRING(pszLongErrorMsg);
    
    if (pError) {
        DJItfFreeDomainJoinError(pError);
    }

    return ctError;
}
