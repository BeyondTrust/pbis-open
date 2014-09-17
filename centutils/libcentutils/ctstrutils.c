/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "config.h"
#include "ctbase.h"
#include "ctstrutils.h"
#include <inttypes.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_TERMIO_H
#include <sys/termio.h>
#endif

DWORD
CTAllocateString(
    PCSTR pszInputString,
    PSTR * ppszOutputString
    )
{
    DWORD ceError = ERROR_SUCCESS;
    size_t len = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    len = strlen(pszInputString);
    ceError = CTAllocateMemory(len+1, (PVOID *)&pszOutputString);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memcpy(pszOutputString, pszInputString, len);
    pszOutputString[len] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(ceError);
}

DWORD
CTStrndup(
    PCSTR pszInputString,
    size_t size2,
    PSTR * ppszOutputString
    )
{
    DWORD ceError = ERROR_SUCCESS;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    copylen = strlen(pszInputString);
    if(copylen > size2)
        copylen = size2;
    ceError = CTAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_CENTERIS_ERROR(ceError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(ceError);
}

void
CTFreeString(
    PSTR pszString
    )
{
    if (pszString) {
        CTFreeMemory(pszString);
    }
}

void
CTFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                CTFreeString(ppStringArray[i]);
            }
        }

        CTFreeMemory(ppStringArray);
    }

    return;
}

void
CTFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    )
{

    if ( ppStringArray ) {
        size_t i;
        for(i = 0; ppStringArray[i] != NULL; i++)
        {
            CTFreeString(ppStringArray[i]);
        }

        CTFreeMemory(ppStringArray);
    }

    return;
}

void
CTStrToUpper(
    PSTR pszString
    )
{
    //PSTR pszTmp = pszString;

    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = toupper((int)*pszString);
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
            *pszString = tolower((int)*pszString);
            pszString++;
        }
    }
}

DWORD
CTEscapeString(
    PCSTR pszOrig,
    PSTR * ppszEscapedString
    )
{
    DWORD ceError = ERROR_SUCCESS;
    int nQuotes = 0;
    PCSTR pszTmp = pszOrig;
    PSTR pszNew = NULL;
    PSTR pszNewTmp = NULL;

    if ( !ppszEscapedString || !pszOrig ) {
        ceError = ERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while(pszTmp && *pszTmp)
    {
        if (*pszTmp=='\'') {
            nQuotes++;
        }
        pszTmp++;
    }

    if (!nQuotes) {
        ceError = CTAllocateString(pszOrig, &pszNew);
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        /*
         * We are going to escape each single quote and enclose it in two other
         * single-quotes
         */
        ceError = CTAllocateMemory( strlen(pszOrig)+3*nQuotes+1, (PVOID*)&pszNew );
        BAIL_ON_CENTERIS_ERROR(ceError);

        pszTmp = pszOrig;
        pszNewTmp = pszNew;

        while(pszTmp && *pszTmp)
        {
            if (*pszTmp=='\'') {
                *pszNewTmp++='\'';
                *pszNewTmp++='\\';
                *pszNewTmp++='\'';
                *pszNewTmp++='\'';
                pszTmp++;
            }
            else {
                *pszNewTmp++ = *pszTmp++;
            }
        }
        *pszNewTmp = '\0';
    }

    *ppszEscapedString = pszNew;
    pszNew = NULL;

error:

    CT_SAFE_FREE_STRING(pszNew);

    return ceError;
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

DWORD
CTAllocateStringPrintfV(
    PSTR* result,
    PCSTR format,
    va_list args
    )
{
    DWORD ceError = ERROR_SUCCESS;
    char *smallBuffer;
    unsigned int bufsize;
    int requiredLength;
    unsigned int newRequiredLength;
    PSTR outputString = NULL;
    va_list args2;

    va_copy(args2, args);

    bufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        ceError = CTAllocateMemory(bufsize, (PVOID*) &smallBuffer);
        CLEANUP_ON_DWORD(ceError);
        requiredLength = vsnprintf(smallBuffer, bufsize, format, args);
        if (requiredLength < 0)
        {
            bufsize *= 2;
        }
        CTFreeMemory(smallBuffer);
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        ceError = ERROR_OUTOFMEMORY;
        CLEANUP_ON_DWORD(ceError);
    }

    ceError = CTAllocateMemory(requiredLength + 2, (PVOID*)&outputString);
    CLEANUP_ON_DWORD(ceError);

    newRequiredLength = vsnprintf(outputString, requiredLength + 1, format, args2);
    if (newRequiredLength < 0)
    {
        ceError = LwMapErrnoToLwError(errno);
        CLEANUP_ON_DWORD(ceError);
    }
    else if (newRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        ceError = ERROR_OUTOFMEMORY;
        CLEANUP_ON_DWORD(ceError);
    }
    else if (newRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }

cleanup:
    va_end(args2);

    if (ceError)
    {
        if (outputString)
        {
            CTFreeMemory(outputString);
            outputString = NULL;
        }
    }
    *result = outputString;
    return ceError;
}

DWORD
CTAllocateStringPrintf(
    PSTR* result,
    PCSTR format,
    ...
    )
{
    DWORD ceError;

    va_list args;
    va_start(args, format);
    ceError = CTAllocateStringPrintfV(result, format, args);
    va_end(args);

    return ceError;
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

static DWORD NullTerminate(StringBuffer *buffer)
{
    DWORD ceError = CTArrayAppend(buffer, 1, "\0", 1);
    CLEANUP_ON_DWORD(ceError);
    buffer->size--;
cleanup:
    return ceError;
}

DWORD
CTStringBufferConstruct(StringBuffer* buffer)
{
    DWORD ceError = ERROR_SUCCESS;
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayConstruct(buffer, 1));

error:

    return ceError;
}

void
CTStringBufferDestroy(StringBuffer* buffer)
{
    CTArrayFree(buffer);
}

void
CTStringBufferClear(
		StringBuffer* buffer
		)
{
    CTArrayRemove(buffer, 0, 1, buffer->size);
    NullTerminate(buffer);
}

char*
CTStringBufferFreeze(StringBuffer* buffer)
{
    char* data = buffer->data;
    buffer->size = buffer->capacity = 0;
    buffer->data = NULL;

    return data;
}

DWORD
EnsureSpace(StringBuffer* buffer, unsigned int space)
{
    DWORD ceError = ERROR_SUCCESS;
    if(space <= buffer->capacity)
    {
        ceError = CTSetCapacity(buffer, 1, space + 1);
        CLEANUP_ON_DWORD(ceError);
        ceError = NullTerminate(buffer);
        CLEANUP_ON_DWORD(ceError);
    }
    
cleanup:
    return ceError;
}

DWORD
CTStringBufferAppend(StringBuffer* buffer, const char* str)
{
    return CTStringBufferAppendLength(buffer, str, strlen(str));
}

DWORD
CTStringBufferAppendLength(StringBuffer* buffer, const char* str, unsigned int length)
{
    DWORD ceError = CTArrayAppend(buffer, 1, str, length);
    CLEANUP_ON_DWORD(ceError);
    ceError = NullTerminate(buffer);
    CLEANUP_ON_DWORD(ceError);

cleanup:
    return ceError;
}

DWORD
CTStringBufferAppendChar(StringBuffer* buffer, char c)
{
    return CTStringBufferAppendLength(buffer, &c, 1);
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

void CTFreeParseTokenContents(CTParseToken *token)
{
    CT_SAFE_FREE_STRING(token->value);
    CT_SAFE_FREE_STRING(token->trailingSeparator);
}

size_t CTGetTokenLen(const CTParseToken *token)
{
    size_t len = 0;
    if(token->value != NULL)
    {
        len += strlen(token->value);
    }
    if(token->trailingSeparator != NULL)
    {
        len += strlen(token->trailingSeparator);
    }
    return len;
}

void CTAppendTokenString(char **pos, const CTParseToken *token)
{
    size_t len;
    if(token->value != NULL)
    {
        len = strlen(token->value);
        memcpy(*pos, token->value, len);
        *pos += len;
    }
    if(token->trailingSeparator != NULL)
    {
        len = strlen(token->trailingSeparator);
        memcpy(*pos, token->trailingSeparator, len);
        *pos += len;
    }
}

DWORD CTReadToken(const char **pos, CTParseToken *store, const char *includeSeparators, const char *excludeSeparators, const char *trimBack)
{
    DWORD ceError = ERROR_SUCCESS;
    char const * token_start = *pos;
    char const * white_start, * white_end;

    memset(store, 0, sizeof(*store));
    while(**pos != '\0' &&
            strchr(includeSeparators, **pos) == NULL &&
            strchr(excludeSeparators, **pos) == NULL)
    {
        (*pos)++;
    }
    white_start = *pos;
    while (**pos != '\0' && strchr(includeSeparators, **pos) != NULL)
    {
        (*pos)++;
    }
    white_end = *pos;
    while (white_start > token_start &&
            white_start[-1] != '\0' &&
            strchr(trimBack, white_start[-1]) != NULL)
    {
        white_start--;
    }
    if(token_start != white_start)
    {
        ceError = CTStrndup(token_start, white_start - token_start,
                    &store->value);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(white_start != white_end)
    {
        ceError = CTStrndup(white_start, white_end - white_start,
                    &store->trailingSeparator);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:
    if (ceError)
        CTFreeParseTokenContents(store);
    return ceError;
}

DWORD CTCopyTokenContents(CTParseToken *dest, const CTParseToken *source)
{
    DWORD ceError = ERROR_SUCCESS;
    memset(dest, 0, sizeof(*dest));
    if(source->value != NULL)
    {
        ceError = CTDupOrNullStr(source->value, &dest->value);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(source->trailingSeparator != NULL)
    {
        ceError = CTDupOrNullStr(source->trailingSeparator, &dest->trailingSeparator);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if(ceError)
        CTFreeParseTokenContents(dest);
    return ceError;
}

DWORD CTDupOrNullStr(const char *src, char **dest)
{
    if(src == NULL)
    {
        *dest = NULL;
        return ERROR_SUCCESS;
    }
    return CTStrdup(src, dest);
}

DWORD CTWriteToken(FILE *file, CTParseToken *token)
{
    const char *value = token->value;
    const char *white = token->trailingSeparator;
    if(value == NULL)
        value = "";
    if(white == NULL)
        white = "";
    return CTFilePrintf(file, "%s%s", value, white);
}

void CTFreeParseToken(CTParseToken **token)
{
    if(*token != NULL)
    {
        CTFreeParseTokenContents(*token);
        CT_SAFE_FREE_MEMORY(*token);
    }
}

DWORD CTCopyToken(const CTParseToken *source, CTParseToken **dest)
{
    DWORD ceError = ERROR_SUCCESS;
    *dest = NULL;
    if(source != NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) dest));
        BAIL_ON_CENTERIS_ERROR(ceError = CTCopyTokenContents(*dest, source));
    }

error:
    if (ceError)
    {
        CTFreeParseToken(dest);
    }
    return ceError;
}

DWORD CTGetTerminalWidth(int terminalFid, int *width)
{
    DWORD ceError = ERROR_SUCCESS;
#ifdef TIOCGWINSZ
    struct winsize size = {0};
    const char *fromEnv = getenv("COLUMNS");

    if(ioctl(terminalFid, TIOCGWINSZ, &size) == -1 || size.ws_col == 0)
    {
        if(fromEnv != NULL)
        {
            size.ws_col = atoi(fromEnv);
        }
        else
        {
            ceError = LwMapErrnoToLwError(errno);
            CLEANUP_ON_DWORD(ceError);
        }
    }

    if(size.ws_col < 1)
    {
        ceError = ERROR_INVALID_OPERATION;
        CLEANUP_ON_DWORD(ceError);
    }
    *width = size.ws_col;
#else
    *width = -1;
#endif

cleanup:
    return ceError;
}

static int CharLen(char c, int tabWidth)
{
    switch(c)
    {
        case '\t':
            return tabWidth;
        default:
            return 1;
    }
}

//Find out how far the next line is indented
static int NextLineIndent(PCSTR input, int tabWidth)
{
    int indent = 0;
    PCSTR nextLine1 = strchr(input, '\n');
    PCSTR nextLine2 = strchr(input, '\r');
    if(nextLine2 != NULL && (nextLine1 == NULL || nextLine2 < nextLine1))
        nextLine1 = nextLine2;
    //This is the last line
    if(nextLine1 == NULL)
        return -1;
    if(*nextLine1 == '\r')
        nextLine1++;
    if(*nextLine1 == '\n')
        nextLine1++;
    while(*nextLine1 == ' ' || *nextLine1 == '\t')
    {
        indent += CharLen(*nextLine1, tabWidth);
        nextLine1++;
    }
    if(*nextLine1 == '\r' || *nextLine1 == '\n' || *nextLine1 == '\0')
    {
        //Blank lines don't have indents
        indent = -1;
    }
    
    return indent;
}

DWORD CTWordWrap(PCSTR input, PSTR *output, int tabWidth, int columns)
{
    DWORD ceError = ERROR_SUCCESS;
    StringBuffer result;
    int pos = 0;
    int start;
    int column;
    int i;
    int previousIndent = -1;
    int nextIndent;

    memset(&result, 0, sizeof(result));

    while(input[pos] != '\0')
    {
        int indentColumns;
        int lastWhite = 0;
        start = pos;
        column = 0;
        while(input[pos] == ' ' || input[pos] == '\t')
        {
            column += CharLen(input[pos], tabWidth);
            pos++;
        }
        indentColumns = column;
        nextIndent = NextLineIndent(input + pos, tabWidth);
        if(indentColumns > 0 && (nextIndent == indentColumns ||
                previousIndent == indentColumns))
        {
            //The line following this one is indented the same amount. If this
            //line wraps, we'll indent it an extra tab to differentiate the
            //two lines.
            indentColumns += tabWidth;
        }
        if(input[pos] == '\r' && input[pos] == '\n')
        {
            //Blank lines don't have indents
            previousIndent = -1;
        }
        else
            previousIndent = column;

        for(i = 0; i < column; i++)
        {
            ceError = CTStringBufferAppendLength(&result, " ", 1);
            CLEANUP_ON_DWORD(ceError);
        }
        start = pos;
        while(input[pos] != '\n' && input[pos] != '\r' && input[pos] != '\0')
        {
            if(input[pos] == '\f')
            {
                //This marks how far to indent the text
                ceError = CTStringBufferAppendLength(&result, input + start, pos - start);
                CLEANUP_ON_DWORD(ceError);
                indentColumns = column;
                pos++;
                start = pos;
            }
            if(column + CharLen(input[pos], tabWidth) > columns && columns != -1)
            {
                //Wrap the line

                //If there was any white space in this line, break it there,
                //otherwise break it in the middle of the word
                if(lastWhite > start)
                    pos = lastWhite;
                //First the text in this line
                ceError = CTStringBufferAppendLength(&result, input + start, pos - start);
                CLEANUP_ON_DWORD(ceError);
                //Now add the newline
                ceError = CTStringBufferAppendLength(&result, "\n", 1);
                CLEANUP_ON_DWORD(ceError);
                //Finally the indent for the new line
                for(column = 0; column < indentColumns; column++)
                {
                    ceError = CTStringBufferAppendLength(&result, " ", 1);
                    CLEANUP_ON_DWORD(ceError);
                }
     
                //Skip any whitespace
                while(input[pos] == ' ' || input[pos] == '\t')
                {
                    pos++;
                }
                start = pos;
                continue;
            }
            if(isspace((int)input[pos]))
                lastWhite = pos;
            if(input[pos] == '\t')
            {
                //First add the text before the tab
                ceError = CTStringBufferAppendLength(&result, input + start, pos - start);
                CLEANUP_ON_DWORD(ceError);
                //Now expand the tab into spaces
                for(i = 0; i < tabWidth; i++)
                {
                    ceError = CTStringBufferAppendLength(&result, " ", 1);
                    CLEANUP_ON_DWORD(ceError);
                }
                start = pos + 1;
            }
            column += CharLen(input[pos], tabWidth);
            pos++;
        }
        if(input[pos] == '\n')
            pos++;
        if(input[pos] == '\r')
            pos++;
        //Copy the line
        ceError = CTStringBufferAppendLength(&result, input + start, pos - start);
        CLEANUP_ON_DWORD(ceError);
    }
    ceError = CTStringBufferAppendLength(&result, "\0", 1);
    CLEANUP_ON_DWORD(ceError);

    *output = result.data;
    result.data = NULL;

cleanup:
    CTStringBufferDestroy(&result);
    return ceError;
}
