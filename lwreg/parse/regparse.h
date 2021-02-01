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
