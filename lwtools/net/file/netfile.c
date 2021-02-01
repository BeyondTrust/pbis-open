/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

