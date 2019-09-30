/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        netsession.c
 *
 * Abstract:
 *
 *        BeyondTrust System NET Utilities
 *
 *        Session Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

DWORD
NetExecSessionEnum(
    PWSTR pwszServername,   /* IN              */
    PWSTR pwszClientname    /* IN     OPTIONAL */
    )
{
    NET_API_STATUS nStatus  = 0;
    PWSTR pwszUsername      = NULL;
    DWORD dwInfoLevel       = 2;
    PBYTE pBuffer           = NULL;
    DWORD dwPrefmaxLen      = UINT32_MAX;
    DWORD dwEntriesRead     = 0;
    DWORD dwTotalEntries    = 0;
    DWORD dwResumeHandle    = 0;
    DWORD iSession          = 0;
    DWORD iSessionCursor    = 0;
    PSESSION_INFO_2 pSessionCursor = NULL;
    PSTR  pszSessionname    = NULL;
    PSTR  pszUsername       = NULL;
    PSTR  pszClientType     = NULL;
    BOOLEAN bContinue       = FALSE;

    do
    {
        if (pBuffer)
        {
            NetApiBufferFree(pBuffer);
            pBuffer = NULL;
        }

        bContinue = FALSE;
        nStatus = NetSessionEnumW(
                        pwszServername,
                        pwszClientname,
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

                pSessionCursor = (PSESSION_INFO_2)pBuffer;

                for (   iSession = 0;
                        iSession < dwEntriesRead;
                        iSession++, pSessionCursor++)
                {
                    if (pSessionCursor->sesi2_cname)
                    {
                        LW_SAFE_FREE_STRING(pszSessionname);

                        nStatus = LwWc16sToMbs(
                                        pSessionCursor->sesi2_cname,
                                        &pszSessionname);
                        BAIL_ON_LTNET_ERROR(nStatus);
                    }

                    if (pSessionCursor->sesi2_username)
                    {
                        LW_SAFE_FREE_STRING(pszUsername);

                        nStatus = LwWc16sToMbs(
                                        pSessionCursor->sesi2_username,
                                        &pszUsername);
                        BAIL_ON_LTNET_ERROR(nStatus);
                    }

                    if (pSessionCursor->sesi2_cltype_name)
                    {
                        LW_SAFE_FREE_STRING(pszClientType);

                        nStatus = LwWc16sToMbs(
                                        pSessionCursor->sesi2_cltype_name,
                                        &pszClientType);
                        BAIL_ON_LTNET_ERROR(nStatus);
                    }

                    printf("Session [%u]\n", ++iSessionCursor);

                    printf("\tComputer:        %s\n",
                            (pszSessionname ? pszSessionname : ""));
                    printf("\tUsername:        %s\n",
                            (pszUsername ? pszUsername : ""));
                    printf("\tClient type:     %s\n",
                            (pszClientType ? pszClientType : ""));
                    printf("\tNumber of opens: %u\n",
                            pSessionCursor->sesi2_num_opens);
                    printf("\tActive time:     %u\n",
                            pSessionCursor->sesi2_time);
                    printf("\tIdle time:       %u\n",
                            pSessionCursor->sesi2_idle_time);
                    printf("\tGuest login?     %s\n",
                            pSessionCursor->sesi2_user_flags & 0x1 ? "yes" : "no");
                    printf("\tEncryption?      %s\n\n",
                            pSessionCursor->sesi2_user_flags & 0x2 ? "yes" : "no");

                }

                break;

            default:

                BAIL_ON_LTNET_ERROR(nStatus);

                break;
        }

    } while (bContinue);

    if (!iSessionCursor)
    {
        printf("There are no entries in the list\n");
    }

cleanup:

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    LW_SAFE_FREE_STRING(pszSessionname);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_FREE_STRING(pszClientType);

    return nStatus;

error:

    fprintf(stderr, "Failed to enumerate sessions.\n");

    goto cleanup;
}

DWORD
NetExecSessionLogoff(
    PWSTR pwszServername,   /* IN     OPTIONAL */
    PWSTR pwszUncClientname /* IN     OPTIONAL */
    )
{
    NET_API_STATUS nStatus = 0;
    PWSTR   pwszUsername = NULL;

    nStatus = NetSessionDelW(pwszServername, pwszUncClientname, pwszUsername);
    BAIL_ON_LTNET_ERROR(nStatus);

cleanup:

    return nStatus;

error:

    fprintf(stderr, "Failed to delete session.\n");

    goto cleanup;
}
