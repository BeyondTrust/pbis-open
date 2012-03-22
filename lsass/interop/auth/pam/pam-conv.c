/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-conv.h
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module
 * 
 *        Application Conversations
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

DWORD
LsaPamConverse(
    pam_handle_t* pamh,
    PCSTR         pszPrompt,
    int           messageStyle,
    PSTR*         ppszResponse
    )
{
    LSA_PAM_CONVERSE_MESSAGE message;

    message.messageStyle = messageStyle;
    message.pszMessage = (PSTR) pszPrompt;
    message.ppszResponse = ppszResponse;

    return LsaPamConverseMulti(pamh, 1, &message);
}

DWORD
LsaPamConverseMulti(
    pam_handle_t*             pamh,
    int                       numMessages,
    PLSA_PAM_CONVERSE_MESSAGE pLsaPamConvMessages
    )
{
    DWORD  dwError = 0;
    struct pam_conv* pConv = NULL;
    struct pam_response* pResponses = NULL;
    struct pam_message* msgs = NULL;
    struct pam_message** pMsgs = NULL;
    int    iPamError = 0;
    int    i;
    
    iPamError = pam_get_item(pamh, PAM_CONV, (PAM_GET_ITEM_TYPE)&pConv);
    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pConv)
    {
        dwError = LsaPamUnmapErrorCode(PAM_CONV_ERR);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pConv->conv)
    {
        dwError = LwAllocateMemory(numMessages * sizeof(*msgs),
                (PVOID*) &msgs);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateMemory(numMessages * sizeof(*pMsgs),
                (PVOID*) &pMsgs);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < numMessages; ++i)
        {
            msgs[i].msg_style = pLsaPamConvMessages[i].messageStyle;
            msgs[i].msg = pLsaPamConvMessages[i].pszMessage;

            switch (msgs[i].msg_style)
            {
                case PAM_PROMPT_ECHO_ON:
                case PAM_PROMPT_ECHO_OFF:
                    if (pLsaPamConvMessages[i].ppszResponse == NULL)
                    {
                        BAIL_WITH_LSA_ERROR(LW_ERROR_INVALID_PARAMETER);
                    }
                    break;

                default:
                    if (pLsaPamConvMessages[i].ppszResponse != NULL)
                    {
                        BAIL_WITH_LSA_ERROR(LW_ERROR_INVALID_PARAMETER);
                    }
                    break;
            }

            pMsgs[i] = &msgs[i];
        }

        iPamError = pConv->conv(numMessages,
                (PAM_CONV_2ND_ARG_TYPE)pMsgs,
                &pResponses,
                pConv->appdata_ptr);
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LSA_LOG_PAM_INFO("Unable to send prompt to user from PAM. Most likely the calling program is non-interactive");
        // Leave pResponses as NULL.
    }

    for (i = 0; i < numMessages; ++i)
    {
        switch (pLsaPamConvMessages[i].messageStyle)
        {
            case PAM_PROMPT_ECHO_ON:
            case PAM_PROMPT_ECHO_OFF:
                /*
                 * The pResponses == NULL check is here because
                 * it's perfectly OK for it to be NULL if none of the
                 * messages required a response.
                 */
                if (pResponses == NULL || pResponses[i].resp == NULL)
                {
                   BAIL_WITH_LSA_ERROR(PAM_CONV_ERR);
                }

                *pLsaPamConvMessages[i].ppszResponse = pResponses[i].resp;
                break;

            default:
                if (pResponses != NULL && pResponses[i].resp != NULL)
                {
                    /* Got a response we weren't expecting. */
                    BAIL_WITH_LSA_ERROR(PAM_CONV_ERR);
                }
                break;
        }
    }

cleanup:
    if (pResponses)
    {
        /*
         * All the .resp pointers have either been copied to
         * the caller or freed.
         */
        free(pResponses);
    }

    LW_SAFE_FREE_MEMORY(msgs);
    LW_SAFE_FREE_MEMORY(pMsgs);

    return dwError;

error:
    if (pResponses)
    {
        for (i = 0; i < numMessages; ++i)
        {
            LW_SECURE_FREE_STRING(pResponses[i].resp);
        }
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
