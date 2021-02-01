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
 *        regio.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser file I/O routines header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#ifndef REGIO_H
#define REGIO_H

#define  REGIO_BUFSIZ  BUFSIZ

DWORD
RegIOOpen(
    PCSTR pszRegFile,
    PHANDLE pHandle);

DWORD
RegIOClose(
    HANDLE handle);

DWORD
RegIOBufferOpen(
    PHANDLE pHandle);

DWORD
RegIOBufferSetData(
    PHANDLE pHandle,
    PSTR inBuf,
    DWORD inBufLen);

DWORD
RegIOBufferGetData(
    PHANDLE pHandle,
    PSTR *outBuf,
    PDWORD outBufLen,
    PDWORD outBufOffset);

DWORD
RegIOClose(
    HANDLE ioHandle);

DWORD
RegIOReadData(
    HANDLE handle);

DWORD
RegIOGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof);

DWORD
RegIOGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar);

DWORD
RegIOUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar);

DWORD
RegIOIsEOF(
    HANDLE handle,
    PBOOLEAN pEof);

#endif
