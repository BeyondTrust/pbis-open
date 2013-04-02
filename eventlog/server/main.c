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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Service (Process Utilities)
 *
 */

#include "includes.h"


EVTSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0,                          /* Process exit flag */
    0,                          /* Process exit code */
    0,                          /* Replace existing db flag */
    0,                          /* Max log size  */
    0,                          /* Max records */
    0,                          /* Remove records older than*/
    0,                          /* Purge records at interval*/
    TRUE,                          /* Enable/disable Remove records a boolean value TRUE or FALSE*/
    FALSE,                       /* Register TCP/IP RPC endpoints*/
    NULL,
};

#define EVT_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define EVT_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)


static
DWORD
EVTCreateAccessDescriptor(
    PCSTR pszAllowReadTo,
    PCSTR pszAllowWriteTo,
    PCSTR pszAllowDeleteTo,
    PSECURITY_DESCRIPTOR_ABSOLUTE* ppDescriptor,
    PBOOLEAN pbFullyResolved
    );

static
DWORD
EVTGetRegisterTcpIp(
    PBOOLEAN pbRegisterTcpIp
    );

DWORD
EVTGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = LwAllocateString(gServerInfo.szCachePath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTCheckAllowed(
    PACCESS_TOKEN pUserToken,
    ACCESS_MASK dwAccessMask,
    BOOLEAN* pAllowed
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    // Do not free
    PSECURITY_DESCRIPTOR_ABSOLUTE pDescriptor = NULL;
    PSTR pszAllowReadTo = NULL;
    PSTR pszAllowWriteTo = NULL;
    PSTR pszAllowDeleteTo = NULL;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bFullyResolved = FALSE;
    GENERIC_MAPPING GenericMapping = {0};
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ACCESS_MASK dwGrantedAccess = 0;
    BOOLEAN allowed = FALSE;

    EVT_LOCK_SERVERINFO;
    bLocked = TRUE;

    if (!gServerInfo.pAccess)
    {
        dwError = LwAllocateString(
                        gServerInfo.pszAllowReadTo ?
                            gServerInfo.pszAllowReadTo : "",
                        &pszAllowReadTo);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateString(
                        gServerInfo.pszAllowWriteTo ?
                            gServerInfo.pszAllowWriteTo : "",
                        &pszAllowWriteTo);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwAllocateString(
                        gServerInfo.pszAllowDeleteTo ?
                            gServerInfo.pszAllowDeleteTo : "",
                        &pszAllowDeleteTo);
        BAIL_ON_EVT_ERROR(dwError);

        EVT_UNLOCK_SERVERINFO;
        bLocked = FALSE;

        dwError = EVTCreateAccessDescriptor(
            pszAllowReadTo,
            pszAllowWriteTo,
            pszAllowDeleteTo,
            &pAbsolute,
            &bFullyResolved);
        BAIL_ON_EVT_ERROR(dwError);

        EVT_LOCK_SERVERINFO;
        bLocked = TRUE;

        if (bFullyResolved && !gServerInfo.pAccess)
        {
            gServerInfo.pAccess = pAbsolute;
            pAbsolute = NULL;
        }
        else
        {
            pDescriptor = pAbsolute;
        }
    }
    if (pDescriptor == NULL)
    {
        pDescriptor = gServerInfo.pAccess;
    }

    if (!RtlAccessCheck(pDescriptor,
                        pUserToken,
                        dwAccessMask,
                        0,
                        &GenericMapping,
                        &dwGrantedAccess,
                        &ntStatus))
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
        if (dwError == ERROR_ACCESS_DENIED)
        {
            dwError = 0;
        }
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        allowed = TRUE;
    }

    *pAllowed = allowed;

cleanup:
    if (bLocked)
    {
        EVT_UNLOCK_SERVERINFO;
    }
    LW_SAFE_FREE_STRING(pszAllowReadTo);
    LW_SAFE_FREE_STRING(pszAllowWriteTo);
    LW_SAFE_FREE_STRING(pszAllowDeleteTo);
    EVTFreeSecurityDescriptor(pAbsolute);
    return (dwError);

error:
    goto cleanup;
}

DWORD
EVTGetMaxRecords(
    DWORD* pdwMaxRecords
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxRecords = gServerInfo.dwMaxRecords;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxAge(
    DWORD* pdwMaxAge
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxAge = gServerInfo.dwMaxAge;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxLogSize(
    DWORD* pdwMaxLogSize
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxLogSize = gServerInfo.dwMaxLogSize;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetDBPurgeInterval(
    PDWORD pdwPurgeInterval
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwPurgeInterval = gServerInfo.dwPurgeInterval;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetRemoveAsNeeded(
    PBOOLEAN pbRemoveAsNeeded
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pbRemoveAsNeeded = gServerInfo.bRemoveAsNeeded;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

static
DWORD
EVTGetRegisterTcpIp(
    PBOOLEAN pbRegisterTcpIp
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pbRegisterTcpIp = gServerInfo.bRegisterTcpIp;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = LwAllocateString(gServerInfo.szPrefixPath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
EVTUnlockServerInfo()
{
    EVT_UNLOCK_SERVERINFO;
}


static
DWORD
EVTSetConfigDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    gServerInfo.dwMaxLogSize = EVT_DEFAULT_MAX_LOG_SIZE;
    gServerInfo.dwMaxRecords =  EVT_DEFAULT_MAX_RECORDS;
    gServerInfo.dwMaxAge = EVT_DEFAULT_MAX_AGE;
    gServerInfo.dwPurgeInterval = EVT_DEFAULT_PURGE_INTERVAL;
    gServerInfo.bRemoveAsNeeded = EVT_DEFAULT_BOOL_REMOVE_RECORDS_AS_NEEDED;
    gServerInfo.bRegisterTcpIp = EVT_DEFAULT_BOOL_REGISTER_TCP_IP;

    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    LW_SAFE_FREE_STRING(gServerInfo.pszAllowReadTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowWriteTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowDeleteTo);

    EVT_UNLOCK_SERVERINFO;

    return dwError;
}

static
DWORD
EVTSetServerDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    strcpy(gServerInfo.szCachePath, CACHEDIR);
    strcpy(gServerInfo.szPrefixPath, PREFIXDIR);

    EVT_UNLOCK_SERVERINFO;

    dwError = EVTSetConfigDefaults();

    return dwError;
}


static PSTR gpszAllowReadTo;
static PSTR gpszAllowWriteTo;
static PSTR gpszAllowDeleteTo;
static LWREG_CONFIG_ITEM gConfigDescription[] =
{
    {
        "MaxDiskUsage",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxLogSize),
        NULL
    },
    {
        "MaxNumEvents",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxRecords),
        NULL
    },
    {
        "MaxEventLifespan",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwMaxAge),
        NULL
    },
    {
        "EventDbPurgeInterval",
        TRUE,
        LwRegTypeDword,
        0,
        -1,
        NULL,
        &(gServerInfo.dwPurgeInterval),
        NULL
    },
    {
        "RemoveEventsAsNeeded",
        TRUE,
        LwRegTypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bRemoveAsNeeded)
    },
    {
        "RegisterTcpIp",
        TRUE,
        LwRegTypeBoolean,
        0,
        -1,
        NULL,
        &(gServerInfo.bRegisterTcpIp),
        NULL
    },
    {
        "AllowReadTo",
        TRUE,
        LwRegTypeString,
        0,
        -1,
        NULL,
        &gpszAllowReadTo,
        NULL
    },
    {
        "AllowWriteTo",
        TRUE,
        LwRegTypeString,
        0,
        -1,
        NULL,
        &gpszAllowWriteTo,
        NULL
    },
    {
        "AllowDeleteTo",
        TRUE,
        LwRegTypeString,
        0,
        -1,
        NULL,
        &gpszAllowDeleteTo,
        NULL
    }
};

VOID
EVTLogConfigReload(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LwAllocateStringPrintf(
                 &pszDescription,
                 "     Current config settings are...\r\n" \
                 "     Max Disk Usage :                 %d\r\n" \
                 "     Max Number Of Events:            %d\r\n" \
                 "     Max Event Lifespan:              %d\r\n" \
                 "     Remove Events As Needed:         %s\r\n" \
                 "     Register TCP/IP RPC endpoints:   %s\r\n" \
                 "     Allow Read   To :                %s\r\n" \
                 "     Allow Write  To :                %s\r\n" \
                 "     Allow Delete To :                %s\r\n",
                 gServerInfo.dwMaxLogSize,
                 gServerInfo.dwMaxRecords,
                 gServerInfo.dwMaxAge,
                 gServerInfo.bRemoveAsNeeded? "true" : "false",
                 gServerInfo.bRegisterTcpIp ? "true" : "false",
                 gServerInfo.pszAllowReadTo ?
                    gServerInfo.pszAllowReadTo: "",
                 gServerInfo.pszAllowWriteTo ?
                    gServerInfo.pszAllowWriteTo: "",
                 gServerInfo.pszAllowDeleteTo ?
                    gServerInfo.pszAllowDeleteTo: "");

    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_INFO("%s", pszDescription);

cleanup:

    LW_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

static
DWORD
EVTStringSplit(
    PCSTR   pszInput,
    PDWORD  pdwCount,
    PSTR**  pppszArray
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PCSTR  pszStart = NULL;
    PCSTR  pszEnd = NULL;
    PSTR* ppszArray = NULL;
    PSTR pszAdd = NULL;

    for (pszStart = pszInput; *pszStart !=  0; pszStart++)
    {
        if (*pszStart == ',') dwCount++;
    }
    dwCount++;

    dwError = LwAllocateMemory(
                  (dwCount+1)*sizeof(PCSTR),
                  (PVOID *)&ppszArray);

    dwCount = 0;
    pszStart = pszInput;
    while (TRUE)
    {
        pszEnd = strchr(pszStart, ',');
        if ( pszEnd )
        {
            dwError = LwStrndup(
                         pszStart,
                         pszEnd - pszStart,
                         &pszAdd);
            BAIL_ON_EVT_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString(
                        pszStart,
                        &pszAdd);
            BAIL_ON_EVT_ERROR(dwError);
        }
        LwStripWhitespace(pszAdd, TRUE, TRUE);
        if (pszAdd[0])
        {
            ppszArray[dwCount++] = pszAdd;
            pszAdd = NULL;
        }
        else
        {
            LW_SAFE_FREE_STRING(pszAdd);
        }

        if (pszEnd)
        {
            pszStart = pszEnd + 1;
        }
        else
        {
            break;
        }
    }

    *pppszArray = ppszArray;
    *pdwCount = dwCount;

cleanup:
    LW_SAFE_FREE_STRING(pszAdd);
    return dwError;

error:
    LwFreeStringArray(
        ppszArray,
        dwCount);
    goto cleanup;
}

VOID
EVTFreeSidArray(
    PLW_MAP_SECURITY_CONTEXT pContext,
    DWORD dwCount,
    PSID* ppSidArray
    )
{
    DWORD dwInputIndex = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        LwMapSecurityFreeSid(pContext, &ppSidArray[dwInputIndex]);
    }
    LW_SAFE_FREE_MEMORY(ppSidArray);
}

static
DWORD
EVTNamesToSids(
    PLW_MAP_SECURITY_CONTEXT pContext,
    DWORD dwCount,
    PSTR* ppszArray,
    PDWORD pdwSidCount,
    PSID** pppSidArray
    )
{
    DWORD dwError = 0;
    PSID pSid = NULL;
    DWORD dwInputIndex = 0;
    DWORD dwOutputIndex = 0;
    PSID* ppSidArray = NULL;

    dwError = LwAllocateMemory(
                sizeof(PSID) * dwCount,
                (PVOID)&ppSidArray);
    BAIL_ON_EVT_ERROR(dwError);

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwError = LwNtStatusToWin32Error(
            LwMapSecurityGetSidFromName(
                pContext,
                &pSid,
                TRUE,
                ppszArray[dwInputIndex]));
        if (dwError == LW_ERROR_NO_SUCH_USER || dwError == ERROR_NOT_FOUND)
        {
            dwError = LwNtStatusToWin32Error(
                LwMapSecurityGetSidFromName(
                    pContext,
                    &pSid,
                    FALSE,
                    ppszArray[dwInputIndex]));
        }
        if (dwError == LW_ERROR_NO_SUCH_GROUP || dwError == ERROR_NOT_FOUND)
        {
            dwError = 0;
        }
        BAIL_ON_EVT_ERROR(dwError);
        if (pSid)
        {
            ppSidArray[dwOutputIndex] = pSid;
            dwOutputIndex++;
        }
    }

    *pppSidArray = ppSidArray;
    *pdwSidCount = dwOutputIndex;

cleanup:
    return dwError;

error:
    EVTFreeSidArray(
        pContext,
        dwCount,
        ppSidArray);
    goto cleanup;
}

DWORD
EVTGetAllowAcesSize(
    DWORD dwCount,
    PSID* ppSidArray
    )
{
    DWORD dwInputIndex = 0;
    DWORD dwSize = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwSize += RtlLengthAccessAllowedAce(ppSidArray[dwInputIndex]);
    }
    return dwSize;
}

static
DWORD
EVTAddAllowAces(
    PACL pDacl,
    DWORD dwCount,
    PSID* ppSidArray,
    ACCESS_MASK dwAccessMask
    )
{
    DWORD dwInputIndex = 0;
    DWORD dwError = 0;

    for (dwInputIndex = 0; dwInputIndex < dwCount; dwInputIndex++)
    {
        dwError = LwNtStatusToWin32Error(
            RtlAddAccessAllowedAceEx(pDacl,
                                            ACL_REVISION,
                                            0,
                                            dwAccessMask,
                                            ppSidArray[dwInputIndex]));
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
EVTFreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    PSID pOwnerSid = NULL;
    BOOLEAN bOwnerDefaulted = FALSE;
    PSID pPrimaryGroupSid = NULL;
    BOOLEAN bPrimaryGroupDefaulted = FALSE;
    PACL pDacl = NULL;
    BOOLEAN bDaclPresent = FALSE;
    BOOLEAN bDaclDefaulted = FALSE;
    PACL pSacl = NULL;
    BOOLEAN bSaclPresent = FALSE;
    BOOLEAN bSaclDefaulted = FALSE;

    if (pSecDesc)
    {
        RtlGetOwnerSecurityDescriptor(pSecDesc,
                                                 &pOwnerSid,
                                                 &bOwnerDefaulted);
        LW_SAFE_FREE_MEMORY(pOwnerSid);

        RtlGetGroupSecurityDescriptor(pSecDesc,
                                                 &pPrimaryGroupSid,
                                                 &bPrimaryGroupDefaulted);
        LW_SAFE_FREE_MEMORY(pPrimaryGroupSid);

        RtlGetDaclSecurityDescriptor(pSecDesc,
                                                &bDaclPresent,
                                                &pDacl,
                                                &bDaclDefaulted);
        LW_SAFE_FREE_MEMORY(pDacl);

        RtlGetSaclSecurityDescriptor(pSecDesc,
                                                &bSaclPresent,
                                                &pSacl,
                                                &bSaclDefaulted);
        LW_SAFE_FREE_MEMORY(pSacl);

        LW_SAFE_FREE_MEMORY(pSecDesc);
    }
}

static
DWORD
EVTCreateAccessDescriptor(
    PCSTR pszAllowReadTo,
    PCSTR pszAllowWriteTo,
    PCSTR pszAllowDeleteTo,
    PSECURITY_DESCRIPTOR_ABSOLUTE* ppDescriptor,
    PBOOLEAN pbFullyResolved
    )
{
    PSECURITY_DESCRIPTOR_ABSOLUTE pDescriptor = NULL;
    DWORD dwCount = 0;
    PSTR* ppszArray = NULL;
    DWORD dwReadCount = 0;
    DWORD dwWriteCount = 0;
    DWORD dwDeleteCount = 0;
    PSID* ppReadList = NULL;
    PSID* ppWriteList = NULL;
    PSID* ppDeleteList = NULL;
    PACL pDacl = NULL;
    DWORD dwDaclSize = 0;
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;
    DWORD dwError = 0;
    PSID pLocalSystem = NULL;
    PSID pAdministrators = NULL;
    BOOLEAN bFullyResolved = TRUE;

    dwError = LwAllocateWellKnownSid(
                    WinLocalSystemSid,
                    NULL,
                    &pLocalSystem,
                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(
                    WinBuiltinAdministratorsSid,
                    NULL,
                    &pAdministrators,
                    NULL);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        LwMapSecurityCreateContext(
                &pContext));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwAllocateMemory(
                SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                (PVOID*)&pDescriptor);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateSecurityDescriptorAbsolute(
                pDescriptor,
                SECURITY_DESCRIPTOR_REVISION));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTStringSplit(
                pszAllowReadTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwReadCount,
                &ppReadList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwReadCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwError = EVTStringSplit(
                pszAllowWriteTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwWriteCount,
                &ppWriteList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwWriteCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwError = EVTStringSplit(
                pszAllowDeleteTo,
                &dwCount,
                &ppszArray);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTNamesToSids(
                pContext,
                dwCount,
                ppszArray,
                &dwDeleteCount,
                &ppDeleteList);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwDeleteCount < dwCount)
    {
        bFullyResolved = FALSE;
    }

    LwFreeStringArray(
        ppszArray,
        dwCount);
    ppszArray = NULL;

    dwDaclSize = ACL_HEADER_SIZE +
        EVTGetAllowAcesSize(dwReadCount, ppReadList) +
        EVTGetAllowAcesSize(dwWriteCount, ppWriteList) +
        EVTGetAllowAcesSize(dwDeleteCount, ppDeleteList);

    dwError = LwAllocateMemory(
        dwDaclSize,
        OUT_PPVOID(&pDacl));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION));
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwReadCount,
                ppReadList,
                EVENTLOG_READ_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwWriteCount,
                ppWriteList,
                EVENTLOG_WRITE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAddAllowAces(
                pDacl,
                dwWriteCount,
                ppWriteList,
                EVENTLOG_DELETE_RECORD);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetDaclSecurityDescriptor(
            pDescriptor,
            TRUE,
            pDacl,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pDacl = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlSetOwnerSecurityDescriptor(
            pDescriptor,
            pLocalSystem,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pLocalSystem = NULL;

    dwError = LwNtStatusToWin32Error(
        RtlSetGroupSecurityDescriptor(
            pDescriptor,
            pAdministrators,
            FALSE));
    BAIL_ON_EVT_ERROR(dwError);
    pAdministrators = NULL;

    *ppDescriptor = pDescriptor;
    if (pbFullyResolved)
    {
        *pbFullyResolved = bFullyResolved;
    }

cleanup:
    if (ppszArray)
    {
        LwFreeStringArray(
            ppszArray,
            dwCount);
    }
    EVTFreeSidArray(
        pContext,
        dwReadCount,
        ppReadList);
    EVTFreeSidArray(
        pContext,
        dwWriteCount,
        ppWriteList);
    EVTFreeSidArray(
        pContext,
        dwDeleteCount,
        ppDeleteList);
    LwMapSecurityFreeContext(&pContext);
    return dwError;

error:
    EVTFreeSecurityDescriptor(pDescriptor);
    *ppDescriptor = NULL;
    if (pbFullyResolved)
    {
        *pbFullyResolved = FALSE;
    }
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pLocalSystem);
    LW_SAFE_FREE_MEMORY(pAdministrators);
    goto cleanup;
}

static
DWORD
EVTReadEventLogConfigSettings()
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    EVT_LOG_INFO("Read Eventlog configuration settings");

    dwError = EVTSetConfigDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOCK_SERVERINFO;
    bLocked = TRUE;

    dwError = RegProcessConfig(
                "Services\\eventlog\\Parameters",
                "Policy\\Services\\eventlog\\Parameters",
                gConfigDescription,
                sizeof(gConfigDescription)/sizeof(gConfigDescription[0]));

    LW_SAFE_FREE_STRING(gServerInfo.pszAllowReadTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowWriteTo);
    LW_SAFE_FREE_STRING(gServerInfo.pszAllowDeleteTo);
    gServerInfo.pszAllowReadTo = gpszAllowReadTo;
    gServerInfo.pszAllowWriteTo = gpszAllowWriteTo;
    gServerInfo.pszAllowDeleteTo = gpszAllowDeleteTo;
    gpszAllowReadTo = NULL;
    gpszAllowWriteTo = NULL;
    gpszAllowDeleteTo = NULL;

    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    EVT_UNLOCK_SERVERINFO;
    bLocked = FALSE;

    EVTLogConfigReload();

cleanup:

    LW_SAFE_FREE_STRING(gpszAllowReadTo);
    LW_SAFE_FREE_STRING(gpszAllowWriteTo);
    LW_SAFE_FREE_STRING(gpszAllowDeleteTo);

    if (bLocked)
    {
        EVT_UNLOCK_SERVERINFO;
    }

    return dwError;

error:
    goto cleanup;
}


static
void*
EVTNetworkThread(
    void* pArg
    )
{
    DWORD dwError = 0;
    DWORD index = 0;
    static ENDPOINT endpoints[] =
    {
        {"ncacn_ip_tcp", NULL, TRUE},
        {NULL, NULL}
    };
    struct timespec delay = {5, 0};
    BOOLEAN *pbExitNow = (BOOLEAN *)pArg;
    BOOLEAN bFirstAttempt = TRUE;
 
    while (endpoints[index].protocol && !*pbExitNow)
    {
        dwError = EVTRegisterEndpoint(
            "Likewise Eventlog Service",
            &endpoints[index]
            );
        
        if (dwError)
        {
            if (bFirstAttempt)
            {
                EVT_LOG_ERROR("Failed to register RPC endpoint.  Error Code: [%u]\n", dwError);
                bFirstAttempt = FALSE;
            }
            dwError = 0;
            dcethread_delay(&delay);
        }
        else
        {
            if (endpoints[index].endpoint)
            {
                EVT_LOG_VERBOSE("Listening on %s:[%s]",
                                endpoints[index].protocol,
                                endpoints[index].endpoint);
            }
            else
            {
                EVT_LOG_VERBOSE("Listening on %s",
                                endpoints[index].protocol,
                                endpoints[index].endpoint);
            }
                                
            index++;
            bFirstAttempt = TRUE;
        }
    }

    dwError = EVTListen();

    if (dwError)
    {
        raise(SIGTERM);
    }
    return NULL;
}

NTSTATUS
EVTSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
EVTSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}

static BOOLEAN gbRegisteredTcpIp = FALSE;
static BOOLEAN gbExitNow = FALSE;
static dcethread* gNetworkThread = NULL;

NTSTATUS
EVTSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;
    BOOLEAN bRegisterTcpIp = TRUE;

    dwError = EVTSetServerDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbCreateDB(gServerInfo.bReplaceDB);
    BAIL_ON_EVT_ERROR(dwError);

    if (gServerInfo.bReplaceDB) {
        goto cleanup;
    }

    dwError = EVTReadEventLogConfigSettings();
    if (dwError != 0)
    {
        EVT_LOG_ERROR("Failed to read eventlog configuration.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = EVTGetRegisterTcpIp(&bRegisterTcpIp);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = LwEvtDbInitEventDatabase();
    BAIL_ON_EVT_ERROR(dwError);

    EvtSnmpSetup();

    dwError = LwmEvtSrvStartListenThread();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTRegisterInterface();
    BAIL_ON_EVT_ERROR(dwError);

    if (bRegisterTcpIp)
    {
        dwError = LwMapErrnoToLwError(dcethread_create(
                                      &gNetworkThread,
                                      NULL,
                                      EVTNetworkThread,
                                      &gbExitNow));
        BAIL_ON_EVT_ERROR(dwError);

        gbRegisteredTcpIp = TRUE;
    }

cleanup:
    return LwWin32ErrorToNtStatus(dwError);

error:
    goto cleanup;
}

NTSTATUS
EVTSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    DWORD dwError = 0;

    EVT_LOG_INFO("Eventlog Service exiting...");

    gbExitNow = TRUE;

    if (gbRegisteredTcpIp)
    {
        dwError = EVTUnregisterAllEndpoints();
        BAIL_ON_EVT_ERROR(dwError);

        dwError = EVTStopListen();
        BAIL_ON_EVT_ERROR(dwError);
    }

    EvtSnmpTearDown();

    dwError = LwmEvtSrvStopListenThread();
    BAIL_ON_EVT_ERROR(dwError);

    if (gbRegisteredTcpIp)
    {
        dwError = LwMapErrnoToLwError(dcethread_interrupt(gNetworkThread));
        BAIL_ON_EVT_ERROR(dwError);

        dwError = LwMapErrnoToLwError(dcethread_join(gNetworkThread, NULL));
        BAIL_ON_EVT_ERROR(dwError);
    }

 cleanup:

    LwEvtDbShutdownEventDatabase();

    EVTSetConfigDefaults();
    EVTFreeSecurityDescriptor(gServerInfo.pAccess);
    gServerInfo.pAccess = NULL;

    return LwWin32ErrorToNtStatus(dwError);

error:

    EVT_LOG_ERROR("Eventlog exiting due to error [code:%d]", dwError);

    goto cleanup;
}

NTSTATUS
EVTSvcmRefresh(
    PLW_SVCM_INSTANCE pInstance
    )
{
   DWORD dwError = ERROR_SUCCESS;

   EVT_LOG_INFO("Refreshing configuration");

    dwError = EVTReadEventLogConfigSettings();
    if (dwError != 0)
    {
        EVT_LOG_ERROR("Refresh. Failed to read eventlog configuration.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = EvtSnmpReadConfiguration();
    if (dwError != ERROR_SUCCESS)
    {
        EVT_LOG_ERROR("Refresh. Failed to read eventlog snmp configuration.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

   return dwError;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = EVTSvcmInit,
    .Destroy = EVTSvcmDestroy,
    .Start = EVTSvcmStart,
    .Stop = EVTSvcmStop,
    .Refresh = EVTSvcmRefresh
};

#define SVCM_ENTRY_POINT LW_RTL_SVCM_ENTRY_POINT_NAME(eventlog)

PLW_SVCM_MODULE
SVCM_ENTRY_POINT(
    VOID
    )
{
    return &gService;
}

