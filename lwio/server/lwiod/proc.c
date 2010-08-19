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
 *        proc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Function to lookup the command line of a pid
 *
 * Authors: Kyle Stemen (kstemen@likewise.com)
 */
#include "includes.h"

DWORD
SMBSrvGetExecutableStatByPid(
    IN  pid_t     pid,
    OUT struct stat*     pStat
    )
{
    DWORD           dwError = 0;

#ifdef HAVE_STRUCT_PSINFO
    // Solaris and AIX have a /proc pseudo filesystem with
    // /proc/pid/psinfo files. Both of those distros also have
    // /proc/pid/object/a.out.

    PSTR pszFilePath = NULL;

    dwError = SMBAllocateStringPrintf(
                  &pszFilePath,
                  "/proc/%d/object/a.out",
                  pid);
    BAIL_ON_LWIO_ERROR(dwError);

    if ( stat(pszFilePath, pStat) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    LWIO_SAFE_FREE_STRING(pszFilePath);

    return dwError;

#elif defined(HAVE_PSTAT_GETPROC)
    // HPUX has no /proc pseudo filesystem but does
    // have pstat_getproc().

    struct pst_status processStatus;
    struct pst_filedetails fileDetails;

    if (pstat_getproc(&processStatus,
                   sizeof(processStatus),
                   0,
                   pid) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    if (pstat_getfiledetails(&fileDetails,
                    sizeof(fileDetails),
                    &processStatus.pst_fid_text) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Fill in the key fields of struct stat, and leave the rest as 0
    memset(pStat, 0, sizeof(pStat));
    pStat->st_dev = fileDetails.psfd_dev;
    pStat->st_ino = fileDetails.psfd_ino;
    pStat->st_mode = fileDetails.psfd_mode;
    pStat->st_nlink = fileDetails.psfd_nlink;
    pStat->st_rdev = fileDetails.psfd_rdev;
    pStat->st_size = fileDetails.psfd_size;
    pStat->st_atime = fileDetails.psfd_atime;
    pStat->st_mtime = fileDetails.psfd_mtime;
    pStat->st_ctime = fileDetails.psfd_ctime;
    pStat->st_blksize = fileDetails.psfd_blksize;
    pStat->st_blocks = fileDetails.psfd_blocks;
    pStat->st_fstype = processStatus.pst_text.psf_fsid.psfs_type;
    pStat->st_realdev = processStatus.pst_text.psf_fsid.psfs_id;
    pStat->st_uid = fileDetails.psfd_uid;
    pStat->st_gid = fileDetails.psfd_gid;

cleanup:

    return dwError;

#elif HAVE_DECL_KERN_PROC_PATHNAME
    // FreeBSD has this

    int iGetProcInfo[] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROC_PID,
        pid
    };
    int iGetPathname[] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROC_PATHNAME,
        pid
    };
    struct kinfo_proc procInfo = {0};
    size_t sProcInfo = sizeof(procInfo);
    size_t sPathLen = 0;
    PSTR pszPathname = NULL;

    if (sysctl(
                iGetProcInfo,
                sizeof(iGetProcInfo)/sizeof(iGetProcInfo[0]),
                &procInfo,
                &sProcInfo,
                NULL,
                0) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }
    
    if (sProcInfo != sizeof(procInfo))
    {
        dwError = EINVAL;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // On FreeBSD 6.1, if one tries to get the pathname of a process where
    // ki_textvp is NULL, it crashes the kernel, even if the program is running
    // as a non-root user.
    if (procInfo.ki_textvp == NULL)
    {
        dwError = ENOENT;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Figure out how long the program path is
    if (sysctl(
                iGetPathname,
                sizeof(iGetPathname)/sizeof(iGetPathname[0]),
                NULL,
                &sPathLen,
                NULL,
                0) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    dwError = SMBAllocateMemory(
                sPathLen,
                (PVOID*)&pszPathname);
    BAIL_ON_LWIO_ERROR(dwError);

    // Grab the pathname
    if (sysctl(
                iGetPathname,
                sizeof(iGetPathname)/sizeof(iGetPathname[0]),
                pszPathname,
                &sPathLen,
                NULL,
                0) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    if ( stat(pszPathname, pStat) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    LWIO_SAFE_FREE_STRING(pszPathname);
    return dwError;

#elif HAVE_DECL_KERN_PROCARGS
    // Mac is like this.
    // On this system, it is possible to get the kernel vnode ptr of the
    // executable through kvm_getprocs or sysctl, but it is not possible to
    // lookup the file path of the vnode ptr short of reading kernel
    // structures directly from kernel memory. The best we can do is hope
    // argv[0] is an absolute path. Note: a program can change its arguments
    // at runtime, so this information can't be trusted for malicious programs.

    char szArgumentBuffer[1024];
    int iMib[] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROCARGS,
        pid
    };
    size_t sBufferLen = sizeof(szArgumentBuffer);

    if (sysctl(
                iMib,
                sizeof(iMib)/sizeof(iMib[0]),
                szArgumentBuffer,
                &sBufferLen,
                NULL,
                0) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

    // Make sure the result is null terminated
    szArgumentBuffer[sBufferLen - 1] = 0;

    // szArgumentBuffer is a null separated array of program arguments. The
    // first argument is the program name.
    if ( stat(szArgumentBuffer, pStat) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    return dwError;

#elif defined(__LWI_LINUX__) || defined(__LWI_NETBSD__)
    // Linux has a /proc pseudo filesystem but not
    // /proc/pid/psinfo files.

    PSTR pszFilePath = NULL;

    dwError = SMBAllocateStringPrintf(
                  &pszFilePath,
                  "/proc/%d/exe",
                  pid);
    BAIL_ON_LWIO_ERROR(dwError);

    if ( stat(pszFilePath, pStat) < 0)
    {
        dwError = errno;
        BAIL_ON_LWIO_ERROR(dwError);
    }

cleanup:

    LWIO_SAFE_FREE_STRING(pszFilePath);

    return dwError;

#else

#error no proc support for this platform

#endif

error:

    if (pStat != NULL)
    {
        memset(pStat, 0, sizeof(*pStat));
    }

    goto cleanup;
}
