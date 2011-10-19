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
 *        nss-error.c
 *
 * Abstract:
 * 
 *        Name Server Switch (Likewise LSASS)
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
