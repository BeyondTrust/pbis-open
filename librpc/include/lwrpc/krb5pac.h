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
 * Abstract: Kerberos 5 PAC decoding functions (rpc client library)
 *
 * Authors: Kyle Stemen (kstemen@likewise.com)
 */

#ifndef _KRB5PAC_H_H
#define _KRB5PAC_H_H

#include <lwrpc/krb5pacdefs.h>

error_status_t
DecodePacLogonInfo(
    const char *pchBuffer,
    size_t sBufferLen,
    PAC_LOGON_INFO **ppLogonInfo
    );

error_status_t
EncodePacLogonInfo(
    PAC_LOGON_INFO* pLogonInfo,
    PDWORD pdwEncodedSize,
    PBYTE* ppEncodedBuffer
    );

void
FreePacLogonInfo(
    PAC_LOGON_INFO *pInfo
    );


void
FreeUnicodeStringContents(
    UnicodeStringEx *pStr
    );


void
FreeRidWithAttributeArrayContents(
    RidWithAttributeArray *pArr
    );


void
FreeNetrSamBaseInfoContents(
    NetrSamBaseInfo *pBase
    );


void
FreeSidAttrArray(
    NetrSidAttr *pSids,
    size_t sCount
    );


void
FreeNetrSamInfo3Contents(
    NetrSamInfo3 *pInfo
    );


#endif /* _KRB5PAC_H_H */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
