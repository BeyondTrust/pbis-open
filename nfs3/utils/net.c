/*
 * Copyright Likewise Software    2004-2010
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net.c
 *
 * Abstract:
 *
 *        NFS3
 *
 *        Network utilities implementation implementation
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 */

#include "includes.h"


NTSTATUS
Nfs3SocketAddressToString(
    struct sockaddr* pSocketAddress,
    PSTR             pszBuf,
    ULONG            ulBufLen
    )
{
    NTSTATUS ntStatus  = STATUS_SUCCESS;
    PVOID pAddressPart = NULL;

    switch (pSocketAddress->sa_family)
    {
        case AF_INET:

            pAddressPart = &((struct sockaddr_in*)pSocketAddress)->sin_addr;

            break;

#ifdef AF_INET6
        case AF_INET6:

            pAddressPart = &((struct sockaddr_in6*)pSocketAddress)->sin6_addr;

            break;
#endif
        default:

           ntStatus = STATUS_NOT_SUPPORTED;
           BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!inet_ntop(pSocketAddress->sa_family, pAddressPart, pszBuf, ulBufLen))
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    // Terminate output buffer
    if (ulBufLen > 0)
    {
        pszBuf[0] = 0;
    }

    goto cleanup;
}
