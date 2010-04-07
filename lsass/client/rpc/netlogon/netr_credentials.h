/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        netr_credentials.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Netlogon credentials functions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _NETR_CREDENTIALS_H_
#define _NETR_CREDENTIALS_H_


#define GETUINT8(buf, off)                      \
    ((UINT8)(buf)[(off)])

#define GETUINT16(buf, off)                     \
    (((UINT16)GETUINT8((buf), 0+(off))) |       \
     ((UINT16)GETUINT8((buf), 1+(off)) << 8))

#define GETUINT32(buf, off)                     \
    (((UINT32)GETUINT16((buf), 0+(off))) |      \
     ((UINT32)GETUINT16((buf), 2+(off)) << 16))

#define SETUINT8(buf, off, v)                   \
    ((buf)[(off)] = (UINT8)((v) & 0xff))

#define SETUINT16(buf, off, v)                  \
    do {                                        \
        SETUINT8((buf), 0+(off), (v));          \
        SETUINT8((buf), 1+(off), ((v) >> 8));   \
    } while (0)

#define SETUINT32(buf, off, v)                  \
    do {                                        \
        SETUINT16((buf), 0+(off), (v));         \
        SETUINT16((buf), 2+(off), ((v) >> 16)); \
    } while (0)


VOID
NetrCredentialsCliStep(
    IN OUT NetrCredentials *pCreds
    );


VOID
NetrCredentialsSrvStep(
    IN OUT NetrCredentials *pCreds
    );


VOID
NetrGetNtHash(
    OUT BYTE    Hash[16],
    IN  PCWSTR  pwszPassword
    );


VOID
NetrGetLmHash(
    OUT BYTE    Hash[16],
    IN  PCWSTR  pwszPassword
    );


#endif /* _NETR_CREDENTIALS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
