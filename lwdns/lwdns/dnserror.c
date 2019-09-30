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

#include "includes.h"

DWORD
DNSMapRCode(
    DWORD dwRCode
    )
{
    DWORD dwError = 0;

    switch (dwRCode)
    {
        case 0:
            dwError = ERROR_SUCCESS;
            break;

        case 1:
            dwError = DNS_ERROR_RCODE_FORMAT_ERROR;
            break;

        case 2:
            dwError = DNS_ERROR_RCODE_SERVER_FAILURE;
            break;

        case 3:
            dwError = DNS_ERROR_RCODE_NAME_ERROR;
            break;

        case 4:
            dwError = DNS_ERROR_RCODE_NOT_IMPLEMENTED;
            break;

        case 5:
            dwError = DNS_ERROR_RCODE_REFUSED;
            break;

        case 6:
            dwError = DNS_ERROR_RCODE_YXDOMAIN;
            break;

        case 7:
            dwError = DNS_ERROR_RCODE_YXRRSET;
            break;

        case 8:
            dwError = DNS_ERROR_RCODE_NXRRSET;
            break;

        case 9:
            dwError = DNS_ERROR_RCODE_NOTAUTH;
            break;

        case 10:
            dwError = DNS_ERROR_RCODE_NOTZONE;
            break;

        case 16:
            dwError = DNS_ERROR_RCODE_BADSIG;
            break;

        case 17:
            dwError = DNS_ERROR_RCODE_BADKEY;
            break;

        case 18:
            dwError = DNS_ERROR_RCODE_BADTIME;
            break;

        default:
            dwError = DNS_ERROR_RCODE;
            break;

    }

    return dwError;
}

DWORD
DNSMapHerrno(
    DWORD dwHerrno
    )
{
    switch (dwHerrno)
    {
        case HOST_NOT_FOUND:
            return WSAHOST_NOT_FOUND;
        case NO_ADDRESS:
#if NO_ADDRESS != NO_DATA
        case NO_DATA:
#endif
            return DNS_ERROR_RECORD_DOES_NOT_EXIST;
        case NO_RECOVERY:
            return DNS_ERROR_RCODE_SERVER_FAILURE;
        case TRY_AGAIN:
            return DNS_ERROR_TRY_AGAIN_LATER;
        default:
            return ERROR_GEN_FAILURE;
    }
}
