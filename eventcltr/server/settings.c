/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 * Collector server settings interface
 *
 */

#include "server.h"
#include "settings_p.h"

DWORD
CltrReadDword(
    PCWSTR pwszName,
    DWORD dwDefault,
    PDWORD pdwResult
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwResult = 0;
    DWORD dwValueSize = 4;
    DWORD dwType = 0;

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Services\\LWCollector"),
                0,
                KEY_READ,
                &hKey);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = RegQueryValueEx(
                hKey,
                pwszName,
                NULL,
                &dwType,
                (BYTE*)&dwResult,
                &dwValueSize);
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        *pdwResult = dwDefault;
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_CLTR_ERROR(dwError);

    if (dwType != REG_DWORD || dwValueSize != sizeof(dwResult))
    {
        dwError = EINVAL;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *pdwResult = dwResult;

cleanup:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return dwError;

error:
    *pdwResult = 0;
    goto cleanup;
}

DWORD
CltrGetRecordsPerPeriod(
    PDWORD pdwRecords
    )
{
    return CltrReadDword(
                TEXT("RecordsPerPeriod"),
                10000,
                pdwRecords);
}

DWORD
CltrGetRecordsPerBatch(
    PDWORD pdwRecords
    )
{
    return CltrReadDword(
                TEXT("RecordsPerBatch"),
                100,
                pdwRecords);
}

DWORD
CltrGetPeriodSeconds(
    PDWORD pdwSecs
    )
{
    return CltrReadDword(
                TEXT("PeriodSeconds"),
                10,
                pdwSecs);
}

DWORD
CltrGetDatabasePath(
   OUT PWSTR* ppwszPath
   )
{
    DWORD dwError = 0;
    PWSTR pwszProgramPath = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszPathPos = NULL;
    size_t sPath = 0;
    size_t sRetrieved = 0;

    while (sPath == sRetrieved)
    {
        CLTR_SAFE_FREE_STRING(pwszProgramPath);
        sPath = sPath * 2 + 10;
        dwError = CltrAllocateMemory((DWORD)(sizeof(WCHAR) * sPath),
                                    (PVOID*)&pwszProgramPath);
        BAIL_ON_CLTR_ERROR(dwError);

        sRetrieved = GetModuleFileName(
                            NULL,
                            pwszProgramPath,
                            (DWORD)sPath);
        if (!sRetrieved)
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

    PathRemoveFileSpec(pwszProgramPath);
    
    sPath = wcslen(pwszProgramPath) + wcslen(TEXT(EVENTLOG_DB_FILENAME)) + 2;
    dwError = CltrAllocateMemory((DWORD)(sizeof(WCHAR) * sPath),
                    (PVOID*)&pwszPath);

    pwszPathPos = pwszPath;
    BAIL_ON_CLTR_ERROR(dwError);
    wcscpy(pwszPathPos, pwszProgramPath);
    pwszPathPos += wcslen(pwszPathPos);

    *pwszPathPos = '\\';
    pwszPathPos++;

    wcscpy(pwszPathPos, TEXT(EVENTLOG_DB_FILENAME));

    *ppwszPath = pwszPath;

cleanup:
    CLTR_SAFE_FREE_STRING(pwszProgramPath);

    return dwError;

error:
    CLTR_SAFE_FREE_STRING(pwszPath);
    *ppwszPath = NULL;

    goto cleanup;
}

DWORD
CltrGetLogPath(
   OUT PWSTR* ppwszPath
   )
{
    DWORD dwError = 0;
    PWSTR pwszProgramPath = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszPathPos = NULL;
    size_t sPath = 0;
    size_t sRetrieved = 0;

    while (sPath == sRetrieved)
    {
        CLTR_SAFE_FREE_STRING(pwszProgramPath);
        sPath = sPath * 2 + 10;
        dwError = CltrAllocateMemory((DWORD)(sizeof(WCHAR) * sPath),
                                    (PVOID*)&pwszProgramPath);
        BAIL_ON_CLTR_ERROR(dwError);

        sRetrieved = GetModuleFileName(
                            NULL,
                            pwszProgramPath,
                            (DWORD)sPath);
        if (!sRetrieved)
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

    PathRemoveFileSpec(pwszProgramPath);
    
    dwError = LwRtlWC16StringAllocatePrintfW(
                    &pwszPath,
                    L"%ws\\%ls",
                    pwszProgramPath,
                    EVENTLOG_LOG_FILENAME);
    BAIL_ON_CLTR_ERROR(dwError);

    *ppwszPath = pwszPath;

cleanup:
    CLTR_SAFE_FREE_STRING(pwszProgramPath);

    return dwError;

error:
    CLTR_SAFE_FREE_STRING(pwszPath);
    *ppwszPath = NULL;

    goto cleanup;
}

DWORD
CltrGetLogLevel(
    CltrLogLevel* pdwResult
    )
{
    DWORD dwResult = 0;
    DWORD dwError = 0;

    dwError = CltrReadDword(
                TEXT("LogLevel"),
                CLTR_LOG_LEVEL_ERROR,
                &dwResult);
    BAIL_ON_CLTR_ERROR(dwError);

    if (dwResult > CLTR_LOG_LEVEL_DEBUG)
    {
        dwResult = CLTR_LOG_LEVEL_ERROR;
    }
    *pdwResult = dwResult;

cleanup:

    return dwError;

error:
    *pdwResult = CLTR_LOG_LEVEL_ERROR;

    goto cleanup;
}

DWORD
CltrGetRemoteSecurityDescriptor(
    PSECURITY_DESCRIPTOR* ppDescriptor
    )
{
    DWORD dwError = 0;
    HKEY hKey = NULL;
    PSECURITY_DESCRIPTOR pDescriptor = NULL;
    DWORD dwAccessSize = 0;
    PWSTR pwszAccess = NULL;
    SID_IDENTIFIER_AUTHORITY NTAuthority = SECURITY_NT_AUTHORITY;

    DWORD dwDescriptorSize = 0;
    PSECURITY_DESCRIPTOR pAbsoluteSD = NULL;
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;
    DWORD dwSaclSize = 0;
    PACL pSacl = NULL;
    DWORD dwOwnerSize = 0;
    PSID pOwner = NULL;
    DWORD dwPrimaryGroupSize = 0;
    PSID pPrimaryGroup = NULL;

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Services\\LWCollector"),
                0,
                KEY_READ,
                &hKey);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = RegQueryValueEx(
                hKey,
                TEXT("RemoteAccess"),
                NULL,
                NULL,
                NULL,
                &dwAccessSize);
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        /* Owner: Local Service
           Group: Builtin Administrators
           Protected (do not inherit parent promissions)

           Domain Computers are allowed to create children (add events)
           Domain Administrators are allowed to create children (add events)
           Domain Administrators are allowed to read properties (read events)
           Domain Administrators are allowed to delete children (delete events)
           Builtin Administrators are allowed to create children (add events)
           Builtin Administrators are allowed to read properties (read events)
           Builtin Administrators are allowed to delete children (delete events)
         */
        dwError = CltrAllocateString(
            TEXT("O:LSG:BAD:PAR(A;;CCDCRP;;;BA)(A;;CCDCRP;;;DA)(A;;CC;;;DC)"),
                        &pwszAccess);
        BAIL_ON_CLTR_ERROR(dwError);
    }
    else
    {
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = CltrAllocateMemory(dwAccessSize * sizeof(pwszAccess[0]),
            (PVOID*)&pwszAccess);
        BAIL_ON_CLTR_ERROR(dwError);

        dwError = RegQueryValueEx(
            hKey,
            TEXT("RemoteAccess"),
            NULL,
            NULL,
            (LPBYTE)pwszAccess,
            &dwAccessSize);
        BAIL_ON_CLTR_ERROR(dwError);

        if (dwAccessSize < 2)
        {
            dwError = CLTR_ERROR_DATA_ERROR;
            BAIL_ON_CLTR_ERROR(dwError);
        }

        pwszAccess[dwAccessSize/2 - 1] = 0;
    }

    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
        pwszAccess,
        SDDL_REVISION_1,
        &pDescriptor,
        NULL))
    {
        dwError = GetLastError();   
        BAIL_ON_CLTR_ERROR(dwError);
    }

    if (!MakeAbsoluteSD(
        pDescriptor,
        &pAbsoluteSD,
        &dwDescriptorSize,
        pDacl,
        &dwDaclSize,
        pSacl,
        &dwSaclSize,
        pOwner,
        &dwOwnerSize,
        pPrimaryGroup,
        &dwPrimaryGroupSize))
    {
        dwError = GetLastError();
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            dwError = 0;
        }
        else
        {
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

    dwError = CltrAllocateMemory(
        dwDescriptorSize,
        (PVOID*)&pAbsoluteSD);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
        dwDaclSize,
        (PVOID*)&pDacl);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
        dwSaclSize,
        (PVOID*)&pSacl);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
        dwOwnerSize,
        (PVOID*)&pOwner);
    BAIL_ON_CLTR_ERROR(dwError);

    dwError = CltrAllocateMemory(
        dwPrimaryGroupSize,
        (PVOID*)&pPrimaryGroup);
    BAIL_ON_CLTR_ERROR(dwError);

    if (!MakeAbsoluteSD(
        pDescriptor,
        pAbsoluteSD,
        &dwDescriptorSize,
        pDacl,
        &dwDaclSize,
        pSacl,
        &dwSaclSize,
        pOwner,
        &dwOwnerSize,
        pPrimaryGroup,
        &dwPrimaryGroupSize))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }

    LocalFree(pDescriptor);
    pDescriptor = NULL;

    if (!dwOwnerSize)
    {
        CltrFreeMemory(pOwner);

        if (!AllocateAndInitializeSid(
            &NTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pOwner))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }

        if (!SetSecurityDescriptorOwner(
            pAbsoluteSD,
            pOwner,
            TRUE))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

    if (!dwPrimaryGroupSize)
    {
        CltrFreeMemory(pPrimaryGroup);

        if (!AllocateAndInitializeSid(
            &NTAuthority,
            1,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pPrimaryGroup))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }

        if (!SetSecurityDescriptorGroup(
            pAbsoluteSD,
            pPrimaryGroup,
            TRUE))
        {
            dwError = GetLastError();
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }

    if (!MakeSelfRelativeSD(
        pAbsoluteSD,
        pDescriptor,
        &dwDescriptorSize))
    {
        dwError = GetLastError();
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            dwError = 0;
        }
        else
        {
            BAIL_ON_CLTR_ERROR(dwError);
        }
    }
    
    pDescriptor = LocalAlloc(LPTR, dwDescriptorSize);
    if (!pDescriptor) {
         dwError = GetLastError();
         BAIL_ON_CLTR_ERROR(dwError);
    }

    if (!MakeSelfRelativeSD(
        pAbsoluteSD,
        pDescriptor,
        &dwDescriptorSize))
    {
        dwError = GetLastError();
        BAIL_ON_CLTR_ERROR(dwError);
    }

    *ppDescriptor = pDescriptor;

cleanup:
    CLTR_SAFE_FREE_STRING(pwszAccess);
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    CLTR_SAFE_FREE_MEMORY(pAbsoluteSD);
    CLTR_SAFE_FREE_MEMORY(pDacl);
    CLTR_SAFE_FREE_MEMORY(pSacl);
    CLTR_SAFE_FREE_MEMORY(pOwner);
    CLTR_SAFE_FREE_MEMORY(pPrimaryGroup);

    return dwError;

error:
    if (pDescriptor)
    {
        LocalFree(pDescriptor);
    }
    *ppDescriptor = NULL;
    goto cleanup;
}