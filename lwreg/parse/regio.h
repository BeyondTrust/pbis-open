/*
 * Copyright Likewise Software    2004-2009
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
