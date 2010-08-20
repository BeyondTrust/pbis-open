#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <lw/base.h>
#include <lwio/lwio.h>
#include <wc16str.h>

int
main(
    int    argc,
    char** ppszArgv
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CHAR szHostname[256] = {0};
    IO_FILE_HANDLE hDir = NULL;
    IO_FILE_HANDLE hFile = NULL;
    PIO_ASYNC_CONTROL_BLOCK pAcb = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_NAME fileName = {0};
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;

    if (gethostname(szHostname, sizeof(szHostname) -1) != 0)
    {
        status = LwErrnoToNtStatus(errno);
        if (status)
        {
            goto error;
        }
    }

#if 0
    status = LwRtlWC16StringAllocatePrintfW(
                    &fileName.FileName,
                    L"/rdr/%s/C$/testdir",
                    szHostname);
#else
    status = LwRtlWC16StringAllocateFromCString(
                    &fileName.FileName,
                    "/pvfs/lwcifs/testdir");
#endif

    if (status)
    {
        goto error;
    }

    // Create the directory
    status = LwNtCreateFile(
                    &hDir,
                    pAcb,
                    &ioStatusBlock,
                    &fileName,
                    pSecDesc,                           /* SecurityDescriptor */
                    NULL,                               /* Security QOS       */
                    WRITE_OWNER|WRITE_DAC|READ_CONTROL|DELETE, /* Desired Access     */
                    0,                                  /* AllocationSize     */
                    FILE_ATTRIBUTE_NORMAL,              /* FileAttributes     */
                    0,                                  /* ShareAccess: None  */
                    FILE_OPEN_IF,                       /* CreateDisposition  */
                    FILE_DIRECTORY_FILE|FILE_DELETE_ON_CLOSE,/* CreateOptions */
                    NULL,                               /* EaBuffer           */
                    0,                                  /* EaLength           */
                    NULL                                /* EcpList            */
                    );
    if (status)
    {
        goto error;
    }

    RTL_FREE(&fileName.FileName);

    // Create the file
    fileName.RootFileHandle = hDir;
    status = LwRtlWC16StringAllocateFromCString(&fileName.FileName, "testfile");
    if (status)
    {
        goto error;
    }

    status = LwNtCreateFile(
                    &hFile,
                    pAcb,
                    &ioStatusBlock,
                    &fileName,
                    pSecDesc,                           /* SecurityDescriptor */
                    NULL,                               /* Security QOS       */
                    WRITE_OWNER|WRITE_DAC|READ_CONTROL|DELETE, /* Desired Access     */
                    0,                                  /* AllocationSize     */
                    FILE_ATTRIBUTE_NORMAL,              /* FileAttributes     */
                    0,                                  /* ShareAccess: None  */
                    FILE_OPEN_IF,                       /* CreateDisposition  */
                    FILE_NON_DIRECTORY_FILE|FILE_DELETE_ON_CLOSE,/* CreateOptions */
                    NULL,                               /* EaBuffer           */
                    0,                                  /* EaLength           */
                    NULL                                /* EcpList            */
                    );
    if (status)
    {
        goto error;
    }

cleanup:

    RTL_FREE(&fileName.FileName);

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    if (hDir)
    {
        LwNtCloseFile(hDir);
    }

    return status;

error:

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

