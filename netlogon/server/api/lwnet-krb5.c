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
 *        lwnet-krb5.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        KRB5 API
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

#define LWNET_KRB5_CONF_PATH_TEMP LWNET_KRB5_CONF_PATH ".temp"

static pthread_mutex_t gLWNetKrb5AffinityLock      = PTHREAD_MUTEX_INITIALIZER;

static
PCSTR
LWNetKrb5SelectAddress(
    IN OPTIONAL PCSTR pszAddress,
    IN OPTIONAL PCSTR pszName
    )
{
    PCSTR pszResult = NULL;

    if (!IsNullOrEmptyString(pszAddress))
    {
        pszResult = pszAddress;
    }
    else if (!IsNullOrEmptyString(pszName))
    {
        LWNET_LOG_ERROR("Missing DC address so using name '%s' while doing krb5 affinity.", pszName);
        pszResult = pszName;
    }
    else
    {
        LWNET_LOG_ERROR("Missing DC address and name while doing krb5 affinity.");
    }

    return pszResult;
}

static
DWORD
LWNetKrb5SetupServerAddressArray(
    OUT PCSTR** ppServerAddressArray,
    OUT PDWORD pdwServerAddressCount,
    IN PLWNET_DC_INFO pDcInfo,
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount
    )
{
    DWORD dwError = 0;
    PCSTR* pServerAddressArray = NULL;
    DWORD dwOutputIndex = 0;
    DWORD dwInputIndex = 0;

    dwError = LWNetAllocateMemory(sizeof(pServerAddressArray[0]) * (dwServerCount + 1),
                                  (PVOID*)&pServerAddressArray);
    BAIL_ON_LWNET_ERROR(dwError);

    pServerAddressArray[dwOutputIndex] = LWNetKrb5SelectAddress(pDcInfo->pszDomainControllerAddress,
                                                                pDcInfo->pszDomainControllerName);
    if (!pServerAddressArray[dwOutputIndex])
    {
        dwError = ERROR_NOT_FOUND;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    dwOutputIndex++;

    for (dwInputIndex = 0; dwInputIndex < dwServerCount; dwInputIndex++)
    {
        // Do not write out the affinitized entry twice.
        PCSTR pszAddress = LWNetKrb5SelectAddress(pServerArray[dwInputIndex].pszAddress,
                                                  pServerArray[dwInputIndex].pszName);
        if (strcasecmp(pServerAddressArray[0], pszAddress))
        {
            pServerAddressArray[dwOutputIndex++] = pszAddress;
        }
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pServerAddressArray);
        dwOutputIndex = 0;
    }

    *ppServerAddressArray = pServerAddressArray;
    *pdwServerAddressCount = dwOutputIndex;

    return dwError;
}

static
DWORD
LWNetKrb5PrintfFile(
    IN FILE* File,
    IN PCSTR pszFormat,
    IN ...
    )
{
    DWORD dwError = 0;
    int ret = 0;    
    va_list args;
    
    va_start(args, pszFormat);

    ret = vfprintf(File, pszFormat, args);
    if (ret < 0)
    {
        dwError = ERROR_WRITE_FAULT;
    }
    
    va_end(args);

    return dwError;
}

static
DWORD
LWNetKrb5WriteAffinityFile(
    IN PCSTR pszOldFileName,
    IN PCSTR pszNewFileName,
    IN PCSTR pszDnsDomainName,
    IN PCSTR* pServerAddressArray,
    IN DWORD dwServerCount
    )
{
    DWORD dwError = 0;
    FILE* newFile = NULL;
    FILE* oldFile = NULL;
    BOOLEAN bEndOfFile = FALSE;
    BOOLEAN bFindNextRealm = TRUE;
    PSTR pszLine = NULL;
    PSTR pszCompareRealm = NULL;
    PSTR pszDnsDomainNameUpper = NULL;

    pthread_mutex_lock(&gLWNetKrb5AffinityLock);

    // ISSUE-2008/07/03-dalmeida Technically, we should be setting the perms
    // on create, but whatever.
    newFile = fopen(pszNewFileName, "w");
    if (!newFile)
    { 
        dwError = ERROR_BAD_CONFIGURATION;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LwChangePermissions(pszNewFileName, LWNET_MODE_BITS_URW_GR_OR);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetKrb5PrintfFile(newFile, "[realms]\n\n");
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwServerCount > 0)
    {
        DWORD dwServerIndex = 0;

        dwError = LWNetAllocateString(
                      pszDnsDomainName,
                      &pszDnsDomainNameUpper);
        BAIL_ON_LWNET_ERROR(dwError);

        LwStrToUpper(pszDnsDomainNameUpper);

        dwError = LWNetKrb5PrintfFile(newFile, "    %s = {\n", pszDnsDomainNameUpper);
        BAIL_ON_LWNET_ERROR(dwError);

        for (dwServerIndex = 0; dwServerIndex < dwServerCount; dwServerIndex++)
        {
            PCSTR pszServerAddress = pServerAddressArray[dwServerIndex];

            if (pszServerAddress)
            {
                dwError = LWNetKrb5PrintfFile(newFile, "        kdc = %s\n", pszServerAddress);
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }

        dwError = LWNetKrb5PrintfFile(newFile, "    }\n");
        BAIL_ON_LWNET_ERROR(dwError);
    }

    oldFile = fopen(pszOldFileName, "r");
    if (!oldFile)
    { 
        // there is no original file to copy data from
        goto error;
    }

    dwError = LwAllocateStringPrintf(
                  &pszCompareRealm,
                  " %s ",
                  pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    LwStrToUpper(pszCompareRealm);

    bFindNextRealm = TRUE;

    while (!bEndOfFile)
    {
        LWNET_SAFE_FREE_STRING(pszLine);

        dwError = LWNetReadNextLine(
                      oldFile,
                      &pszLine,
                      &bEndOfFile);
        BAIL_ON_LWNET_ERROR(dwError);

        if (bEndOfFile)
        {
            break;
        }

        if (strchr(pszLine, '{') != NULL)
        {
            if (strstr(pszLine, pszCompareRealm) != NULL)
            {
                bFindNextRealm = TRUE;
                continue;
            }
            else
            {
                bFindNextRealm = FALSE;
            }
        }

        if (bFindNextRealm)
        {
            continue;
        }

        dwError = LWNetKrb5PrintfFile(newFile, "%s", pszLine);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (newFile)
    {
        fclose(newFile);
    }
    if (oldFile)
    {
        fclose(oldFile);
    }

    if (dwError)
    {
        LwRemoveFile(pszNewFileName);
    }

    pthread_mutex_unlock(&gLWNetKrb5AffinityLock);

    LWNET_SAFE_FREE_STRING(pszLine);
    LWNET_SAFE_FREE_STRING(pszCompareRealm);
    LWNET_SAFE_FREE_STRING(pszDnsDomainNameUpper);

    return dwError;
}

DWORD
LWNetKrb5UpdateAffinity(
    IN PCSTR pszDnsDomainName,
    IN PLWNET_DC_INFO pDcInfo,
    IN PDNS_SERVER_INFO pServerArray,
    IN DWORD dwServerCount
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PCSTR* pServerAddressArray = NULL;
    DWORD dwServerAddressCount = 0;

    dwError = LWNetKrb5SetupServerAddressArray(&pServerAddressArray,
                                               &dwServerAddressCount,
                                               pDcInfo,
                                               pServerArray,
                                               dwServerCount);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwCheckFileTypeExists(
                    LWNET_KRB5_CONF_DIRNAME,
                    LWFILE_DIRECTORY,
                    &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (!bExists)
    {
        dwError = LwCreateDirectory(LWNET_KRB5_CONF_DIRNAME,
                                       LWNET_MODE_BITS_URWX_GRX_ORX);
        BAIL_ON_LWNET_ERROR(dwError); 
    }

    dwError = LwCheckFileTypeExists(
            LWNET_KRB5_CONF_PATH_TEMP,
            LWFILE_REGULAR,
            &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (bExists)
    {
        dwError = LwRemoveFile(LWNET_KRB5_CONF_PATH_TEMP);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetKrb5WriteAffinityFile(
                  LWNET_KRB5_CONF_PATH,
                  LWNET_KRB5_CONF_PATH_TEMP,
                  pszDnsDomainName,
                  pServerAddressArray,
                  dwServerAddressCount);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwMoveFile(LWNET_KRB5_CONF_PATH_TEMP, LWNET_KRB5_CONF_PATH);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    LWNET_SAFE_FREE_MEMORY(pServerAddressArray);
    if (dwError)
    {
        LwRemoveFile(LWNET_KRB5_CONF_PATH_TEMP);
    }
    return dwError;
}

