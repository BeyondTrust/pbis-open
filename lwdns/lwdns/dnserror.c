/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
