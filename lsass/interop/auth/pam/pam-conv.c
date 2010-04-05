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

int
LsaPamConverse(
    pam_handle_t* pamh,
    PCSTR         pszPrompt,
    int           messageStyle,
    PSTR*         ppszResponse
    )
{
    DWORD  dwError = PAM_SUCCESS;
    struct pam_conv* pConv = NULL;
    struct pam_response* pResponse = NULL;
    struct pam_message msg;
    struct pam_message* pMsg = NULL;
    PSTR   pszResponse = NULL;
    
    dwError = pam_get_item(pamh, PAM_CONV, (PAM_GET_ITEM_TYPE)&pConv);
    BAIL_ON_LSA_ERROR(dwError);
    
    memset(&msg, 0, sizeof(struct pam_message));
    pMsg = &msg;
    
    pMsg->msg_style = messageStyle;
    pMsg->msg       = (PAM_MESSAGE_MSG_TYPE)pszPrompt;
    
    if (pConv->conv)
    {
        dwError = pConv->conv(1,
                (PAM_CONV_2ND_ARG_TYPE)&pMsg,
                &pResponse,
                pConv->appdata_ptr);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LSA_LOG_PAM_INFO("Unable to send prompt to user from PAM. Most likely the calling program is non-interactive");
        // Leave pResponse as NULL.
    }
    
    switch (messageStyle)
    {
        case PAM_PROMPT_ECHO_ON:
        case PAM_PROMPT_ECHO_OFF:
            if (pResponse == NULL || (pResponse->resp == NULL)) {
                
               dwError = PAM_CONV_ERR;
               BAIL_ON_LSA_ERROR(dwError);
               
            } else {
               
               dwError = LwAllocateString(pResponse->resp, &pszResponse);
               BAIL_ON_LSA_ERROR(dwError);
               
            }
            break;
    }
    
    // We don't need a response for certain message styles
    // For instance, PAM_ERROR_MSG or PAM_TEXT_INFO
    if (ppszResponse) {
       *ppszResponse = pszResponse;
    }
    
cleanup:

    if (pResponse) {
        if (pResponse->resp) {
            memset(pResponse->resp, 0, strlen(pResponse->resp));
            free(pResponse->resp);
        }
        free(pResponse);
    }

    return LsaPamMapErrorCode(dwError, NULL);
    
error:

    if (ppszResponse != NULL)
    {
        *ppszResponse = NULL;
    }
    
    LW_SAFE_CLEAR_FREE_STRING(pszResponse);

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
