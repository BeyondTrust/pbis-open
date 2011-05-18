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
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszCachePath,
    PSTR*    ppszSourcePath,
    PSTR*    ppszTargetPath,
    PSTR*    ppszPrincipal,
    PSTR*    ppszDomain,
    PSTR*    ppszPassword,
    PBOOLEAN pbCopyRecursive,
    PBOOLEAN pbResolve
    );

static
NTSTATUS
LwIoReadPassword(
    PSTR* ppszPassword
    );

static
NTSTATUS
GetPassword(
    PSTR* ppszPassword
    );

static
NTSTATUS
LwIoCreateKrb5Cache(
    PCSTR pszPrincipal,
    PCSTR pszPassword,
    PSTR* ppszCachePath
    );

static
VOID
ShowUsage(
    VOID
    );

static
NTSTATUS
GetKrb5PrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    );

static
VOID
MapErrorCodes(
    NTSTATUS ntStatus
    );

static
VOID
LwIoExitHandler(
    VOID
    );

static
NTSTATUS
LwIoDestroyKrb5Cache(
    PCSTR pszCachePath
    );

int
main(
    int argc,
    char* argv[]
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszCachePath = NULL;
    PSTR pszPrincipal = NULL;
    PSTR pszPassword = NULL;
    PSTR pszDomain = NULL;
    PIO_CREDS pCreds = NULL;
    BOOLEAN bRevertThreadCreds = FALSE;
    BOOLEAN bDestroyKrb5Cache = FALSE;
    BOOLEAN bCopyRecursive = FALSE;
    BOOLEAN bResolve = FALSE;

    if (atexit(LwIoExitHandler) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = ParseArgs(
                argc,
                argv,
                &pszCachePath,
                &pszSourcePath,
                &pszTargetPath,
                &pszPrincipal,
                &pszDomain,
                &pszPassword,
                &bCopyRecursive,
                &bResolve);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!IsNullOrEmptyString(pszPrincipal))
    {
        if (!pszPassword)
        {
            ntStatus = LwIoReadPassword(&pszPassword);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!pszDomain)
        {
            LWIO_SAFE_FREE_STRING(pszCachePath);
            
            ntStatus = LwIoCreateKrb5Cache(
                pszPrincipal,
                pszPassword,
                &pszCachePath);
            BAIL_ON_NT_STATUS(ntStatus);
            
            bDestroyKrb5Cache = TRUE;
            
            ntStatus = SMBAllocateString(pszCachePath, &gpszLwioCopyKrb5CachePath);
            BAIL_ON_NT_STATUS(ntStatus);

            bDestroyKrb5Cache = FALSE;
        }
    }

    if (!IsNullOrEmptyString(pszCachePath))
    {
        ntStatus = GetKrb5PrincipalName(pszCachePath, &pszPrincipal);
        if (IsNullOrEmptyString(pszPrincipal))
        {
            fprintf(stderr,"Invalid cache path \n");
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        ntStatus = LwIoCreateKrb5CredsA(
                        pszPrincipal,
                        pszCachePath,
                        &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwIoSetThreadCreds(pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        bRevertThreadCreds = TRUE;
    }
    else
    {
        ntStatus = LwIoCreatePlainCredsA(
            pszPrincipal ? pszPrincipal : "",
            pszDomain ? pszDomain : "",
            pszPassword ? pszPassword : "",
            &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwIoSetThreadCreds(pCreds);
        BAIL_ON_NT_STATUS(ntStatus);
        
        bRevertThreadCreds = TRUE;
    }

    if (bResolve)
    {
        ntStatus = ResolveFile(pszSourcePath);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = CopyFile(pszSourcePath, pszTargetPath, bCopyRecursive);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pCreds)
    {
        if (bRevertThreadCreds)
        {
            LwIoSetThreadCreds(NULL);
        }

        LwIoDeleteCreds(pCreds);
    }

    if (bDestroyKrb5Cache)
    {
        LwIoDestroyKrb5Cache(pszCachePath);
    }

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);
    LWIO_SAFE_FREE_STRING(pszPrincipal);
    LWIO_SAFE_FREE_STRING(pszDomain);
    LWIO_SAFE_FREE_STRING(pszPassword);

    return (ntStatus);

error:

    MapErrorCodes(ntStatus);

    goto cleanup;
}

static
NTSTATUS
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszCachePath,
    PSTR*    ppszSourcePath,
    PSTR*    ppszTargetPath,
    PSTR*    ppszPrincipal,
    PSTR*    ppszDomain,
    PSTR*    ppszPassword,
    PBOOLEAN pbCopyRecursive,
    PBOOLEAN pbResolve
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    typedef enum
    {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_KRB5_CACHE_PATH,
        PARSE_MODE_UPN,
        PARSE_MODE_PASSWORD,
        PARSE_MODE_DOMAIN
    } ParseMode;
    typedef enum
    {
        LWIO_COPY_KRB5_NO_SPEC = 0,
        LWIO_COPY_KRB5_CACHE_SPECIFIED,
        LWIO_COPY_KRB5_UPN_SPECIFIED
    } LwioCopyKrb5Spec;
    int iArg = 1;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszCachePath = NULL;
    PSTR pszPrincipal = NULL;
    PSTR pszDomain = NULL;
    PSTR pszPassword = NULL;
    LwioCopyKrb5Spec krb5Spec = LWIO_COPY_KRB5_NO_SPEC;
    BOOLEAN bCopyRecursive = FALSE;
    BOOLEAN bResolve = FALSE;

    for (iArg = 1; iArg < argc; iArg++)
    {
        PSTR pszArg = argv[iArg];

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if (!strcasecmp(pszArg, "-h") || !strcasecmp(pszArg, "--help"))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (!strcasecmp(pszArg, "-r"))
                {
                    bCopyRecursive = TRUE;
                }
                else if (!strcasecmp(pszArg, "--resolve"))
                {
                    bResolve = TRUE;
                }
                else if (!strcasecmp(pszArg, "-k"))
                {
                    parseMode = PARSE_MODE_KRB5_CACHE_PATH;
                }
                else if (!strcasecmp(pszArg, "-u"))
                {
                    parseMode = PARSE_MODE_UPN;
                }
                else if (!strcasecmp(pszArg, "-p"))
                {
                    parseMode = PARSE_MODE_PASSWORD;
                }
                else if (!strcasecmp(pszArg, "-d"))
                {
                    parseMode = PARSE_MODE_DOMAIN;
                }
                else if (IsNullOrEmptyString(pszSourcePath))
                {
                    ntStatus = SMBAllocateString(
                                pszArg,
                                &pszSourcePath);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
                else if (IsNullOrEmptyString(pszTargetPath))
                {
                    ntStatus = SMBAllocateString(
                                pszArg,
                                &pszTargetPath);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
                else 
                {
                    ShowUsage();
                    fprintf(stderr, "Error: Invalid option \n");
                    exit(1);
                }
                break;

            case PARSE_MODE_KRB5_CACHE_PATH:

                if (krb5Spec == LWIO_COPY_KRB5_UPN_SPECIFIED)
                {
                    fprintf(stderr, "Attempt to specify both the kerberos cache path and user principal name\n");
                    ntStatus = STATUS_INVALID_PARAMETER;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                LWIO_SAFE_FREE_STRING(pszCachePath);

                ntStatus = SMBAllocateString(
                            pszArg,
                            &pszCachePath);
                BAIL_ON_NT_STATUS(ntStatus);

                krb5Spec = LWIO_COPY_KRB5_CACHE_SPECIFIED;

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_UPN:

                if (krb5Spec == LWIO_COPY_KRB5_CACHE_SPECIFIED)
                {
                    fprintf(stderr, "Attempt to specify both the kerberos cache path and user principal name\n");
                    ntStatus = STATUS_INVALID_PARAMETER;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                LWIO_SAFE_FREE_STRING(pszPrincipal);

                ntStatus = SMBAllocateString(
                            pszArg,
                            &pszPrincipal);
                BAIL_ON_NT_STATUS(ntStatus);

                krb5Spec = LWIO_COPY_KRB5_UPN_SPECIFIED;

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_PASSWORD:

                LWIO_SAFE_FREE_STRING(pszPassword);

                ntStatus = SMBAllocateString(
                            pszArg,
                            &pszPassword);
                BAIL_ON_NT_STATUS(ntStatus);

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_DOMAIN:

                LWIO_SAFE_FREE_STRING(pszDomain);

                ntStatus = SMBAllocateString(
                            pszArg,
                            &pszDomain);
                BAIL_ON_NT_STATUS(ntStatus);

                parseMode = PARSE_MODE_OPEN;

                break;

            default:

                ShowUsage();
                exit(0);

                break;
        }
    }

    if ( argc == 1 || 
         (!pszSourcePath && !pszTargetPath) )
	{
       ShowUsage();
       fprintf(stderr, "Error: Missing the options to be passed\n");
       exit(1);
	}


    if(pszSourcePath)
    {
        PSTR pszStr = NULL;

        pszStr = strchr (pszSourcePath,'-');

        if ( pszStr && !strcmp( pszSourcePath, pszStr) )
        {
           fprintf(stderr, "Invalid option '%s' passed\n",pszSourcePath);
           fprintf(stdout, "Try 'lwio-copy --help' for more information\n");

           pszStr = NULL;

           ntStatus = STATUS_INVALID_PARAMETER;
           BAIL_ON_NT_STATUS(ntStatus);
        }

    }

    if(pszTargetPath)
    {
        PSTR pszStr = NULL;

        pszStr = strchr (pszTargetPath,'-');

        if ( pszStr && !strcmp( pszTargetPath, pszStr) )
        {
           fprintf(stderr, "Invalid option '%s' passed\n",pszTargetPath);
           fprintf(stdout, "Try 'lwio-copy --help' for more information\n");

           pszStr = NULL;

           ntStatus = STATUS_INVALID_PARAMETER;
           BAIL_ON_NT_STATUS(ntStatus);
        }

    }

    if(!pszSourcePath)
    {
        fprintf(stderr, "Missing the path to be passed\n");
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if(!pszTargetPath && !bResolve)
    {
        fprintf(stderr, "Missing the path to be passed\n");
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppszCachePath = pszCachePath;
    *ppszSourcePath = pszSourcePath;
    *ppszTargetPath = pszTargetPath;
    *ppszPrincipal = pszPrincipal;
    *ppszDomain = pszDomain;
    *ppszPassword  = pszPassword;
    *pbCopyRecursive = bCopyRecursive;
    *pbResolve = bResolve;

cleanup:

    return ntStatus;

error:

    *ppszCachePath = NULL;
    *ppszSourcePath = NULL;
    *ppszTargetPath = NULL;
    *ppszPrincipal = NULL;
    *ppszPassword  = NULL;
    *pbCopyRecursive = FALSE;

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);
    LWIO_SAFE_FREE_STRING(pszPrincipal);
    LWIO_SAFE_FREE_STRING(pszDomain);
    LWIO_SAFE_FREE_STRING(pszPassword);

    goto cleanup;
}

static
NTSTATUS
GetKrb5PrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context    ctx = NULL;
    krb5_ccache     cc = NULL;
    krb5_principal  pKrb5Principal = NULL;
    PSTR  pszKrb5PrincipalName = NULL;
    PSTR  pszPrincipalName = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_get_principal(ctx, cc, &pKrb5Principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_unparse_name(ctx, pKrb5Principal, &pszKrb5PrincipalName);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ntStatus = SMBAllocateString(pszKrb5PrincipalName, &pszPrincipalName);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszPrincipalName = pszPrincipalName;

cleanup:

    if (ctx)
    {
        if (pszKrb5PrincipalName)
        {
            krb5_free_unparsed_name(ctx, pszKrb5PrincipalName);
        }
        if (pKrb5Principal)
        {
            krb5_free_principal(ctx, pKrb5Principal);
        }
        if (cc)
        {
            krb5_cc_close(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    return ntStatus;

error:

    *ppszPrincipalName = NULL;

    LWIO_SAFE_FREE_STRING(pszPrincipalName);

    ntStatus = STATUS_INVALID_ACCOUNT_NAME;

    goto cleanup;
}

static
NTSTATUS
LwIoReadPassword(
    PSTR* ppszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszPassword = NULL;

    fprintf(stdout, "Password: ");
    fflush(stdout);

    ntStatus = GetPassword(&pszPassword);
    BAIL_ON_NT_STATUS(ntStatus);

    fprintf(stdout, "\n");

    *ppszPassword = pszPassword;

cleanup:

    return ntStatus;

error:

    *ppszPassword = NULL;

    LWIO_SAFE_FREE_STRING(pszPassword);

    goto cleanup;
}

static
NTSTATUS
LwIoCreateKrb5Cache(
    PCSTR pszPrincipal,
    PCSTR pszPassword,
    PSTR* ppszCachePath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context    ctx = NULL;
    krb5_ccache     cc = NULL;
    krb5_principal  client = NULL;
    krb5_creds      creds = {0};
    krb5_get_init_creds_opt opts;
    PCSTR pszTempCachePath = NULL;
    PSTR  pszCachePath = NULL;
    PSTR  pszIter = NULL;

    pszIter = index(pszPrincipal, '@');
    if (!pszIter)
    {
        fprintf(stderr, "Invalid user principal [%s]\n", pszPrincipal);
        goto error;
    }

    // upper case the realm
    while (pszIter && *pszIter)
    {
        *pszIter = toupper((int)*pszIter);
        pszIter++;
    }

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_tkt_life(&opts, 12 * 60 * 60);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    /* Generates a new filed based credentials cache in /tmp. */
    ret = krb5_cc_new_unique(
           ctx,
           "FILE",
           "hint",
           &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszPrincipal, &client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_get_init_creds_password(
                    ctx,
                    &creds,
                    client,
                    (PSTR)pszPassword,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    &opts);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszTempCachePath = krb5_cc_get_name(ctx, cc);

    ntStatus = SMBAllocateString(pszTempCachePath, &pszCachePath);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszCachePath = pszCachePath;

cleanup:

    if (ctx)
    {
        if (client)
        {
            krb5_free_principal(ctx, client);
        }

        krb5_free_cred_contents(ctx, &creds);

        if (cc != NULL)
        {
            krb5_cc_close(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    return ntStatus;

error:

    *ppszCachePath = NULL;

    LWIO_SAFE_FREE_STRING(pszCachePath);

    goto cleanup;
}

static
NTSTATUS
GetPassword(
    PSTR* ppszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CHAR szBuf[129];
    DWORD idx = 0;
    struct termios old, new;
    CHAR ch;

    memset(szBuf, 0, sizeof(szBuf));

    tcgetattr(0, &old);
    memcpy(&new, &old, sizeof(struct termios));
    new.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &new);

    while ( (idx < 128) )
    {
        if (read(0, &ch, 1))
        {
            if (ch != '\n')
            {
                szBuf[idx++] = ch;
            }
            else
            {
                break;
            }
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (idx == 128)
    {
        ntStatus = LwErrnoToNtStatus(ENOBUFS);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (idx > 0)
    {
        ntStatus = SMBAllocateString(szBuf, ppszPassword);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        *ppszPassword = NULL;
    }

cleanup:

    tcsetattr(0, TCSANOW, &old);

    return ntStatus;

error:

    *ppszPassword = NULL;

    goto cleanup;
}

static
VOID
ShowUsage(
    VOID
    )
{
    // printf("Usage: lwio-copy [-h] [-r] [ -k <path> | -u <user id>@REALM] <source path> <target path>\n");
    printf("Usage: lwio-copy [-h] [ -k <path> | -u user-id@REALM -p <password> | -u user -d DOMAIN -p <password> ] <source path> <target path>\n");
    printf("\t-h Show help\n");
    // printf("\t-r Recurse when copying a directory\n");
    printf("\t-k kerberos cache path\n");
    printf("Usage: lwio-copy //imgserver.abc.com/public/apple.jpg ./apple.jpg\n");
}


static
VOID
LwIoExitHandler(
    VOID
    )
{
    if (!IsNullOrEmptyString(gpszLwioCopyKrb5CachePath))
    {
        LwIoDestroyKrb5Cache(gpszLwioCopyKrb5CachePath);
        LWIO_SAFE_FREE_STRING(gpszLwioCopyKrb5CachePath);
    }
}

static
NTSTATUS
LwIoDestroyKrb5Cache(
    PCSTR pszCachePath
    )
{
    // TODO: Should we use krb5 apis to delete this file?
    return SMBRemoveFile(gpszLwioCopyKrb5CachePath);
}


static
VOID
MapErrorCodes(
    NTSTATUS ntStatus
    )
{
    switch (ntStatus)
    {
        case STATUS_INVALID_PARAMETER:
            fprintf(stderr,"Error: Invalid parameter passed\n");
            break;
        case STATUS_NO_SUCH_FILE:            
            fprintf(stderr,"Error: Invalid local file/dir path passed\n");
            break;
        case STATUS_FILE_IS_A_DIRECTORY:            
            fprintf(stderr,"Error: Invalid directory path passed\n");
            fprintf(stdout,"Please proivde a valid file name to copy a file\n");
            break;
        case STATUS_BAD_NETWORK_NAME:            
            fprintf(stderr,"Error: Invalid remote path passed\n");
            fprintf(stdout,"Please check if a valid remote host name/valid directory is provided\n");
            break;
        case STATUS_OBJECT_PATH_INVALID:            
        case STATUS_OBJECT_PATH_NOT_FOUND:            
        case STATUS_OBJECT_PATH_SYNTAX_BAD:            
        case STATUS_OBJECT_NAME_INVALID:            
        case STATUS_OBJECT_NAME_NOT_FOUND:            
            fprintf(stderr,"Error: Invalid remote file path passed\n");
            break;
        case STATUS_UNSUCCESSFUL:            
            fprintf(stderr,"Error: lwio-copy unsuccessfull\n");
            fprintf(stdout,"Please check if lwiod and lsassd running\n");
            break;
        case STATUS_LOGON_FAILURE:
        case STATUS_NO_SUCH_USER:
            fprintf(stderr,"Error: Logon failed, please check the username/password\n");
            break;
        case STATUS_NO_SUCH_DOMAIN:
            fprintf(stderr,"Error: Invalid domain\n");
            fprintf(stdout,"Please check if machine is joined to a domain\n");
            break;
        case STATUS_PASSWORD_EXPIRED: 
            fprintf(stderr,"Error: password expired\n");
            break;
        case STATUS_INVALID_ACCOUNT_NAME: 
            fprintf(stderr,"Error: Invalid account name\n");
            break;
        case STATUS_WRONG_PASSWORD: 
            fprintf(stderr,"Error: password mismatch\n");
            break;
        case STATUS_TIME_DIFFERENCE_AT_DC:      
            fprintf(stderr,"Error: clock skew detected\n");
            break;
        case STATUS_ACCESS_DENIED:      
            fprintf(stderr,"Error: Access denied\n");
            fprintf(stdout,"Try logging in as a domain user having an admin privilege\n");
            break;
    }
}
