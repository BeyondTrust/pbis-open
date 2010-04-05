/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

DWORD
LWAllocateString(
    PCSTR pszInputString,
    PSTR * ppszOutputString
    )
{
    DWORD dwError = 0;
    size_t len = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }
    
    len = strlen(pszInputString);
    dwError = LWAllocateMemory(len+1, (PVOID *)&pszOutputString);
    BAIL_ON_MAC_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, len);
    pszOutputString[len] = 0;
    
    *ppszOutputString = pszOutputString;
    
cleanup:

    return dwError;

error:
    
    *ppszOutputString = NULL;
    
    goto cleanup;
}

VOID
LWFreeString(
    PSTR pszString
    )
{
    if (pszString) {
        LWFreeMemory(pszString);
    }
}

void
LWStrToUpper(
    PSTR pszString
    )
{
    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = toupper(*pszString);
            pszString++;
        }
    }
}

void
LWStrToLower(
    PSTR pszString
    )
{
    if (pszString != NULL) {
        while (*pszString != '\0') {
            *pszString = tolower(*pszString);
            pszString++;
        }
    }
}

void
LWStripLeadingWhitespace(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL ||
        *pszString == '\0' ||
        !isspace((int)*pszString))
    {
        return;
    }

    while (pszTmp != NULL &&
           *pszTmp != '\0' &&
           isspace((int)*pszTmp))
    {
        pszTmp++;
    }

    while (pszTmp != NULL &&
           *pszTmp != '\0')
    {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}

void
LWStripTrailingWhitespace(
    PSTR pszString
    )
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (pszString == NULL ||
        *pszString == '\0')
    {
        return;
    }

    while (pszTmp != NULL &&
           *pszTmp != '\0')
    {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL)
    {
        *pszLastSpace = '\0';
    }
}

// Remove leading whitespace and newline characters which are added by libxml2 only.
void
LWRemoveLeadingWhitespacesOnly(
    PSTR pszString
    )
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL ||
        *pszString == '\0' ||
        !isspace((int)*pszString))
    {
        return;
    }

    if( *pszTmp == '\n' )
        pszTmp++;

    while (pszTmp != NULL &&
           *pszTmp != '\0' &&
           isspace((int)*pszTmp))
    {
        if ( *pszTmp == '\n' )
        {
            break;
        }
        pszTmp++;
    }

    while (pszTmp != NULL &&
           *pszTmp != '\0')
    {
        *pszNew++ = *pszTmp++;
    }

    *pszNew = '\0';
}

// Remove trailing whitespace and newline characters which are added by libxml2 only.
void
LWRemoveTrailingWhitespacesOnly(
    PSTR pszString
    )
{
    if (!IsNullOrEmptyString(pszString))
    {
        PSTR pszLastNewLine = NULL;
        PSTR pszTmp = pszString;
        
        while (pszTmp != NULL &&
               *pszTmp != '\0')
        {
            if(*pszTmp == '\n')
            {
                pszLastNewLine = pszTmp;
            }
            pszTmp++;
        }
    
        if (pszLastNewLine != NULL)
        {
            if( *(pszLastNewLine-1) != '\n' )
            {
                *pszLastNewLine = '\n';
                pszLastNewLine++;            
            }
            *pszLastNewLine = '\0';
        }
    }
}

void
LWStripWhitespace(
    PSTR pszString
    )
{
    if (!IsNullOrEmptyString(pszString))
    {
        LWStripLeadingWhitespace(pszString);
        LWStripTrailingWhitespace(pszString);
    }
}

DWORD
LWAllocateStringPrintf(
    PSTR* result,
    PCSTR format,
    ...
    )
{
    DWORD dwError = 0;

    va_list args;
    va_start(args, format);
    dwError = LWAllocateStringPrintfV(result, format, args);
    va_end(args);

    return dwError;
}

DWORD
LWAllocateStringPrintfV(
    PSTR* result,
    PCSTR format,
    va_list args
    )
{
    DWORD dwError = 0;
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
        dwError = LWAllocateMemory(bufsize, (PVOID*) &smallBuffer);
        BAIL_ON_MAC_ERROR(dwError);
        requiredLength = vsnprintf(smallBuffer, bufsize, format, args);
        if (requiredLength < 0)
        {
            bufsize *= 2;
        }
        LWFreeMemory(smallBuffer);
    } while (requiredLength < 0);

    if (requiredLength >= 4000000)
    {
        dwError = ENOMEM;
        BAIL_ON_MAC_ERROR(dwError);
    }

    dwError = LWAllocateMemory(requiredLength + 2, (PVOID*)&outputString);
    BAIL_ON_MAC_ERROR(dwError);

    newRequiredLength = vsnprintf(outputString, requiredLength + 1, format, args2);
    if (newRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }
    else if (newRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = eMemoryAllocError;
        BAIL_ON_MAC_ERROR(dwError);
    }
    else if (newRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }

    *result = outputString;

cleanup:

    va_end(args2);

    return dwError;
    
error:

    LW_SAFE_FREE_STRING(outputString);
    
    *result = NULL;
    
    goto cleanup;
}

BOOLEAN
LWStrStartsWith(
    PCSTR str,
    PCSTR prefix
    )
{
    if (prefix == NULL)
        return TRUE;
    
    if (str == NULL)
        return FALSE;

    return strncmp(str, prefix, strlen(prefix)) == 0;
}

BOOLEAN
LWStrEndsWith(
    PCSTR str,
    PCSTR suffix
    )
{
    size_t strLen, suffixLen;
    
    if (suffix == NULL)
        return TRUE;
    
    if (str == NULL)
        return FALSE;

    strLen = strlen(str);
    
    suffixLen = strlen(suffix);
    
    if (suffixLen > strLen)
        return FALSE;

    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

BOOLEAN
LWIsAllDigit(
    PCSTR pszVal
    )
{
    while (pszVal && *pszVal)
    {
        if (!isdigit((int)*pszVal++))
        {
            return FALSE;
        }
    }

    return TRUE;
}

DWORD
LWStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    )
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LWAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_MAC_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

error:

    *ppszOutputString = pszOutputString;

    return(dwError);
}


