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
 *        regio.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser file I/O routines
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */


#include "includes.h"

/* Implementation of File I/O functions */
static DWORD
RegIOFileClose(
    HANDLE handle);

static DWORD
RegIOFileIsEOF(
    HANDLE handle,
    PBOOLEAN pEof);

static DWORD
RegIOFileReadData(
    HANDLE handle);

static DWORD
RegIOFileGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof);

static DWORD
RegIOFileGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar);

static DWORD
RegIOFileUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar);

/* Implementation of char buffer I/O functions */
static DWORD
RegIOBufferClose(
    HANDLE handle);

static DWORD
RegIOBufferIsEOF(
    HANDLE handle,
    PBOOLEAN pEof);

static DWORD
RegIOBufferReadData(
    HANDLE handle);

static DWORD
RegIOBufferGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof);

static DWORD
RegIOBufferGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar);

static DWORD
RegIOBufferUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar);


typedef struct __REGIO_HANDLE
{
    DWORD (*pfn_RegIOIsEOF)(HANDLE handle, PBOOLEAN pEof);
    DWORD (*pfn_RegIOReadData)(HANDLE handle);
    DWORD (*pfn_RegIOGetChar)(HANDLE handle, PCHAR nextChar, PBOOLEAN pEof);
    DWORD (*pfn_RegIOGetPrevChar)(HANDLE handle, PCHAR pPrevChar);
    DWORD (*pfn_RegIOUnGetChar)(HANDLE handle, PCHAR pPrevChar);
    DWORD (*pfn_RegIOClose)(HANDLE handle);

    /* This structure MUST begin with only function pointers */

    FILE *fp;
    PSTR ioBuf;
    DWORD ioBufLen;
    LW_INT32 ioCursor;
    CHAR curChar;
    CHAR prevChar;
    BOOLEAN eof;
    PIV_CONVERT_CTX pivHandle;
    BOOLEAN isUTF16;
} REGIO_HANDLE, *PREGIO_HANDLE;


typedef struct __REGIO_BUFFER_HANDLE
{
    DWORD (*pfn_RegIOIsEOF)(HANDLE handle, PBOOLEAN pEof);
    DWORD (*pfn_RegIOReadData)(HANDLE handle);
    DWORD (*pfn_RegIOGetChar)(HANDLE handle, PCHAR nextChar, PBOOLEAN pEof);
    DWORD (*pfn_RegIOGetPrevChar)(HANDLE handle, PCHAR pPrevChar);
    DWORD (*pfn_RegIOUnGetChar)(HANDLE handle, PCHAR pPrevChar);
    DWORD (*pfn_RegIOClose)(HANDLE handle);

    /* This structure MUST begin with only function pointers */

    PSTR ioBuf;
    DWORD ioBufLen;
    LW_INT32 ioCursor;
    PSTR inBuf;
    DWORD inBufLen;
    DWORD inBufCursor;
    CHAR curChar;
    CHAR prevChar;
    BOOLEAN eof;
} REGIO_BUFFER_HANDLE, *PREGIO_BUFFER_HANDLE;


/*
 * Same as RegIOOpen() but does I/O from a char buffer vs a file
 */
DWORD
RegIOBufferOpen(
    PHANDLE pHandle)
{
    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = NULL;

    dwError = RegAllocateMemory(sizeof(*ioHandle), (PVOID*)&ioHandle);
    BAIL_ON_INVALID_HANDLE(ioHandle);

    ioHandle->ioBufLen = REGIO_BUFSIZ;
    ioHandle->ioCursor = -1;

    dwError = RegAllocateMemory(ioHandle->ioBufLen, (LW_PVOID*)&ioHandle->ioBuf);
    BAIL_ON_INVALID_POINTER(ioHandle->ioBuf);

    ioHandle->pfn_RegIOClose = RegIOBufferClose;
    ioHandle->pfn_RegIOIsEOF = RegIOBufferIsEOF;
    ioHandle->pfn_RegIOReadData = RegIOBufferReadData;
    ioHandle->pfn_RegIOGetChar = RegIOBufferGetChar;
    ioHandle->pfn_RegIOGetPrevChar = RegIOBufferGetPrevChar;
    ioHandle->pfn_RegIOUnGetChar = RegIOBufferUnGetChar;

    *pHandle = (HANDLE) ioHandle;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(ioHandle->ioBuf);
    LWREG_SAFE_FREE_MEMORY(ioHandle);
    goto cleanup;
}


/* Add data to IO Buffer */
DWORD
RegIOBufferSetData(
    PHANDLE pHandle,
    PSTR inBuf,
    DWORD inBufLen)
{
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) pHandle;

    ioHandle->inBuf = inBuf;
    ioHandle->inBufLen = inBufLen;
    ioHandle->inBufCursor = 0;

    ioHandle->ioBufLen = REGIO_BUFSIZ;
    ioHandle->ioCursor = -1;
    ioHandle->eof = FALSE;
    ioHandle->curChar = '\0';
    ioHandle->prevChar = '\0';

    return 0;
}


DWORD
RegIOBufferGetData(
    PHANDLE pHandle,
    PSTR *outBuf,
    PDWORD outBufLen,
    PDWORD outBufOffset)
{
    PREGIO_BUFFER_HANDLE ioHandle = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pHandle);
    ioHandle = (HANDLE) pHandle;

    if (outBuf)
    {
        *outBuf = ioHandle->inBuf;
    }
    if (outBufLen)
    {
        *outBufLen = ioHandle->inBufLen;
    }
    if (outBufOffset)
    {
        *outBufOffset = ioHandle->ioCursor;
    }

cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegIOOpen(
    PCSTR pszRegFile,
    PHANDLE pHandle)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle;
    DWORD inChar = 0;

    dwError = RegAllocateMemory(sizeof(*ioHandle), (PVOID*)&ioHandle);
    BAIL_ON_INVALID_HANDLE(ioHandle);

    if (!strcmp(pszRegFile, "-"))
    {
        ioHandle->fp = stdin;
    }
    else
    {
        ioHandle->fp = fopen(pszRegFile, "rb");
        BAIL_ON_INVALID_HANDLE(ioHandle->fp);
    }

    /*
     * Read first byte and try to determine if is UTF-16 or not
     * This test is not optimal, based on the first byte of the file
     * not being 8-bit clean.
     */
    inChar = fgetc(ioHandle->fp);
    if (inChar & (1<<7))
    {
        ioHandle->isUTF16 = TRUE;
    }
    rewind(ioHandle->fp);

    ioHandle->ioBufLen = REGIO_BUFSIZ;
    ioHandle->ioCursor = -1;

    dwError = RegAllocateMemory(ioHandle->ioBufLen, (PVOID*)&ioHandle->ioBuf);
    BAIL_ON_INVALID_POINTER(ioHandle->ioBuf);

    dwError = RegIconvConvertOpen(&ioHandle->pivHandle,
                          REGICONV_ENCODING_UTF8,
                          REGICONV_ENCODING_UCS2);
    BAIL_ON_REG_ERROR(dwError);

    ioHandle->pfn_RegIOClose = RegIOFileClose;
    ioHandle->pfn_RegIOIsEOF = RegIOFileIsEOF;
    ioHandle->pfn_RegIOReadData = RegIOFileReadData;
    ioHandle->pfn_RegIOGetChar = RegIOFileGetChar;
    ioHandle->pfn_RegIOGetPrevChar = RegIOFileGetPrevChar;
    ioHandle->pfn_RegIOUnGetChar = RegIOFileUnGetChar;

    *pHandle = (HANDLE) ioHandle;

cleanup:
    return dwError;

error:
    if (ioHandle->fp && ioHandle->fp != stdin)
    {
        fclose(ioHandle->fp);
    }
    LWREG_SAFE_FREE_MEMORY(ioHandle->ioBuf);
    LWREG_SAFE_FREE_MEMORY(ioHandle);

    goto cleanup;
}


DWORD
RegIOBufferClose(
    HANDLE handle)
{

    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);

    if (ioHandle->ioBuf)
    {
        RegMemoryFree(ioHandle->ioBuf);
    }
    RegMemoryFree(ioHandle);

cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegIOBufferIsEOF(
    HANDLE handle,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);

    return ioHandle->eof;

cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegIOBufferFread(
    PCHAR outBuf,
    SSIZE_T size,
    SSIZE_T numElem,
    HANDLE handle)
{
    SSIZE_T ioBytes = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    ioBytes = size * numElem;
    if (ioBytes > (ioHandle->inBufLen - ioHandle->inBufCursor))
    {
        ioBytes = ioHandle->inBufLen - ioHandle->inBufCursor;
    }
    memcpy(outBuf, ioHandle->inBuf + ioHandle->inBufCursor, ioBytes);
    ioHandle->inBufCursor += ioBytes;

    return ioBytes;
}


DWORD
RegIOBufferReadData(
    HANDLE handle)
{
    DWORD dwError = 0;
    DWORD items = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);

    if (ioHandle->ioCursor < 0 ||
        ioHandle->ioCursor >= ioHandle->ioBufLen)
    {
        items = RegIOBufferFread(
                    ioHandle->ioBuf,
                    1,
                    ioHandle->ioBufLen,
                    ioHandle);
        if (items == 0)
        {
            ioHandle->eof = TRUE;
            return dwError;
        }
        ioHandle->ioBufLen = items;
        ioHandle->ioCursor = 0;

    }

cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegIOBufferGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(nextChar);
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = RegIOReadData(ioHandle);
    if (dwError || ioHandle->eof)
    {
        *pEof = ioHandle->eof;
        return dwError;
    }

    if (ioHandle->curChar)
    {
        ioHandle->prevChar = ioHandle->curChar;
    }

    ioHandle->curChar = ioHandle->ioBuf[ioHandle->ioCursor];
    *nextChar = ioHandle->curChar;
    ioHandle->ioCursor++;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOBufferGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);
    BAIL_ON_INVALID_HANDLE(pPrevChar);

    if (ioHandle->ioCursor > 0)
    {
        *pPrevChar = ioHandle->prevChar;
    }
    else
    {
        dwError = LWREG_ERROR_NOT_HANDLED;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOBufferUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_BUFFER_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);


    if ((ioHandle->ioCursor - 1) >= 0)
    {
        ioHandle->ioCursor--;
        if (pPrevChar)
        {
            ioHandle->ioBuf[ioHandle->ioCursor] = *pPrevChar;
        }
    }
    else if (ioHandle->ioBufLen > 0)
    {
        ioHandle->ioCursor = 0;
        dwError = LWREG_ERROR_NOT_HANDLED;
    }

cleanup:
    return dwError;

error:
    goto cleanup;

}



DWORD
RegIOFileClose(
    HANDLE handle)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);
    if (ioHandle->fp && ioHandle->fp != stdin)
    {
        fclose(ioHandle->fp);
    }
    if (ioHandle->ioBuf)
    {
        RegMemoryFree(ioHandle->ioBuf);
    }
    RegIconvConvertClose(ioHandle->pivHandle);
    RegMemoryFree(ioHandle);


cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegIOClose(
    HANDLE handle)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOClose(handle);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOIsEOF(
    HANDLE handle,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOIsEOF(handle, pEof);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOReadData(
    HANDLE handle)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOReadData(handle);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOGetChar(handle, nextChar, pEof);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOGetPrevChar(handle, pPrevChar);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = ioHandle->pfn_RegIOUnGetChar(handle, pPrevChar);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOFileIsEOF(
    HANDLE handle,
    PBOOLEAN pEof)
{
    DWORD dwError = 0;

    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);
    BAIL_ON_INVALID_HANDLE(pEof);

    *pEof = ioHandle->eof;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOFileReadData(
    HANDLE handle)
{
    DWORD dwError = 0;
    DWORD items = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    BAIL_ON_INVALID_HANDLE(ioHandle);

    if (ioHandle->ioCursor < 0 ||
        ioHandle->ioCursor >= ioHandle->ioBufLen)
    {
        if (ioHandle->isUTF16)
        {
            items = RegIconvConvertReadBuf(ioHandle->pivHandle,
                                   ioHandle->fp,
                                   &ioHandle->ioBuf,
                                   (ssize_t *) &ioHandle->ioBufLen);
        }
        else
        {
            items = fread(ioHandle->ioBuf,
                          1,
                          ioHandle->ioBufLen,
                          ioHandle->fp);
        }
        if (items == 0)
        {
            ioHandle->eof = TRUE;
            return dwError;
        }
        ioHandle->ioBufLen = items;
        ioHandle->ioCursor = 0;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegIOFileGetChar(
    HANDLE handle,
    PCHAR nextChar,
    PBOOLEAN pEof)
{
    PREGIO_HANDLE ioHandle = (HANDLE) handle;
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(nextChar);
    BAIL_ON_INVALID_HANDLE(ioHandle);

    dwError = RegIOReadData(ioHandle);
    if (dwError || ioHandle->eof)
    {
        *pEof = ioHandle->eof;
        return dwError;
    }

    if (ioHandle->curChar)
    {
        ioHandle->prevChar = ioHandle->curChar;
    }

    ioHandle->curChar = ioHandle->ioBuf[ioHandle->ioCursor];
    *nextChar = ioHandle->curChar;
    ioHandle->ioCursor++;

cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegIOFileGetPrevChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);
    BAIL_ON_INVALID_HANDLE(pPrevChar);

    if (ioHandle->ioCursor > 0)
    {
        *pPrevChar = ioHandle->prevChar;
    }
    else
    {
        dwError = LWREG_ERROR_NOT_HANDLED;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegIOFileUnGetChar(
    HANDLE handle,
    PCHAR pPrevChar)
{
    DWORD dwError = 0;
    PREGIO_HANDLE ioHandle = (HANDLE) handle;

    BAIL_ON_INVALID_HANDLE(ioHandle);

    if ((ioHandle->ioCursor - 1) >= 0)
    {
        ioHandle->ioCursor--;
        if (pPrevChar)
        {
            ioHandle->ioBuf[ioHandle->ioCursor] = *pPrevChar;
        }
    }
    else if (ioHandle->ioBufLen > 0)
    {
        ioHandle->ioCursor = 0;
        dwError = LWREG_ERROR_NOT_HANDLED;
    }


cleanup:
    return dwError;

error:
    goto cleanup;
}
