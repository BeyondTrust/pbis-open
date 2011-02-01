/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

/* Adapted from pvfs */
static
VOID
LwIoFuseMapAccessFromPosix(
    IN OUT PACCESS_MASK pAccess,
    IN     BOOLEAN bIsDirectory,
    IN     mode_t Mode,
    IN     mode_t Read,
    IN     mode_t Write,
    IN     mode_t Execute
    )
{
    ACCESS_MASK Access = *pAccess;

    if (Mode & Read)
    {
        Access |= FILE_GENERIC_READ;

        if (bIsDirectory)
        {
            Access |= FILE_LIST_DIRECTORY;
        }
    }

    if (Mode & Write)
    {
        Access |= (FILE_GENERIC_WRITE|DELETE);

        if (bIsDirectory)
        {
            Access |= (FILE_DELETE_CHILD|FILE_ADD_SUBDIRECTORY);
        }
    }

    if (Mode & Execute)
    {
        Access |= FILE_GENERIC_EXECUTE;

        if (bIsDirectory)
        {
            Access |= FILE_TRAVERSE;
        }
    }

    if ((Mode & Read) && (Mode & Write) && (Mode & Execute))
    {
        /* Force the caller to decide if we need to include
           WRITE_OWNER|WRITE_DAC */

        Access = FILE_ALL_ACCESS & ~(WRITE_OWNER|WRITE_DAC);
    }

    *pAccess = Access;

    return;
}

static
NTSTATUS
LwIoFuseGetDaclFromPosix(
    PACL* ppDacl,
    struct stat* pStat
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSizeDacl = 0;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSID pEveryoneSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;
    ACCESS_MASK AccessMask = 0;
    BOOLEAN bIsDir = S_ISDIR(pStat->st_mode);
    PLW_MAP_SECURITY_CONTEXT pContext = NULL;

    status = LwMapSecurityCreateContext(&pContext);
    BAIL_ON_NT_STATUS(status);

    status = LwMapSecurityGetSidFromId(pContext, &pOwnerSid, TRUE, pStat->st_uid);
    BAIL_ON_NT_STATUS(status);
    dwSidCount++;

    status = LwMapSecurityGetSidFromId(pContext, &pGroupSid, FALSE, pStat->st_gid);
    BAIL_ON_NT_STATUS(status);
    dwSidCount++;

    status = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(status);
    dwSidCount++;


    dwSizeDacl = ACL_HEADER_SIZE +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pOwnerSid) +
        RtlLengthSid(pGroupSid) +
        RtlLengthSid(pEveryoneSid) -
        dwSidCount * sizeof(ULONG);

    status= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(status);

    status = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(status);

    AccessMask = 0;
    LwIoFuseMapAccessFromPosix(
        &AccessMask,
        bIsDir,
        pStat->st_mode,
        S_IRUSR,
        S_IWUSR,
        S_IXUSR);
    AccessMask |= (READ_CONTROL|WRITE_DAC|WRITE_OWNER);
    status = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pOwnerSid);
    BAIL_ON_NT_STATUS(status);

    AccessMask = 0;
    LwIoFuseMapAccessFromPosix(
        &AccessMask,
        bIsDir,
        pStat->st_mode,
        S_IRGRP,
        S_IWGRP,
        S_IXGRP);
    AccessMask |= (pStat->st_mode & S_ISGID) ? WRITE_DAC : 0;
    status = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pGroupSid);
    BAIL_ON_NT_STATUS(status);

    AccessMask = 0;
    LwIoFuseMapAccessFromPosix(
        &AccessMask,
        bIsDir,
        pStat->st_mode,
        S_IROTH,
        S_IWOTH,
        S_IXOTH);
    status = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pEveryoneSid);
    BAIL_ON_NT_STATUS(status);

    *ppDacl = pDacl;
    pDacl = NULL;

cleanup:

    LwMapSecurityFreeContext(&pContext);

    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pEveryoneSid);
    LW_RTL_FREE(&pDacl);

    return status;

error:

    goto cleanup;
}

NTSTATUS
LwIoFuseChmod(
    const char* pszPath,
    mode_t mode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pDacl = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pRelative = NULL;
    ULONG ulLength = 0;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_FILE_NAME filename = {0};
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    struct stat statbuf = {0};

    status = LwIoFuseGetattr(pszPath, &statbuf);
    BAIL_ON_NT_STATUS(status);

    statbuf.st_mode = mode;

    status = RTL_ALLOCATE(&pAbsolute, VOID, SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(pAbsolute, SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseGetDaclFromPosix(&pDacl, &statbuf);
    BAIL_ON_NT_STATUS(status);

    status = RtlSetDaclSecurityDescriptor(pAbsolute, TRUE, pDacl, FALSE);
    BAIL_ON_NT_STATUS(status);

    status = RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        NULL,
        &ulLength);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        BAIL_ON_NT_STATUS(status);
    }
    status = STATUS_SUCCESS;

    status = RTL_ALLOCATE(&pRelative, VOID, ulLength);
    BAIL_ON_NT_STATUS(status);

    status = RtlAbsoluteToSelfRelativeSD(
        pAbsolute,
        pRelative,
        &ulLength);
    BAIL_ON_NT_STATUS(status);

    pFuseContext = LwIoFuseGetContext();

    status = LwIoFuseGetNtFilename(
        pFuseContext,
        pszPath,
        &filename);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,                 /* File handle */
        NULL,                    /* Async control block */
        &ioStatus,               /* IO status block */
        &filename,               /* Filename */
        NULL,                    /* Security descriptor */
        NULL,                    /* Security QOS */
        WRITE_DAC | FILE_READ_ATTRIBUTES, /* Desired access mask */
        0,                       /* Allocation size */
        0,                       /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,       /* Share access */
        FILE_OPEN,               /* Create disposition */
        0,                       /* Create options */
        NULL,                    /* EA buffer */
        0,                       /* EA length */
        NULL,                     /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);

    status = LwNtSetSecurityFile(
        handle, /* File handle */
        NULL, /* Async control block */
        &ioStatus, /* IO status block */
        DACL_SECURITY_INFORMATION,
        pRelative,
        ulLength);
    BAIL_ON_NT_STATUS(status);

error:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_UNICODE_STRING_FREE(&filename.Name);
    RTL_FREE(&pDacl);
    RTL_FREE(&pRelative);
    RTL_FREE(&pAbsolute);

    return status;
}

