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
 *        nss-error.c
 *
 * Abstract:
 * 
 *        Name Server Switch (BeyondTrust LSASS)
 * 
 *        Handle NSS Errors
 *        Specifically, Map LSA Errors to Name Server Switch (NSS) Errors
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "lsanss.h"

int
LsaNssMapErrorCode(
    DWORD dwError,
    int*  pErrno
    )
{
    int ret = NSS_STATUS_SUCCESS;
    
    if (!dwError) {
        goto cleanup;
    }
        
    switch(dwError)
    {
        case LW_ERROR_NO_SUCH_USER:
        case LW_ERROR_NO_SUCH_GROUP:
        case ECONNREFUSED:
            {
                ret = NSS_STATUS_NOTFOUND;
                // Don't set errno if we can't find the user (or lsass)
                break;
            }
        case LW_ERROR_INSUFFICIENT_BUFFER:
            {
                ret = NSS_STATUS_TRYAGAIN;
                if (pErrno) {
                    *pErrno = ERANGE;
                }
                break;
            }
        case LW_ERROR_NULL_BUFFER: 
            {
                //On some platforms, a NULL buffer is used to signal the caller
                //does not need the userinfo data, just confirmation the user exists
                ret = NSS_STATUS_SUCCESS;
                break;
            }
        default:
            {
                ret = NSS_STATUS_UNAVAIL;
                if (dwError < 0x8000 && pErrno) {
                    // This error code is probably an errno.
                    *pErrno = dwError;
                }
                break;
            }
    }

cleanup:
    
    return ret;
}
