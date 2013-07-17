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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

extern
DWORD
EventlogConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
LsassConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
NetlogonConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

#ifdef ENABLE_TDB
extern
DWORD
LwiauthConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszSecretsFile,
    PCSTR pszRegFile
    );
#endif

extern
DWORD
LwioConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
SqliteMachineAccountToPstore(
    PCSTR pszSqlDb
    );

extern
DWORD
TestParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
TestSambaParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );


static
DWORD
GetErrorMessage(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    );

static
VOID
PrintUsage(
    PCSTR pszAdditionalMessage
    );

int
main(
    int argc,
    char *argv[]
    )
{
    DWORD dwError = 0;
    PSTR pszErrMsg = NULL;

    if (argc < 2 )
    {
        PrintUsage(NULL);
        goto cleanup;
    }

    if (!strcmp(argv[1], "--eventlog"))
    {
        if (argc < 4)
        {
            PrintUsage("--eventlog requires two arguments.");
        }
        else
        {
            dwError = EventlogConfFileToRegFile(argv[2], argv[3]);
        }
    }
    else if (!strcmp(argv[1], "--lsass"))
    {
        if (argc < 4)
        {
            PrintUsage("--lsass requires two arguments.");
        }
        else
        {
            dwError = LsassConfFileToRegFile(argv[2], argv[3]);
        }
    }
    else if (!strcmp(argv[1], "--netlogon"))
    {
        if (argc < 4)
        {
            PrintUsage("--netlogon requires two arguments.");
        }
        else
        {
            dwError = NetlogonConfFileToRegFile(argv[2], argv[3]);
        }
    }
#ifdef ENABLE_TDB
    else if (!strcmp(argv[1], "--lwiauth"))
    {
        if (argc < 5)
        {
            PrintUsage("--lwiauth requires three arguments and usually root privileges.");
        }
        else
        {
            dwError = LwiauthConfFileToRegFile(argv[2], argv[3], argv[4]);
        }
    }
#endif
    else if (!strcmp(argv[1], "--pstore-sqlite"))
    {
        if (argc < 3)
        {
            PrintUsage("--pstore-sqlite requires one argument and usually root privileges.");
        }
        else
        {
            dwError = SqliteMachineAccountToPstore(argv[2]);
        }
    }
    else
    {
        PrintUsage(NULL);
    }

cleanup:

    if (dwError)
    {
        if (!GetErrorMessage(dwError, &pszErrMsg) && pszErrMsg)
        {
            fputs(pszErrMsg, stderr);
            fputs("\n", stderr);
            LW_SAFE_FREE_STRING(pszErrMsg);
        }
        else
        {
            fprintf(stderr, "Error %lu\n", (unsigned long)dwError);
        }

        return 1;
    }
    return 0;
}

static
VOID
PrintUsage(
    PCSTR pszAdditionalMessage
    )
{
    fputs(
"conf2reg: Generates files, from previous Likewise releases, into\n"
"          the current format and imports the machine account."
"\n", stderr);

    fputs(
"--lsass CONF REG\n"
"  Convert lsass 5.x configuration file to registry\n"
"  import file.\n"
"\n", stderr);

    fputs(
"--netlogon CONF REG\n"
"  Convert netlogon 5.x configuration file to registry\n"
"  import file.\n"
"\n", stderr);

    fputs(
"--eventlog CONF REG\n"
"  Convert eventlog 5.x configuration file to registry\n"
"  import file.\n"
"\n", stderr);

#ifdef ENABLE_TDB
    fputs(
"--lwiauth CONF TDB REG\n"
"  Import 4.1 machine account (requires root privileges)\n"
"  using the files lwiauthd.conf and secrets.tdb.\n"
"  Also generates a registry file for use with lwregshell\n"
"  to preserve various system settings.\n"
"\n", stderr);
#endif

    fputs(
"--pstore-sqlite SQLDB\n"
"  Import 5.0/5.1/5.3 machine account (requires root privileges)\n"
"  stored in a sqlite database.\n"
"\n", stderr);

   if (pszAdditionalMessage)
   {
       fputs("\n", stderr);
       fputs(pszAdditionalMessage, stderr);
   }
}

static
DWORD
GetErrorMessage(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg)
{
    DWORD dwError = 0;
    DWORD dwErrorBufferSize = 0;
    DWORD dwLen = 0;
    PSTR pszErrorMsg = NULL;
    PSTR pszErrorBuffer = NULL;

    dwErrorBufferSize = LwGetErrorString(dwErrCode, NULL, 0);

    if(!dwErrorBufferSize)
        goto cleanup;

    dwError = LwAllocateMemory(
                    dwErrorBufferSize,
                    OUT_PPVOID(&pszErrorBuffer));
    BAIL_ON_UP_ERROR(dwError);

    dwLen = LwGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);
    if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
    {
        dwError = LwAllocateStringPrintf(
                    &pszErrorMsg,
                    "Error: %s [error code: %d]", pszErrorBuffer,
                    dwErrCode);
        BAIL_ON_UP_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:
    LW_SAFE_FREE_MEMORY(pszErrorBuffer);

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

