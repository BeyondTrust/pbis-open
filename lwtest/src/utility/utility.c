#include "includes.h"

BOOL
StringsAreEqual(
    PCSTR pszStr1,
    PCSTR pszStr2
    )
{
    /* If both are NULL, then they are 'equal'.
     * If only one is NULL, then they are not 'equal'.  */
    if ( pszStr1 == NULL )
    {
        if ( pszStr2 == NULL )
            return 1;
        else
            return 0;
    }
    else 
    {
        if ( pszStr2 == NULL )
            return 0;
    }
    return !strcmp(pszStr1, pszStr2);
}

BOOL
StringsNoCaseAreEqual(
    PCSTR pszStr1,
    PCSTR pszStr2
    )
{
    /* If both are NULL, then they are 'equal'.
     * If only one is NULL, then they are not 'equal'.  */
    if ( pszStr1 == NULL )
    {
        if ( pszStr2 == NULL )
            return 1;
        else
            return 0;
    }
    else 
    {
        if ( pszStr2 == NULL )
            return 0;
    }
    return !strcasecmp(pszStr1, pszStr2);
}


void
Lwt_strcat(
    char *str,
    size_t size,
    const char *addition
    )
{
    if ( size > 0 )
    {
        size_t len = strlen(str);
        if ( len < size - 1 )
        {
            size_t i, j;

            for ( i = len, j = 0; i < size - 1 && addition[j]; i++, j++)
                str[i] = addition[j];
            str[i] = '\0';
        }
    }
}

/*
 * See
 * http://www.faqs.org/docs/abs/HTML/exitcodes.html#EXITCODESREF
 * for error codes to avoid.
 */
DWORD
LwtMapErrorToProgramStatus(
    DWORD dwError
    )
{
    DWORD dwStatus = 0;

    if (dwError == LW_ERROR_SUCCESS)
        dwStatus = 0;
    else if (dwError == LW_ERROR_TEST_FAILED)
        dwStatus = 3;
    else if (dwError == LW_ERROR_TEST_SKIPPED)
        dwStatus = 4;
    else
        dwStatus = 1;

    return dwStatus;
}


static const unsigned char Base64Table[64] = 
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

unsigned char bits(int c)
{
    if ( 'A' <= c && c <= 'Z' )
        return c - 'A';             /* 0 .. 25 */
    else if ( 'a' <= c && c <= 'z' )
        return 26 + (c - 'a');      /* 26 .. 51 */
    else if ( '0' <= c && c <= '9' )
        return 52 + (c - '0');      /* 52 .. 61 */
    else if ( c == '+' )
        return 62;                  /* 62 */
    else if ( c == '/' )
        return 63;                  /* 63 */
    else
        return 255;                 /* Shouldn't get here! */
}




DWORD
Base64Decode(
    PCSTR  pszInput,
    PSTR  *ppOctetString,
    PDWORD pdwByteCount
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PSTR pOctetString = NULL;
    DWORD dwByteCount = 0;

    char buf[4] = { 0 };
    PCSTR pszStr = pszInput;

    DWORD dwBase64CharacterCount = 0;

    int c;
    int i, j;

    while ( (c = *pszStr) != '\0' )
    {
        if ( ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
             c == '+' || c == '/' )
        {
            dwBase64CharacterCount++;
        }
        pszStr++;
    }

    /* 6 bits per characeter, 8 bits per byte */
    dwByteCount = (dwBase64CharacterCount * 6)/ 8;

    pOctetString = calloc(dwByteCount, 1);
    if ( ! pOctetString )
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    

    pszStr = pszInput;
    i = 0;
    j = 0;
    while ( (c = *pszStr) != '\0' )
    {
        if ( ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '+' ||c == '/' )
        {
            buf[i++] = c;
       
            if ( i == 4 )
            {
                pOctetString[j++] = 
                    ((bits(buf[0]) & 0x3f) << 2) | ((bits(buf[1]) & 0x30) >> 4);
                pOctetString[j++] = 
                    ((bits(buf[1]) & 0x0f) << 4) | ((bits(buf[2]) & 0x3c) >> 2);
                pOctetString[j++] = 
                    ((bits(buf[2]) & 0x03) << 6) | ((bits(buf[3]) & 0x3f) >> 0);

                buf[0] = buf[1] = buf[2] = buf[3] = '\0';
                i = 0;
            }
        }
        pszStr++;
    }
    while ( i < 4 )
        buf[i++] = 'A';


    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[0]) & 0x3f) << 2) | ((bits(buf[1]) & 0x30) >> 4);
    }
    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[1]) & 0x0f) << 4) | ((bits(buf[2]) & 0x3c) >> 2);
    }
    if ( j < dwByteCount )
    {
        pOctetString[j++] = 
            ((bits(buf[2]) & 0x03) << 6) | ((bits(buf[3]) & 0x3f) >> 0);
    }

cleanup:

    *pdwByteCount = dwByteCount;
    *ppOctetString = pOctetString;

    return dwError;

error:

    free(pOctetString);
    pOctetString = NULL;

    dwByteCount = 0;

    goto cleanup;
}


        


