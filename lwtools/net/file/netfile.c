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
 */

#include "includes.h"

DWORD
NetExecFileEnum(
    PCWSTR pwszServername
    )
{
    NET_API_STATUS nStatus  = 0;
    PWSTR pwszBasepath      = NULL;
    PWSTR pwszUsername      = NULL;
    DWORD dwInfoLevel       = 3;
    PBYTE pBuffer           = NULL;
    DWORD dwPrefmaxLen      = UINT32_MAX;
    DWORD dwEntriesRead     = 0;
    DWORD dwTotalEntries    = 0;
    DWORD dwResumeHandle    = 0;
    DWORD iFile             = 0;
    DWORD iFileCursor       = 0;
    PFILE_INFO_3 pFileCursor = NULL;
    PSTR  pszPathname       = NULL;
    PSTR  pszUsername       = NULL;
    BOOLEAN bContinue       = TRUE;

    do
    {
        if (pBuffer)
        {
            NetApiBufferFree(pBuffer);
            pBuffer = NULL;
        }

        bContinue = FALSE;
        nStatus = NetFileEnumW(
                        pwszServername,
                        pwszBasepath,
                        pwszUsername,
                        dwInfoLevel,
                        &pBuffer,
                        dwPrefmaxLen,
                        &dwEntriesRead,
                        &dwTotalEntries,
                        &dwResumeHandle);

        if (nStatus == ERROR_MORE_DATA)
        {
            bContinue = TRUE;
        }
        switch (nStatus)
        {
            case ERROR_SUCCESS:
            case ERROR_MORE_DATA:

                pFileCursor = (PFILE_INFO_3)pBuffer;

                for (iFile = 0; iFile < dwEntriesRead; iFile++, pFileCursor++)
                {
                    if (pFileCursor->fi3_path_name)
                    {
                        LW_SAFE_FREE_STRING(pszPathname);

                        nStatus = LwWc16sToMbs(
                                        pFileCursor->fi3_path_name,
                                        &pszPathname);
                        BAIL_ON_LTNET_ERROR(nStatus);
                    }

                    if (pFileCursor->fi3_username)
                    {
                        LW_SAFE_FREE_STRING(pszUsername);

                        nStatus = LwWc16sToMbs(
                                        pFileCursor->fi3_username,
                                        &pszUsername);
                        BAIL_ON_LTNET_ERROR(nStatus);
                    }

                    printf("File [%u]\n", ++iFileCursor);

                    printf("\tId:              %u\n", pFileCursor->fi3_idd);
                    printf("\tPathname:        %s\n",
                            (pszPathname ? pszPathname : ""));
                    printf("\tUsername:        %s\n",
                            (pszUsername ? pszUsername : ""));
                    printf("\tNumber of locks: %u\n",
                            pFileCursor->fi3_num_locks);
                    printf("\tPermissions:     0x%08x\n",
                            pFileCursor->fi3_permissions);
                }

                break;

            default:

                BAIL_ON_LTNET_ERROR(nStatus);

                break;
        }

    } while (bContinue);

    if (!iFileCursor)
    {
        printf("There are no entries in the list\n");
    }

cleanup:

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    LW_SAFE_FREE_STRING(pszPathname);
    LW_SAFE_FREE_STRING(pszUsername);

    return nStatus;

error:

    fprintf(stderr, "Failed to enumerate files.\n");

    goto cleanup;
}

DWORD
NetExecFileQueryInfo(
    PCWSTR pwszServername,
    DWORD  dwFileId
    )
{
    NET_API_STATUS nStatus     = 0;
    DWORD          dwInfoLevel = 3;
    PBYTE          pBuffer     = NULL;
    PFILE_INFO_3   pFileInfo   = NULL;
    PSTR           pszPathname = NULL;
    PSTR           pszUsername = NULL;

    nStatus = NetFileGetInfoW(pwszServername, dwFileId, dwInfoLevel, &pBuffer);
    BAIL_ON_LTNET_ERROR(nStatus);

    pFileInfo = (PFILE_INFO_3)pBuffer;

    if (pFileInfo->fi3_path_name)
    {
        LW_SAFE_FREE_STRING(pszPathname);

        nStatus = LwWc16sToMbs(pFileInfo->fi3_path_name, &pszPathname);
        BAIL_ON_LTNET_ERROR(nStatus);
    }

    if (pFileInfo->fi3_username)
    {
        LW_SAFE_FREE_STRING(pszUsername);

        nStatus = LwWc16sToMbs(pFileInfo->fi3_username, &pszUsername);
        BAIL_ON_LTNET_ERROR(nStatus);
    }

    printf("\tId:              %u\n",     pFileInfo->fi3_idd);
    printf("\tPathname:        %s\n",     (pszPathname ? pszPathname : ""));
    printf("\tUsername:        %s\n",     (pszUsername ? pszUsername : ""));
    printf("\tNumber of locks: %u\n",     pFileInfo->fi3_num_locks);
    printf("\tPermissions:     0x%08x\n", pFileInfo->fi3_permissions);

cleanup:

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    LW_SAFE_FREE_STRING(pszPathname);
    LW_SAFE_FREE_STRING(pszUsername);

    return nStatus;

error:

    fprintf(stderr, "Failed to get information on file [Id: %u].\n", dwFileId);

    goto cleanup;
}

DWORD
NetExecFileClose(
    PCWSTR pwszServername,
    DWORD  dwFileId
    )
{
    NET_API_STATUS nStatus = 0;

    nStatus = NetFileCloseW(pwszServername, dwFileId);
    BAIL_ON_LTNET_ERROR(nStatus);

cleanup:

    return nStatus;

error:

    fprintf(stderr, "Failed to close file [Id: %u].\n", dwFileId);

    goto cleanup;
}

