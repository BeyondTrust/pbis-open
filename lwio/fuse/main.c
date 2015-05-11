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

#define LWIO_OPT_KEY(t, p, v) { t, offsetof(IO_FUSE_CONTEXT, p), v }

enum
{
    KEY_VERSION,
    KEY_HELP
};

static struct fuse_opt lwio_opts[] =
{
    LWIO_OPT_KEY("--path %s", pszUncPath, 0),
    LWIO_OPT_KEY("--user %s", pszUsername, 0),
    LWIO_OPT_KEY("--domain %s", pszDomain, 0),
    LWIO_OPT_KEY("--password %s", pszPassword, 0),
    LWIO_OPT_KEY("-h", bHelp, 1),
    LWIO_OPT_KEY("--help", bHelp, 1),
    FUSE_OPT_END
};

static
void
show_help()
{
    printf("lwio-fuse-mount: mount lwio path onto filesystem\n"
           "\n"
           "Usage: lwio-fuse-mount --path unc_path mount_point\n"
           "\n"
           "Options:\n"
           "\n"
           "    --path unc_path           Specify UNC path\n"
           "    --user   name             User to log in as\n"
           "    --domain name             Domain of user\n"
           "    --password password       Password for user\n"
           "\n");
}

int
main(int argc,
     char** argv
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_FUSE_CONTEXT pFuseContext = NULL;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    PWSTR pwszUncPath = NULL;
    static WCHAR wszCredPrefix[] = {'/', 'r', 'd', 'r', '/', 0};
    UNICODE_STRING credPrefix = LW_RTL_CONSTANT_STRING(wszCredPrefix);
    struct termios oldFlags, newFlags;
    FILE* ttyIn = stdin;
    FILE* ttyOut = stdout;
    char szPassword[256] = {0};
#if 0
    IO_FILE_HANDLE handle = NULL;
    IO_FILE_NAME filename = {0};
    IO_STATUS_BLOCK ioStatus = {0};
#endif

    status = RTL_ALLOCATE(&pFuseContext, IO_FUSE_CONTEXT, sizeof(*pFuseContext));
    BAIL_ON_NT_STATUS(status);

    if (fuse_opt_parse(&args, pFuseContext, lwio_opts, NULL) == -1)
    {
        goto error;
    }

    if (pFuseContext->bHelp)
    {
        show_help();
        return 0;
    }

    if (!pFuseContext->pszUncPath)
    {
        printf("Error: No UNC path specified\n");
        goto error;
    }

    status = LwRtlWC16StringAllocateFromCString(
        &pwszUncPath,
        pFuseContext->pszUncPath);
    BAIL_ON_NT_STATUS(status);

    status = LwIoUncPathToInternalPath(
        pwszUncPath,
        &pFuseContext->pwszInternalPath);
    BAIL_ON_NT_STATUS(status);

    pFuseContext->ownerUid = geteuid();
    pFuseContext->ownerGid = getegid();

    if (pFuseContext->pszUsername)
    {
        if (!pFuseContext->pszPassword)
        {
            tcgetattr(fileno(ttyIn), &oldFlags);
            memcpy(&newFlags, &oldFlags, sizeof(newFlags));
            newFlags.c_lflag &= ~(ECHO);
            tcsetattr(fileno(ttyIn), TCSANOW, &newFlags);
            fprintf(ttyOut, "Password for %s: ", pFuseContext->pszUsername);
            fflush(ttyOut);
            pFuseContext->pszPassword = fgets(szPassword, sizeof(szPassword), ttyIn);
            if (szPassword[strlen(szPassword) - 1] == '\n')
            {
                szPassword[strlen(szPassword) - 1] = '\0';
            }
            fprintf(ttyOut, "\n");
            tcsetattr(fileno(ttyIn), TCSANOW, &oldFlags);
        }

        status = LwIoCreatePlainCredsA(
            pFuseContext->pszUsername,
            pFuseContext->pszDomain,
            pFuseContext->pszPassword,
            &pFuseContext->pCreds);
        BAIL_ON_NT_STATUS(status);

        status = LwIoSetPathCreds(&credPrefix, pFuseContext->pCreds);
        BAIL_ON_NT_STATUS(status);
    }
  
#if 0
    /* Perform a test open of / to catch any obvious problems now rather than
     * when servicing a syscall */
    status = LwIoFuseGetNtFilename(
        pFuseContext,
        "/",
        &filename);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,               /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &filename,             /* Filename */
        NULL,                  /* Security descriptor */
        NULL,                  /* Security QOS */
        READ_CONTROL | FILE_READ_ATTRIBUTES,  /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,             /* Create disposition */
        FILE_DIRECTORY_FILE,   /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL,                  /* ECP list */
        NULL);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCloseFile(handle);
    handle = NULL;
    BAIL_ON_NT_STATUS(status);
    RTL_FREE(&filename.FileName);
#endif

    return fuse_main(args.argc, args.argv, LwIoFuseGetOperationsTable(), pFuseContext);

error:

    fprintf(stderr, "Error: %s (0x%x)\n", LwNtStatusToName(status), (unsigned int) status);

#if 0
    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&filename.FileName);
#endif

    return 1;
}

