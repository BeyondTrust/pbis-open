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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Test Program for exercising the PVFS driver
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "includes.h"

/***********************************************************************
 **********************************************************************/

int
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Check Arg count */

    if (argc <= 1) {
        PrintUsage(argv[0]);
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Process Args */

    if (strcmp(argv[1], "-c") == 0)
    {
        ntError = CopyFileToPvfs(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "-C") == 0)
    {
        ntError = CopyFileFromPvfs(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "--cat") == 0)
    {
        ntError = CatFileFromPvfs(argv[2]);
    }
    else if (strcmp(argv[1], "--notify") == 0)
    {
        ntError = TestReadDirectoryChange(argv[2]);
    }
    else if (strcmp(argv[1], "-O") == 0)
    {
        ntError = RequestOplock(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "-S") == 0)
    {
        ntError = StatRemoteFile(argv[2]);
    }
    else if (strcmp(argv[1], "-F") == 0)
    {
        ntError = SetEndOfFile(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "-D") == 0)
    {
        ntError = DeletePath(argv[2]);
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        char *filter = NULL;

        if ((argc-2) > 1) {
            filter = argv[3];
        }

        ntError = ListDirectory(argv[2], filter);
    }
    else if (strcmp(argv[1], "-L") == 0)
    {
        ntError = LockTest(argv[2]);
    }
    else if (strcmp(argv[1], "--ls-open-files") == 0)
    {
        ntError = ListOpenFiles(argv[2]);
    }
    else if (strcmp(argv[1], "--max-open-files") == 0)
    {
        ntError = PrintMaxOpenFiles();
    }
    else if (strcmp(argv[1], "--get-security") == 0)
    {
        ntError = GetFileSecurity(argv[2]);
    }
    else if (strcmp(argv[1], "--set-security") == 0)
    {
        ntError = SetFileSecurity(argc-2, argv+2);
    }
    else
    {
        PrintUsage(argv[0]);
        ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    if (ntError != STATUS_SUCCESS)
    {        
        printf("Final NTSTATUS was %s (%s)\n",
               NtStatusToDescription(ntError),
               NtStatusToName(ntError));
    }

    return ntError == STATUS_SUCCESS;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

void
PrintUsage(
    char *pszProgName
    )
{
    fprintf(stderr, "Usage: %s <command> [command options]\n", pszProgName);
    fprintf(stderr, "(All pvfs files should be given in the format \"/pvfs/path/...\")\n");
    fprintf(stderr, "    -c <src> <dst>                 Copy src to the Pvfs dst file\n");
    fprintf(stderr, "    -C <src> <dst>                 Copy the pvfs src file to the local dst file\n");
    fprintf(stderr, "    -S <path>                      Stat a Pvfs path (directory or file)\n");
    fprintf(stderr, "    -l <dir>                       List the files in a directory\n");
    fprintf(stderr, "    -F <file> <size>               Set the end-of-file\n");
    fprintf(stderr, "    -D <path>                      Delete a file or directory using delete-on-close\n");
    fprintf(stderr, "    -L <filename>                  Locking Tests\n");
    fprintf(stderr, "    -O <filename>                  Oplock Test\n");
    fprintf(stderr, "    --ls-open-files <level>        List open files\n");
    fprintf(stderr, "    --max-open-files               Print the maximum number of concurrent open fds\n");
    fprintf(stderr, "    --get-security <file>          Print the file security information in sddl format\n");
    fprintf(stderr, "    --set-security <sddl> <file>   Set the file security information using the sddl string\n");

    fprintf(stderr, "\n");

    return;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
CopyFileToPvfs(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME DstFilename = {0};
    PSTR pszSrcPath = NULL;
    PSTR pszDstPath = NULL;
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];

    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <src> and <dst>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszSrcPath = argv[0];
    pszDstPath = argv[1];

    ntError = RtlUnicodeStringAllocateFromCString(&DstFilename.Name, pszDstPath);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote Destination file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &DstFilename,
                           NULL,
                           NULL,
                           FILE_GENERIC_WRITE,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OVERWRITE,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the local source */

    if ((fd = open(pszSrcPath, O_RDONLY, 0)) == -1)
    {
        fprintf(stderr, "Failed to open local file \"%s\" for copy (%s).\n",
                pszDstPath, strerror(errno));
        ntError = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Copy the file */

    do {
        if ((bytes = read(fd, pBuffer, sizeof(pBuffer))) == -1)
        {
            fprintf(stderr, "Read failed! (%s)\n", strerror(errno));
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }

        if (bytes == 0) {
            break;
        }

        ntError = NtWriteFile(hFile,
                             NULL,
                             &StatusBlock,
                             pBuffer,
                             bytes,
                             0, 0);
        BAIL_ON_NT_STATUS(ntError);

    } while (bytes != 0);

    close(fd);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_UNICODE_STRING_FREE(&DstFilename.Name);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    if (fd != -1) {
        close(fd);
    }
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
CopyFileFromPvfs(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME SrcFilename = {0};
    PSTR pszSrcPath = NULL;
    PSTR pszDstPath = NULL;
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];

    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <src> and <dst>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszSrcPath = argv[0];
    pszDstPath = argv[1];

    ntError = RtlUnicodeStringAllocateFromCString(&SrcFilename.Name, pszSrcPath);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &SrcFilename,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the local destination */

    if ((fd = open(pszDstPath, O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1)
    {
        fprintf(stderr, "Failed to open local file \"%s\" for copy (%s).\n",
                pszDstPath, strerror(errno));
        ntError = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Copy the file */

    do {
        ntError = NtReadFile(hFile,
                             NULL,
                             &StatusBlock,
                             pBuffer,
                             sizeof(pBuffer),
                             NULL, NULL);
        BAIL_ON_NT_STATUS(ntError);

        if (StatusBlock.BytesTransferred == 0) {
            break;
        }

        if ((bytes = write(fd, pBuffer, StatusBlock.BytesTransferred)) == -1)
        {
            fprintf(stderr, "Write failed! (%s)\n", strerror(errno));
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }


    } while (bytes != 0);

    close(fd);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_UNICODE_STRING_FREE(&SrcFilename.Name);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    if (fd != -1) {
        close(fd);
    }
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
CatFileFromPvfs(
    char *pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME SrcFilename = {0};
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];

    ntError = RtlUnicodeStringAllocateFromCString(&SrcFilename.Name, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &SrcFilename,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Read the file */

    do {
        ntError = NtReadFile(hFile,
                             NULL,
                             &StatusBlock,
                             pBuffer,
                             sizeof(pBuffer),
                             NULL, NULL);
        if (ntError == STATUS_END_OF_FILE) {
            ntError = STATUS_SUCCESS;
            break;
        }
        BAIL_ON_NT_STATUS(ntError);

        if (StatusBlock.BytesTransferred == 0) {
            break;
        }

        if ((bytes = write(1, pBuffer, StatusBlock.BytesTransferred)) == -1)
        {
            fprintf(stderr, "Write failed! (%s)\n", strerror(errno));
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }


    } while (bytes != 0);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_UNICODE_STRING_FREE(&SrcFilename.Name);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    if (fd != -1) {
        close(fd);
    }
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
StatRemoteFile(
    char *pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_BASIC_INFORMATION FileBasicInfo = {0};
    FILE_STANDARD_INFORMATION FileStdInfo = {0};
    FILE_INTERNAL_INFORMATION FileInternalInfo = {0};
    IO_FILE_NAME Filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name,
                                               pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileBasicInfo,
                                     sizeof(FileBasicInfo),
                                     FileBasicInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileStdInfo,
                                     sizeof(FileStdInfo),
                                     FileStandardInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileInternalInfo,
                                     sizeof(FileInternalInfo),
                                     FileInternalInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

    printf("Filename:             %s\n", pszFilename);

    printf("CreationTime:         %lld\n", (long long) FileBasicInfo.CreationTime);
    printf("Last Access Time:     %lld\n", (long long) FileBasicInfo.LastAccessTime);
    printf("Last Modification:    %lld\n", (long long) FileBasicInfo.LastWriteTime);
    printf("Change Time:          %lld\n", (long long) FileBasicInfo.ChangeTime);

    printf("Allocation Size:      %lld\n", (long long) FileStdInfo.AllocationSize);
    printf("File Size:            %lld\n", (long long) FileStdInfo.EndOfFile);
    printf("Number of Links:      %d\n", FileStdInfo.NumberOfLinks);
    printf("Is Directory:         %s\n", FileStdInfo.Directory ? "yes" : "no");
    printf("Pending Delete:       %s\n", FileStdInfo.DeletePending ? "yes" : "no");
    printf("Attributes:           0x%x\n", FileBasicInfo.FileAttributes);
    printf("Index Number:         %lld\n", (long long) FileInternalInfo.IndexNumber);

    printf("\n");

cleanup:
    RTL_UNICODE_STRING_FREE(&Filename.Name);

    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

VOID
PrintFileBothDirInformation(
    PFILE_BOTH_DIR_INFORMATION pFileInfo
    )
{
    NTSTATUS ntError;
    PSTR pszFilename = NULL;
    PSTR pszShortFilename = NULL;

    ntError = RtlCStringAllocateFromWC16String(&pszFilename,
                                               pFileInfo->FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringAllocateFromWC16String(&pszShortFilename,
                                               pFileInfo->ShortName);
    BAIL_ON_NT_STATUS(ntError);

    printf("Filename:             %s\n", pszFilename);
    printf("8.3 Filename:         %s\n", pszShortFilename);

    printf("CreationTime:         %lld\n", (long long) pFileInfo->CreationTime);
    printf("Last Access Time:     %lld\n", (long long) pFileInfo->LastAccessTime);
    printf("Last Modification:    %lld\n", (long long) pFileInfo->LastWriteTime);
    printf("Change Time:          %lld\n", (long long) pFileInfo->ChangeTime);

    printf("Allocation Size:      %lld\n", (long long) pFileInfo->AllocationSize);
    printf("File Size:            %lld\n", (long long) pFileInfo->EndOfFile);
    printf("Attributes:           0x%x\n", pFileInfo->FileAttributes);
    printf("EA Size:              %d\n", pFileInfo->EaSize);

    printf("\n");

cleanup:
    return;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
ListDirectory(
    char *pszDirectory,
    char *pszPattern
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_NAME Dirname = {0};
    IO_FILE_HANDLE hDir = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    PFILE_BOTH_DIR_INFORMATION pFileInfo = NULL;
    PVOID pBuffer =  NULL;
    DWORD dwBufLen = 0;
    IO_MATCH_FILE_SPEC FileSpec = { 0 };

    ntError = RtlUnicodeStringAllocateFromCString(&Dirname.Name,
                                               pszDirectory);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCreateFile(&hDir,
                           NULL,
                           &StatusBlock,
                           &Dirname,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Allocate the buffer.  Include space for four files including
       256 WCHAR length filename */

    dwBufLen = (sizeof(FILE_BOTH_DIR_INFORMATION) + (sizeof(WCHAR)*256)) * 4;
    ntError = RTL_ALLOCATE(&pBuffer, VOID, dwBufLen);
    BAIL_ON_NT_STATUS(ntError);

    if (pszPattern) {
        ntError = LwRtlUnicodeStringAllocateFromCString(&FileSpec.Pattern, pszPattern);
        BAIL_ON_NT_STATUS(ntError);
    }

    do
    {
        memset(pBuffer, 0x0, dwBufLen);
        ntError = NtQueryDirectoryFile(hDir,
                                       NULL,
                                       &StatusBlock,
                                       pBuffer,
                                       dwBufLen,
                                       FileBothDirectoryInformation,
                                       FALSE,     /* ignored */
                                       pszPattern ? &FileSpec : NULL,
                                       FALSE);
        BAIL_ON_NT_STATUS(ntError);

        pFileInfo = (PFILE_BOTH_DIR_INFORMATION)pBuffer;

        while (pFileInfo != NULL)
        {
            PrintFileBothDirInformation(pFileInfo);
            pFileInfo = (pFileInfo->NextEntryOffset != 0) ?
                        (PVOID)pFileInfo + pFileInfo->NextEntryOffset :
                        NULL;
        }
    } while (NT_SUCCESS(ntError));



    ntError = NtCloseFile(hDir);
    hDir = (IO_FILE_HANDLE)NULL;
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_UNICODE_STRING_FREE(&Dirname.Name);

    return ntError;

error:
    if (hDir) {
        NtCloseFile(hDir);
    }
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DeletePath(
    char *pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    
    BAIL_ON_NT_STATUS(ntError);
    
cleanup:
    return ntError;
    
error:
    goto cleanup;
}
    

/***********************************************************************
 **********************************************************************/

NTSTATUS
SetEndOfFile(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_BASIC_INFORMATION FileBasicInfo = {0};
    FILE_STANDARD_INFORMATION FileStdInfo = {0};
    FILE_INTERNAL_INFORMATION FileInternalInfo = {0};
    FILE_END_OF_FILE_INFORMATION FileEndOfFileInfo = {0};    
    IO_FILE_NAME Filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    LONG64 EndOfFile = 0;
    PSTR pszFilename = NULL;
    char *p = NULL;    

    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <file> and <size>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszFilename = argv[0];
    EndOfFile   = strtol(argv[1], &p, 10);    

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name,
                                               pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           FILE_GENERIC_WRITE,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN_IF,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);
    
    FileEndOfFileInfo.EndOfFile = EndOfFile;    
    ntError = NtSetInformationFile(hFile,
                                   NULL,
                                   &StatusBlock,
                                   &FileEndOfFileInfo,
                                   sizeof(FileEndOfFileInfo),
                                   FileEndOfFileInformation);
    BAIL_ON_NT_STATUS(ntError);
    

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileBasicInfo,
                                     sizeof(FileBasicInfo),
                                     FileBasicInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileStdInfo,
                                     sizeof(FileStdInfo),
                                     FileStandardInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileInternalInfo,
                                     sizeof(FileInternalInfo),
                                     FileInternalInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

    printf("Filename:             %s\n", pszFilename);

    printf("CreationTime:         %lld\n", (long long) FileBasicInfo.CreationTime);
    printf("Last Access Time:     %lld\n", (long long) FileBasicInfo.LastAccessTime);
    printf("Last Modification:    %lld\n", (long long) FileBasicInfo.LastWriteTime);
    printf("Change Time:          %lld\n", (long long) FileBasicInfo.ChangeTime);

    printf("Allocation Size:      %lld\n", (long long) FileStdInfo.AllocationSize);
    printf("File Size:            %lld\n", (long long) FileStdInfo.EndOfFile);
    printf("Number of Links:      %d\n", FileStdInfo.NumberOfLinks);
    printf("Is Directory:         %s\n", FileStdInfo.Directory ? "yes" : "no");
    printf("Pending Delete:       %s\n", FileStdInfo.DeletePending ? "yes" : "no");
    printf("Attributes:           0x%x\n", FileBasicInfo.FileAttributes);
    printf("Index Number:         %lld\n", (long long) FileInternalInfo.IndexNumber);

    printf("\n");



cleanup:
    return ntError;

error:
    goto cleanup;
}    

/***********************************************************************
 **********************************************************************/

NTSTATUS
LockTest(
    char *pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_NAME Filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name,
                                               pszPath);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN_IF,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtLockFile(hFile,
                         NULL,
                         &StatusBlock,
                         0, 10, 0,
                         FALSE,
                         TRUE);    
    BAIL_ON_NT_STATUS(ntError);

    sleep(5);    

    ntError = NtUnlockFile(hFile,
                           NULL,
                           &StatusBlock,
                           0, 10, 0);
    BAIL_ON_NT_STATUS(ntError);
    
cleanup:
    if (hFile) {
        NtCloseFile(hFile);
    }

    return ntError;

error:
    goto cleanup;    
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
RequestOplock(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszFilename = NULL;
    IO_FILE_NAME Filename = {0};
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER OplockRequestInput = {0};
    IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER OplockRequestOutput = {0};
    IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER OplockBreakInput = {0};
    IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER OplockBreakOutput = {0};
    ULONG SleepCount = 5;

    if (argc != 1)
    {
        fprintf(stderr, "Missing parameter. Requires <file>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszFilename = argv[0];

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(
                  &hFile,
                  NULL,
                  &StatusBlock,
                  &Filename,
                  NULL,
                  NULL,
                  FILE_GENERIC_READ,
                  0,
                  FILE_ATTRIBUTE_NORMAL,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  FILE_OPEN,
                  FILE_NON_DIRECTORY_FILE,
                  NULL,
                  0,
                  NULL,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    OplockRequestInput.OplockRequestType = IO_OPLOCK_REQUEST_OPLOCK_BATCH;

    ntError = NtFsControlFile(
                  hFile,
                  NULL,
                  &StatusBlock,
                  IO_FSCTL_OPLOCK_REQUEST,
                  (PVOID)&OplockRequestInput,
                  sizeof(OplockRequestInput),
                  (PVOID)&OplockRequestOutput,
                  sizeof(OplockRequestOutput));
    BAIL_ON_NT_STATUS(ntError);    
    
    printf("Oplock broken! (%d)\n", OplockRequestOutput.OplockBreakResult);

    OplockBreakInput.Response = IO_OPLOCK_BREAK_ACKNOWLEDGE;
    
    ntError = NtFsControlFile(
                  hFile,
                  NULL,
                  &StatusBlock,
                  IO_FSCTL_OPLOCK_BREAK_ACK,
                  (PVOID)&OplockBreakInput,
                  sizeof(OplockBreakInput),
                  (PVOID)&OplockBreakOutput,
                  sizeof(OplockBreakOutput));
    BAIL_ON_NT_STATUS(ntError);

    printf("Oplock broken!\n");
    
    printf("Sleeping for %d seconds...\n", SleepCount);
    sleep(SleepCount);
    printf("awake\n");

cleanup:
    RTL_UNICODE_STRING_FREE(&Filename.Name);

    if (hFile) {
        NtCloseFile(hFile);
    }

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PrintOpenFileInfo0(
    PVOID pBuffer
    );

static
NTSTATUS
PrintOpenFileInfo100(
    PVOID pBuffer
    );

NTSTATUS
ListOpenFiles(
    char *pszInfoLevel
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ULONG Level = 0;
    PSTR pszString = NULL;
    IO_OPEN_FILE_INFO_INPUT_BUFFER InputBuffer = {0};
    PVOID pOutputBuffer = NULL;
    IO_STATUS_BLOCK Status = {0};
    IO_FILE_NAME FileName = {0};
    IO_FILE_HANDLE hDevice = (IO_FILE_HANDLE)NULL;
    ULONG AllocSize = 4096;

    if (pszInfoLevel)
    {
        Level = strtol(pszInfoLevel, &pszString, 10);
        if (pszString != NULL && *pszString != '\0')
        {
            printf("Invalid infor level (%s)\n", pszInfoLevel);
            ntError = STATUS_INVALID_INFO_CLASS;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = LwRtlUnicodeStringAllocateFromCString(
                  &FileName.Name,
                  "\\pvfs");
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCreateFile(
                  &hDevice,
                  NULL,
                  &Status,
                  &FileName,
                  NULL,
                  NULL,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  NULL,
                  0,
                  NULL,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    InputBuffer.Level = Level;

    do
    {
        ntError = RTL_ALLOCATE(&pOutputBuffer, VOID, AllocSize);
        BAIL_ON_NT_STATUS(ntError);

        ntError = NtDeviceIoControlFile(
                      hDevice,
                      NULL,
                      &Status,
                      IO_DEVICE_CTL_OPEN_FILE_INFO,
                      &InputBuffer,
                      sizeof(InputBuffer),
                      pOutputBuffer,
                      AllocSize);

        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            AllocSize *= 2;
            RTL_FREE(&pOutputBuffer);
        }        
    } while (ntError == STATUS_BUFFER_TOO_SMALL);
    

    switch(Level)
    {
    case 0:
        ntError = PrintOpenFileInfo0(pOutputBuffer);
        break;

    case 100:
        ntError = PrintOpenFileInfo100(pOutputBuffer);
        break;

    default:
        printf("Don't know how to print OpenFileInfo level %d\n", Level);
        ntError = STATUS_INVALID_INFO_CLASS;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (hDevice)
    {
        NtCloseFile(hDevice);
        hDevice = (IO_FILE_HANDLE)NULL;
    }

    LW_RTL_UNICODE_STRING_FREE(&FileName.Name);
    LW_RTL_FREE(&pOutputBuffer);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PrintOpenFileInfo0(
    PVOID pBuffer
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIO_OPEN_FILE_INFO_0 pInfo0 = (PIO_OPEN_FILE_INFO_0)pBuffer;
    PSTR pszFilename = NULL;
    ULONG Offset = 0;

    printf("Handles Filename\n");
    printf("------- ---------\n");
    
    while (pBuffer)
    {
        pInfo0 = (PIO_OPEN_FILE_INFO_0)(pBuffer + Offset);

        LwRtlCStringFree(&pszFilename);

        ntError = LwRtlCStringAllocateFromWC16String(
                      &pszFilename,
                      (PCWSTR)pInfo0->pwszFileName);
        BAIL_ON_NT_STATUS(ntError);

        printf("%-4d    %s\n", pInfo0->OpenHandleCount, pszFilename);

        if (pInfo0->NextEntryOffset != 0)
        {
            Offset += pInfo0->NextEntryOffset;
        }
        else
        {
            pBuffer = NULL;
        }
    }
    
cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PrintOpenFileInfo100(
    PVOID pBuffer
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIO_OPEN_FILE_INFO_100 pInfo100 = (PIO_OPEN_FILE_INFO_100)pBuffer;
    PSTR pszFilename = NULL;
    ULONG Offset = 0;

    printf("Handles Delete Filename\n");
    printf("------- ------ ---------\n");

    while (pBuffer)
    {
        pInfo100 = (PIO_OPEN_FILE_INFO_100)(pBuffer + Offset);

        LwRtlCStringFree(&pszFilename);

        ntError = LwRtlCStringAllocateFromWC16String(
                      &pszFilename,
                      (PCWSTR)pInfo100->pwszFileName);
        BAIL_ON_NT_STATUS(ntError);

        printf("%-4d    %-6s %s\n", 
               pInfo100->OpenHandleCount,
               pInfo100->bDeleteOnClose ? "Yes" : "No",
               pszFilename);

        if (pInfo100->NextEntryOffset != 0)
        {
            Offset += pInfo100->NextEntryOffset;
        }
        else
        {
            pBuffer = NULL;
        }
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

#define NOTIFY_BUFFER_SIZE 1024

NTSTATUS
TestReadDirectoryChange(
    char *pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME SrcFilename = {0};
    BYTE pBuffer[NOTIFY_BUFFER_SIZE];
    PFILE_NOTIFY_INFORMATION pChange = NULL;
    PSTR pszNotifyFilename = NULL;

    ntError = LwRtlUnicodeStringAllocateFromCString(&SrcFilename.Name, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &SrcFilename,
                           NULL,
                           NULL,
                           FILE_GENERIC_READ,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtReadDirectoryChangeFile(
                  hFile,
                  NULL,
                  &StatusBlock,
                  pBuffer,
                  NOTIFY_BUFFER_SIZE,
                  FILE_NOTIFY_CHANGE_NAME,
                  TRUE,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    pChange = (PFILE_NOTIFY_INFORMATION)pBuffer;
    ntError = LwRtlCStringAllocateFromWC16String(
                  &pszNotifyFilename,
                  pChange->FileName);

    printf("%s (Action = %d)\n", pszNotifyFilename, pChange->Action);    

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LW_RTL_UNICODE_STRING_FREE(&SrcFilename.Name);
    LwRtlCStringFree(&pszNotifyFilename);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

#define MAX_OPEN_FILE_TEST 0x00010000

NTSTATUS
PrintMaxOpenFiles(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFileArray[MAX_OPEN_FILE_TEST];
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME SrcFilename = {0};
    int i = 0;
    NTSTATUS savedStatus = STATUS_SUCCESS;

    ntError = LwRtlUnicodeStringAllocateFromCString(
                  &SrcFilename.Name, 
                  "/pvfs/tmp/max_open_file_test");
    BAIL_ON_NT_STATUS(ntError);

    for (i=0; i<MAX_OPEN_FILE_TEST; i++)
    {
        ntError = NtCreateFile(
                      &hFileArray[i],
                      NULL,
                      &StatusBlock,
                      &SrcFilename,
                      NULL,
                      NULL,
                      FILE_GENERIC_READ,
                      0,
                      FILE_ATTRIBUTE_NORMAL,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      FILE_OPEN_IF,
                      FILE_NON_DIRECTORY_FILE|FILE_DELETE_ON_CLOSE,
                      NULL,
                      0,
                      NULL,
                      NULL);
        if (ntError != STATUS_SUCCESS)
        {
            savedStatus = ntError;
        }
    }
    
    printf("Max open file limit == %d\n", i);

    for (; i>=0; i--)
    {
        NtCloseFile(hFileArray[i]);
    }

    ntError = savedStatus;

cleanup:
    LW_RTL_UNICODE_STRING_FREE(&SrcFilename.Name);

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
GetFileSecurity(
    char *pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME Filename = {0};
    BYTE pSecurityDescriptor[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    PSTR pszStringSecurityDescriptor = NULL;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           READ_CONTROL,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           0,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQuerySecurityFile(hFile,
                                  NULL,
                                  &StatusBlock,
                                  SecInfoAll,
                                  (PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptor,
                                  SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
    BAIL_ON_NT_STATUS(ntError);


    ntError = RtlAllocateSddlCStringFromSecurityDescriptor(&pszStringSecurityDescriptor,
                                                           (PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptor,
                                                           SDDL_REVISION_1,
                                                           SecInfoAll);
    BAIL_ON_NT_STATUS(ntError);

    printf("File '%s' security sddl information: \n%s\n",
           pszFilename+strlen("/pvfs"),
           pszStringSecurityDescriptor);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LwRtlCStringFree(&pszStringSecurityDescriptor);
    RTL_UNICODE_STRING_FREE(&Filename.Name);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    goto cleanup;
}

NTSTATUS
SetFileSecurity(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME Filename = {0};
    PSTR pszSddl = NULL;
    PSTR pszFilePath = NULL;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor = NULL;
    ULONG ulSecurityDescriptor = 0;
    ULONG ulSecurityDescriptorRetrieved = 0;

    BYTE pSecurityDescriptorRetrieved[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    BOOLEAN bIsSetSecuritySucceeded = TRUE;


    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <sddl> and <file>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszSddl = argv[0];
    pszFilePath = argv[1];

    ntError = RtlUnicodeStringAllocateFromCString(&Filename.Name, pszFilePath);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote Destination file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           WRITE_DAC | WRITE_OWNER | READ_CONTROL,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           0,
                           NULL,
                           0,
                           NULL,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAllocateSecurityDescriptorFromSddlCString(
                           &pSecurityDescriptor,
                           &ulSecurityDescriptor,
                           pszSddl,
                           SDDL_REVISION_1);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtSetSecurityFile(hFile,
                                NULL,
                                &StatusBlock,
                                SecInfoAll,
                                pSecurityDescriptor,
                                ulSecurityDescriptor);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQuerySecurityFile(hFile,
                                  NULL,
                                  &StatusBlock,
                                  SecInfoAll,
                                  (PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptorRetrieved,
                                  SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
    BAIL_ON_NT_STATUS(ntError);

    ulSecurityDescriptorRetrieved = RtlLengthSecurityDescriptorRelative((PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptorRetrieved);
    if (ulSecurityDescriptorRetrieved != ulSecurityDescriptor)
    {
        bIsSetSecuritySucceeded = FALSE;
    }
    else if (!LwRtlEqualMemory(pSecurityDescriptor, pSecurityDescriptorRetrieved, ulSecurityDescriptor))
    {
        bIsSetSecuritySucceeded = FALSE;
    }

    if (bIsSetSecuritySucceeded)
    {
        printf("Set security information succeeded.\n");
    }

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_UNICODE_STRING_FREE(&Filename.Name);
    RTL_FREE(&pSecurityDescriptor);

    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    goto cleanup;
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

