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
 *        regparse.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#ifndef REGPARSE_H
#define REGPARSE_H

#define REGPARSE_BUFSIZ (BUFSIZ * 2)

typedef struct _REGPARSE_HANDLE REGPARSE_HANDLE, *PREGPARSE_HANDLE;
typedef DWORD (*PFN_REG_CALLBACK)(PREG_PARSE_ITEM pItem, HANDLE userContext);

typedef struct _REGPARSE_CALLBACK_ENTRY
{
    PFN_REG_CALLBACK pfnCallback;
    HANDLE userContext;
    BOOLEAN used;
}  REGPARSE_CALLBACK_ENTRY, *PREGPARSE_CALLBACK_ENTRY;


typedef struct _REGPARSE_CALLBACK
{
    REGPARSE_CALLBACK_ENTRY callbacks[64];
    DWORD entries;
} REGPARSE_CALLBACK, *PREGPARSE_CALLBACK;


struct _REGPARSE_HANDLE
{
    HANDLE ioHandle;
    PREGLEX_ITEM lexHandle;
    REGLEX_TOKEN valueType; /* Distinguish between default @ and valueName */
    REGLEX_TOKEN dataType;  /* Type of data stored in valueName */
    PSTR keyName;           /* Current registry subkey */
    PSTR valueName;         /* Current valueName */
    PSTR attrName;          /* Registry schema field being addressed */
    BOOLEAN bTypeSet;       /* Registry schema type field is set */
    REG_PARSE_ITEM registryEntry;
    PSTR pszStringData;
    UCHAR *binaryData;
    DWORD binaryDataLen;
    DWORD binaryDataAllocLen;
    PVOID pCurrentAttrValue;
    DWORD dwCurrentAttrValueLen;
    DWORD dwCurrentAttrValueType;
    REGPARSE_CALLBACK parseCallback;
};


typedef struct _UCS2_STRING_ENTRY
{
    PWSTR ucs2String;
    DWORD ucs2StringLen;
} UCS2_STRING_ENTRY, *PUCS2_STRING_ENTRY;

void
RegParseFreeRegAttrData(
    HANDLE pHandle);

#if 1 /* Public parser API. */

DWORD
RegParseOpen(
    PSTR pszRegFileName,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    HANDLE *ppNewHandle);

void
RegParseClose(
    HANDLE pHandle);

DWORD
RegParseRegistry(
    HANDLE pHandle);

#endif /* Public parser API. */

void
RegParsePrintASCII(
    UCHAR *buf,
    DWORD buflen);

void RegParsePrintBinaryData(
    PUCHAR binaryData,
    DWORD binaryDataLen);

DWORD RegParseAppendData(
    PREGPARSE_HANDLE parseHandle,
    PSTR pszHexValue);

DWORD
RegParseBinaryData(
    PREGPARSE_HANDLE parseHandle);

void
RegParsePrintASCII(
    UCHAR *buf,
    DWORD buflen);

DWORD
RegParseRunCallbacks(
    PREGPARSE_HANDLE parseHandle);


DWORD
RegParseInstallCallback(
    PREGPARSE_HANDLE parseHandle,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    PDWORD indexCallback);

DWORD
RegParseRemoveCallback(
    PREGPARSE_HANDLE parseHandle,
    DWORD indexCallback);


DWORD
RegParseGetLineNumber(
    HANDLE pHandle,
    PDWORD pdwLineNum);

#endif
