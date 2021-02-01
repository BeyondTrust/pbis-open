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
 *        smbfileapi.h
 *
 * Abstract:
 *
 *        SMB-specific API functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#ifndef __LW_IO_SMBFILEAPI_H__
#define __LW_IO_SMBFILEAPI_H__

#include <lwio/io-types.h>

LW_NTSTATUS
LwIoGetSessionKey(
    IO_FILE_HANDLE File,
    LW_PUSHORT pKeyLength,
    LW_PBYTE* ppKeyBuffer
    );

LW_NTSTATUS
LwIoGetPeerAccessToken(
    IO_FILE_HANDLE File,
    PACCESS_TOKEN* ppToken
    );

LW_NTSTATUS
LwIoGetPeerAddress(
    IO_FILE_HANDLE File,
    LW_PBYTE pAddress,
    LW_PUSHORT pusAddressLength
    );

LW_NTSTATUS
LwIoConnectNamedPipe(
    IO_FILE_HANDLE File,
    PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    PIO_STATUS_BLOCK IoStatusBlock
    );

LW_NTSTATUS
LwIoSetRdrDomainHints(
    LW_PWSTR* ppwszDomains,
    ULONG ulCount
    );

LW_NTSTATUS
LwIoRdrGetPhysicalPath(
    IO_FILE_HANDLE File,
    LW_PWSTR* ppResolved
    );

VOID
LwIoRdrFreePhysicalPath(
    LW_PWSTR pResolved
    );

#endif /* !__LW_IO_SMBFILEAPI_H__ */
