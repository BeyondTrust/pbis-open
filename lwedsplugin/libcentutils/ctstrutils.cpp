/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"
#include "ctstrutils.h"
#include <inttypes.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_TERMIO_H
#include <sys/termio.h>
#endif
#include "Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

void
CTStrToUpper(
    PSTR pszString
    )
{
    //PSTR pszTmp = pszString;

    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = toupper(*pszString);
            pszString++;
        }
    }
}

void
CTStrToLower(
    PSTR pszString
    )
{
    //PSTR pszTmp = pszString;

    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = tolower(*pszString);
            pszString++;
        }
    }
}

void
CTStripLeadingWhitespace(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

void
CTStripTrailingWhitespace(
    PSTR pszString
    )
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

// Remove leading whitespace and newline characters which are added by libxml2 only.
void
CTRemoveLeadingWhitespacesOnly(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    if( *pszTmp == '\n' )
        pszTmp++;

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        if ( *pszTmp == '\n' )
            break;
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }

    *pszNew = '\0';
}

// Remove trailing whitespace and newline characters which are added by libxml2 only.
void
CTRemoveTrailingWhitespacesOnly(
    PSTR pszString
    )
{
    PSTR pszLastNewLine = NULL;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        if(*pszTmp == '\n')
            pszLastNewLine = pszTmp;
        pszTmp++;
    }

    if (pszLastNewLine != NULL) {
        if( *(pszLastNewLine-1) != '\n' ){
            *pszLastNewLine = '\n';
            pszLastNewLine++;            
        }
        *pszLastNewLine = '\0';
    }
}

void
CTStripWhitespace(
    PSTR pszString
    )
{
    if (pszString == NULL || *pszString == '\0') {
        return;
    }

    CTStripLeadingWhitespace(pszString);
    CTStripTrailingWhitespace(pszString);
}

long
LWAllocateStringPrintfV(
    PSTR* result,
    PCSTR format,
    va_list args
    )
{
    long macError = eDSNoErr;
    char *smallBuffer;
    int bufsize;
    int requiredLength;
    int newRequiredLength;
    PSTR outputString = NULL;
    va_list args2;

    va_copy(args2, args);

    bufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        macError = LWAllocateMemory(bufsize, (PVOID*) &smallBuffer);
        CLEANUP_ON_CENTERROR(macError);
        requiredLength = vsnprintf(smallBuffer, bufsize, format, args);
        if (requiredLength < 0)
        {
            bufsize *= 2;
        }
        LWFreeMemory(smallBuffer);
    } while (requiredLength < 0);

    if (requiredLength >= 4000000)
    {
        macError = eMemoryAllocError;
        CLEANUP_ON_CENTERROR(macError);
    }

    macError = LWAllocateMemory(requiredLength + 2, (PVOID*)&outputString);
    CLEANUP_ON_CENTERROR(macError);

    newRequiredLength = vsnprintf(outputString, requiredLength + 1, format, args2);
    if (newRequiredLength < 0)
    {
        macError = CTMapSystemError(errno);
        CLEANUP_ON_CENTERROR(macError);
    }
    else if (newRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        macError = eMemoryAllocError;
        CLEANUP_ON_CENTERROR(macError);
    }
    else if (newRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }

cleanup:

    va_end(args2);

    if (macError)
    {
        if (outputString)
        {
            LWFreeMemory(outputString);
            outputString = NULL;
        }
    }
    *result = outputString;
    return macError;
}

long
LWAllocateStringPrintf(
    PSTR* result,
    PCSTR format,
    ...
    )
{
    long macError;

    va_list args;
    va_start(args, format);
    macError = LWAllocateStringPrintfV(result, format, args);
    va_end(args);

    return macError;
}

BOOLEAN
CTStrStartsWith(
    PCSTR str,
    PCSTR prefix
    )
{
    if(prefix == NULL)
        return TRUE;
    if(str == NULL)
        return FALSE;

    return strncmp(str, prefix, strlen(prefix)) == 0;
}

BOOLEAN
CTStrEndsWith(
    PCSTR str,
    PCSTR suffix
    )
{
    size_t strLen, suffixLen;
    if(suffix == NULL)
        return TRUE;
    if(str == NULL)
        return FALSE;

    strLen = strlen(str);
    suffixLen = strlen(suffix);
    if(suffixLen > strLen)
        return FALSE;

    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

BOOLEAN
CTIsAllDigit(
    PCSTR pszVal
    )
{
  while (pszVal && *pszVal)
    if (!isdigit((int)*pszVal++)) return FALSE;

  return TRUE;
}

