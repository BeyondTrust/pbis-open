/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "includes.h"

DWORD
EventlogConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

static
DWORD
EVTParseDiskUsage(
    PCSTR  pszDiskUsage,
    PDWORD pdwDiskUsage
    );

static
DWORD
EVTParseMaxEntries(
    PCSTR  pszMaxEntries,
    PDWORD pdwMaxEntries
    );

static
DWORD
EVTParseDays(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

static
DWORD
EVTConfigPrint(
    FILE *fp,
    PEVTSERVERINFO pConfig
    );

static
DWORD
EVTSetConfigDefaults(
    PEVTSERVERINFO pConfig
    )
{
    DWORD dwError = 0;

    pConfig->dwMaxLogSize = EVT_DEFAULT_MAX_LOG_SIZE;
    pConfig->dwMaxRecords =  EVT_DEFAULT_MAX_RECORDS;
    pConfig->dwMaxAge = EVT_DEFAULT_MAX_AGE;
    pConfig->dwPurgeInterval = EVT_DEFAULT_PURGE_INTERVAL;

    pConfig->pszAllowReadTo = NULL;
    pConfig->pszAllowWriteTo = NULL;
    pConfig->pszAllowDeleteTo = NULL;

    return dwError;
}

static
VOID
EVTConfigFreeContents(
    PEVTSERVERINFO pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszAllowReadTo);
    LW_SAFE_FREE_STRING(pConfig->pszAllowWriteTo);
    LW_SAFE_FREE_STRING(pConfig->pszAllowDeleteTo);
}

/* call back functions to get the values from config file */
static
DWORD
EVTConfigSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
EVTConfigNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PEVTSERVERINFO pConfig = (PEVTSERVERINFO)pData;

    if (!strcmp(pszName, "max-disk-usage"))
    {
        DWORD dwDiskUsage = 0;
        EVTParseDiskUsage(pszValue, &dwDiskUsage);
        pConfig->dwMaxLogSize = dwDiskUsage;
    }
    else if (!strcmp(pszName, "max-num-events"))
    {
        DWORD dwMaxEntries = 0;
        EVTParseMaxEntries(pszValue, &dwMaxEntries);
        pConfig->dwMaxRecords = dwMaxEntries;
    }
    else if (!strcmp(pszName, "max-event-lifespan"))
    {
        DWORD dwMaxLifeSpan = 0;
        EVTParseDays(pszValue, &dwMaxLifeSpan);
        pConfig->dwMaxAge = dwMaxLifeSpan;
    }
    else if (!strcmp(pszName, "event-db-purge-interval"))
    {
        DWORD dwPurgeInterval = 0;
        EVTParseDays(pszValue, &dwPurgeInterval);
        pConfig->dwPurgeInterval = dwPurgeInterval;
    }
    else if (!strcmp(pszName, "remove-events-as-needed"))
    {
        BOOLEAN bRemoveAsNeeded = FALSE;
        if(!strcmp(pszValue,"true")) {
            bRemoveAsNeeded = TRUE;
        }
        pConfig->bRemoveAsNeeded = bRemoveAsNeeded;
    }
    else if (!strcmp(pszName, "allow-read-to"))
    {
        LW_SAFE_FREE_STRING(pConfig->pszAllowReadTo);
        dwError = LwAllocateString(pszValue, &pConfig->pszAllowReadTo);
        BAIL_ON_UP_ERROR(dwError);
    }
    else if (!strcmp(pszName, "allow-write-to"))
    {
        LW_SAFE_FREE_STRING(pConfig->pszAllowWriteTo);
        dwError = LwAllocateString(pszValue, &pConfig->pszAllowWriteTo);
        BAIL_ON_UP_ERROR(dwError);
    }
    else if (!strcmp(pszName, "allow-delete-to"))
    {
        LW_SAFE_FREE_STRING(pConfig->pszAllowDeleteTo);
        dwError = LwAllocateString(pszValue, &pConfig->pszAllowDeleteTo);
        BAIL_ON_UP_ERROR(dwError);
    }

    *pbContinue = TRUE;

error:

    return dwError;
}


DWORD
EventlogConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;
    EVTSERVERINFO Config;
    FILE *fp = NULL;

    memset(&Config, 0, sizeof(EVTSERVERINFO));

    fp = fopen(pszRegFile, "w");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = EVTSetConfigDefaults(&Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpParseConfigFile(
                pszConfFile,
                &EVTConfigSectionHandler,
                &EVTConfigNameValuePair,
                &Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = EVTConfigPrint(fp, &Config);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    EVTConfigFreeContents(&Config);

    return dwError;

error:

    goto cleanup;

}

static
DWORD
EVTConfigPrint(
    FILE *fp,
    PEVTSERVERINFO pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\eventlog]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\eventlog\\Parameters]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "AllowReadTo", pConfig->pszAllowReadTo);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "AllowWriteTo", pConfig->pszAllowWriteTo);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "AllowDeleteTo", pConfig->pszAllowDeleteTo);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "MaxDiskUsage", pConfig->dwMaxLogSize);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "MaxEventLifespan", pConfig->dwMaxAge);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "MaxNumEvents", pConfig->dwMaxRecords);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintDword(fp, "EventDbPurgeInterval", pConfig->dwPurgeInterval);
    BAIL_ON_UP_ERROR(dwError);
    if (fputs("\n", fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
EVTParseDays(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    )
{
    DWORD  dwError = 0;
    PSTR   pszTimeIntervalLocal = 0;
    DWORD  dwTimeIntervalLocalLen = 0;
    BOOLEAN  bConvert = FALSE;
    PSTR   pszUnitCode = NULL;
    
    dwError = LwAllocateString(
                    pszTimeInterval, 
                    &pszTimeIntervalLocal
                    );
    BAIL_ON_UP_ERROR(dwError);

    dwTimeIntervalLocalLen = strlen(pszTimeIntervalLocal);
    
    pszUnitCode = pszTimeIntervalLocal + dwTimeIntervalLocalLen - 1;

    if (isdigit((int)(*pszUnitCode))) 
    {
        bConvert = TRUE;
    }
    else if(*pszUnitCode == 'd' || *pszUnitCode == 'D') 
    {
        bConvert = TRUE;
    }

    if(bConvert) {
    	*pdwTimeInterval = (DWORD) atoi(pszTimeIntervalLocal);
    }
    else 
    {
    	*pdwTimeInterval = 0;
        dwError = LW_ERROR_INVALID_PARAMETER;
    }
    
cleanup:
    
    LW_SAFE_FREE_STRING(pszTimeIntervalLocal);
    
    return dwError;

error:
    
    goto cleanup;
}

static
DWORD
EVTParseDiskUsage(
    PCSTR  pszDiskUsage,
    PDWORD pdwDiskUsage
    )
{
    DWORD  dwError = 0;
    PSTR   pszDiskUsageLocal = 0;
    DWORD  dwLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;
    
    dwError = LwAllocateString(
                    pszDiskUsage, 
                    &pszDiskUsageLocal
                    );
    BAIL_ON_UP_ERROR(dwError);

    dwLen = strlen(pszDiskUsageLocal);
    
    pszUnitCode = pszDiskUsageLocal + dwLen - 1;

    if (isdigit((int)(*pszUnitCode))) 
    {
        dwUnitMultiplier = 1;
    }
    else if(*pszUnitCode == 'k' || *pszUnitCode == 'K') 
    {
        dwUnitMultiplier = EVT_BYTES_IN_KB;
    }
    else if(*pszUnitCode == 'm' || *pszUnitCode == 'M') 
    {
        dwUnitMultiplier = EVT_BYTES_IN_MB;
    }
    else if(*pszUnitCode == 'g' || *pszUnitCode == 'G') 
    {
        dwUnitMultiplier = EVT_BYTES_IN_GB;
    }
    else 
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }	

    *pszUnitCode = ' ';

    *pdwDiskUsage = (DWORD) atoi(pszDiskUsageLocal) * dwUnitMultiplier;
    
cleanup:
    
    LW_SAFE_FREE_STRING(pszDiskUsageLocal);
    
    return dwError;

error:
    
    goto cleanup;
}

static
DWORD
EVTParseMaxEntries(
    PCSTR  pszMaxEntries,
    PDWORD pdwMaxEntries
    )
{
    DWORD  dwError = 0;
    PSTR   pszMaxEntriesLocal = 0;

    dwError = LwAllocateString(
                    pszMaxEntries, 
                    &pszMaxEntriesLocal
                    );
    BAIL_ON_UP_ERROR(dwError);

    *pdwMaxEntries = (DWORD) atoi(pszMaxEntriesLocal) ;

cleanup:

    LW_SAFE_FREE_STRING(pszMaxEntriesLocal);

    return dwError;

error:

    goto cleanup;
}
