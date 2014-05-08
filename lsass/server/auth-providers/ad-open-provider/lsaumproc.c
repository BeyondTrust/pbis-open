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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"
#include "provider-main.h"

#ifdef HAVE_STRUCT_PSINFO
    // To determine whether the user is truly gone we have
    // to determine whether there are any active processes
    // owned by the user.
    //
    // Solaris and AIX have a /proc pseudo filesystem with
    // /proc/pid/psinfo files.

DWORD
LsaUmpIsUserActive(
    IN  uid_t     uUid,
    OUT BOOLEAN * pbUserIsActive
    )
{
    DWORD           dwError = 0;
    BOOLEAN         bFoundUserProcess = FALSE;
    DIR *           dir = NULL;
    struct dirent * dirEntry = NULL;
    PSTR            filePath = NULL;
    struct psinfo   infoStruct;
    FILE *          infoFile = NULL;
    BOOLEAN         bFileExists;

    if ((dir = opendir("/proc")) == NULL)
    {
        LSA_LOG_DEBUG("LSA User Manager - opendir reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off.");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    while (1)
    {
        errno = 0;

        dirEntry = readdir(dir);
        if (dirEntry == NULL)
            break;

        if ( dirEntry->d_name[0] == '.' )
            continue;

        if(!isdigit((int)dirEntry->d_name[0]))
            continue;

        LW_SAFE_FREE_STRING(filePath);
        dwError = LwAllocateStringPrintf(
                      &filePath,
                      "/proc/%s/psinfo",
                      dirEntry->d_name);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCheckFileExists(filePath, &bFileExists);
        BAIL_ON_LSA_ERROR(dwError);

        if ( !bFileExists )
        {
            // On AIX 6.1, a defunct process can lack a psinfo file.
            continue;
        }

        if ( infoFile )
        {
            fclose(infoFile);
            infoFile = NULL;
        }
        if ( (infoFile = fopen(filePath, "r")) == NULL )
        {
            continue;
        }

        if ( fread(&infoStruct, sizeof(infoStruct), 1, infoFile) == 1 )
        {
            if ( uUid == infoStruct.pr_uid )
            {
                bFoundUserProcess = TRUE;
                break;
            }
        }

        fclose(infoFile);
        infoFile = NULL;
    }

    *pbUserIsActive = bFoundUserProcess;

cleanup:

    if ( infoFile )
        fclose(infoFile);

    LW_SAFE_FREE_STRING(filePath);

    if ( dir )
        closedir(dir);

    return dwError;

error:

    *pbUserIsActive = TRUE;

    goto cleanup;
}

#elif HAVE_DECL_PSTAT_GETPROC
    // HPUX has no /proc pseudo filesystem but does
    // have pstat_getproc().

DWORD
LsaUmpIsUserActive(
    IN  uid_t     uUid,
    OUT BOOLEAN * pbUserIsActive
    )
{
    DWORD             dwError = 0;
    BOOLEAN           bFoundUserProcess = FALSE;
    struct pst_status status[10];
    int               inBuffer;
    int               i;

    // If status is too large pstat_getproc will
    // fail with EFAULT.

    inBuffer = pstat_getproc(status,
                   sizeof(status[0]),
                   sizeof(status)/sizeof(status[0]),
                   0);
    if ( inBuffer < 0 )
    {
        LSA_LOG_DEBUG("LSA User Manager - pstat_getproc reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off.");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    while ( inBuffer > 0 )
    {
        for ( i = 0 ; i < inBuffer ; i++ )
        {
            if ( uUid == status[i].pst_uid )
            {
                bFoundUserProcess = TRUE;
                break;
            }
        }

        if ( bFoundUserProcess )
            break;

        inBuffer = pstat_getproc(status,
                       sizeof(status[0]),
                       sizeof(status)/sizeof(status[0]),
                       status[inBuffer - 1].pst_idx + 1);
    }

    *pbUserIsActive = bFoundUserProcess;

cleanup:

    return dwError;

error:

    *pbUserIsActive = TRUE;

    goto cleanup;
}

#elif defined(__LWI_DARWIN__)
    // Like FreeBSD, Apple has kvm_getprocs but it does
    // not work and sysctl must be used instead.

DWORD
LsaUmpIsUserActive(
    IN  uid_t     uUid,
    OUT BOOLEAN * pbUserIsActive
    )
{
    DWORD   dwError = 0;
    BOOLEAN bFoundUserProcess = FALSE;
    int     mib[4];
    size_t  len = 0;
    struct  kinfo_proc *procs = NULL;

    // There are two calls to sysctl.  The first is to obtain
    // the size of the required buffer.  Because the data changes
    // frequently, the result is an estimate of what will be 
    // needed and it rounds upwards.  It's not until the second
    // call where the data is retrieved that an accurate size is
    // obtained.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_RUID;
    mib[3] = uUid;

    if ( sysctl(mib, 4, NULL, &len, NULL, 0) < 0 )
    {
        LSA_LOG_DEBUG("LSA User Manager - sysctl reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  len,
                  (PVOID*)&procs);
    BAIL_ON_LSA_ERROR(dwError);

    if ( sysctl(mib, 4, procs, &len, NULL, 0) < 0 )
    {
        LSA_LOG_DEBUG("LSA User Manager - sysctl reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( len > 0 )
    {
        bFoundUserProcess = TRUE;
    }

    *pbUserIsActive = bFoundUserProcess;

cleanup:

    LW_SAFE_FREE_MEMORY(procs);

    return dwError;

error:

    *pbUserIsActive = TRUE;

    goto cleanup;
}

#elif HAVE_DECL_KVM_GETPROCS
    // FreeBSD has no /proc pseudo filesystem but does
    // have pstat_getproc().

#ifdef __LWI_FREEBSD__
#define LSA_UM_PROCESS_PATH "/dev/null"
#else
#define LSA_UM_PROCESS_PATH NULL
#endif

#ifdef KERN_PROC_RUID
#define LSA_UM_PROCESS_RUID KERN_PROC_RUID
#elif defined(KINFO_PROC_RUID)
#define LSA_UM_PROCESS_RUID KINFO_PROC_RUID
#else
#error "unsupported KVM platform"
#endif

DWORD
LsaUmpIsUserActive(
    IN  uid_t     uUid,
    OUT BOOLEAN * pbUserIsActive
    )
{
    DWORD   dwError = 0;
    BOOLEAN bFoundUserProcess = FALSE;
    int     filteredCount = 0;
    kvm_t * kd = NULL;
    struct  kinfo_proc *procs = NULL;

    // Use of /dev/null is specific to FreeBSD.
    kd = kvm_open(NULL,
                  LSA_UM_PROCESS_PATH,
                  NULL,
                  O_RDONLY,
                  NULL);
    if (kd == NULL)
    {
        LSA_LOG_DEBUG("LSA User Manager - kvm_open reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    procs = kvm_getprocs(
                kd,
                LSA_UM_PROCESS_RUID,
                uUid,
                &filteredCount);

    if ( procs && filteredCount > 0 )
    {
        bFoundUserProcess = TRUE;
    }

    *pbUserIsActive = bFoundUserProcess;

cleanup:

    if ( kd )
        kvm_close(kd);

    return dwError;

error:

    *pbUserIsActive = TRUE;

    goto cleanup;
}

#elif defined(__LWI_LINUX__)
    // Linux has a /proc pseudo filesystem but not
    // /proc/pid/psinfo files.

DWORD
LsaUmpIsUserActive(
    IN  uid_t     uUid,
    OUT BOOLEAN * pbUserIsActive
    )
{
    DWORD           dwError = 0;
    BOOLEAN         bFoundUserProcess = FALSE;
    DIR *           dir = NULL;
    struct dirent * dirEntry = NULL;
    PSTR            filePath = NULL;
    struct stat     compareStat;
    int             result = 0;

    if ((dir = opendir("/proc")) == NULL)
    {
        LSA_LOG_DEBUG("LSA User Manager - opendir reported %u", errno);
        LSA_LOG_ERROR("LSA User Manager - unable to determine whether users have logged off.");
        dwError = LW_ERROR_CANNOT_DETECT_USER_PROCESSES;
    }
    BAIL_ON_LSA_ERROR(dwError);

    while (1)
    {
        errno = 0;

        dirEntry = readdir(dir);
        if (dirEntry == NULL)
            break;

        if ( dirEntry->d_name[0] == '.' )
            continue;

        if(!isdigit((int)dirEntry->d_name[0]))
            continue;

        LW_SAFE_FREE_STRING(filePath);
        dwError = LwAllocateStringPrintf(
                      &filePath,
                      "/proc/%s",
                      dirEntry->d_name);
        BAIL_ON_LSA_ERROR(dwError);

        while( (result = stat(filePath, &compareStat)) < 0 )
        {
            if ( errno != EINTR )
            {
                break;
            }
        }
        if ( result == 0 )
        {
            if ( uUid == compareStat.st_uid )
            {
                bFoundUserProcess = TRUE;
                break;
            }
        }
    }

    *pbUserIsActive = bFoundUserProcess;

cleanup:

    LW_SAFE_FREE_STRING(filePath);

    if ( dir )
        closedir(dir);

    return dwError;

error:

    *pbUserIsActive = TRUE;

    goto cleanup;
}

#else
#error "unsupported platform"
#endif

