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
 *        regiconv.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser file I/O utf-16/utf-8 decoding layer
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#include "includes.h"


struct _IV_CONVERT_CTX
{
    iconv_t iconvHandle;      /* conversion descriptor */
    char    inbuf[BUFSIZ];    /* input buffer */
    char    *inchar;          /* ptr to input character */
    size_t  inbytesleft;      /* num bytes left in input buffer */
    char    outbuf[BUFSIZ];   /* output buffer */
    char    *outchar;         /* ptr to output character */
    size_t  outbytesleft;     /* num bytes left in output buffer */
};


int RegIconvConvertOpen(
    PIV_CONVERT_CTX *ppivHandle,
    char *ivToCode,
    char *ivFromCode)
{
    PIV_CONVERT_CTX pivHandle = NULL;

    pivHandle = (PIV_CONVERT_CTX) calloc(1, sizeof(IV_CONVERT_CTX));
    if (!pivHandle)
    {
        return -1;
    }

    /* Initiate conversion -- get conversion descriptor */
    pivHandle->iconvHandle = iconv_open(ivToCode, ivFromCode);
    if (pivHandle->iconvHandle == (iconv_t) -1)
    {
        free(pivHandle);
        return -1;
    }

    *ppivHandle = pivHandle;
    return 0;
}


void RegIconvConvertClose(
    PIV_CONVERT_CTX pivHandle)
{
    if (pivHandle && pivHandle->iconvHandle)
    {
        iconv_close(pivHandle->iconvHandle);
        free(pivHandle);
    }
}


int RegIconvConvertBuffer(
    PIV_CONVERT_CTX pivHandle,
    PBYTE pszInBuf,
    SSIZE_T inBufLen,
    PCHAR pszOutBuf,
    SSIZE_T *pInBufUsed,
    SSIZE_T *pOutBufLen)
{
    int status;
    static SSIZE_T bufsiz = BUFSIZ;
    DWORD bytesRead = 0;
    DWORD copySize = 0;
    SSIZE_T outBufLen = 0;

    pivHandle->inchar = pivHandle->inbuf;
    pivHandle->outchar = pivHandle->outbuf; /* points to output buffer */
    pivHandle->outbytesleft = bufsiz;       /* no of bytes to be converted */

    copySize = bufsiz - pivHandle->inbytesleft;
    bytesRead = inBufLen < copySize ? inBufLen : copySize;
    memcpy(pivHandle->inbuf + pivHandle->inbytesleft,
           pszInBuf,
           bytesRead);

    pivHandle->inbytesleft += bytesRead;
 
    status = iconv(pivHandle->iconvHandle,
                    (ICONV_IN_TYPE)&pivHandle->inchar, &pivHandle->inbytesleft,
                    &pivHandle->outchar, &pivHandle->outbytesleft);
    if (status == -1 && (errno == EINVAL || errno == E2BIG))
    {
        /* Input conversion stopped due to an incomplete
         * character or shift sequence at the end of the
         * input buffer.
         *
         * Copy data left, to the start of buffer
         */
        memcpy(pivHandle->inbuf, pivHandle->inchar, pivHandle->inbytesleft);
        *pInBufUsed = bytesRead;
    }
    else if ((status == -1) && (errno == EILSEQ))
    {
        /* Input conversion stopped due to an input byte
         * that does not belong to the input codeset.
         */
        return -1;
    }

    outBufLen = BUFSIZ - pivHandle->outbytesleft;
    if (pszOutBuf && pOutBufLen)
    {
        *pOutBufLen = outBufLen;
        *pInBufUsed = bytesRead - pivHandle->inbytesleft;
        memcpy(pszOutBuf, pivHandle->outbuf, *pOutBufLen);
    }
    return outBufLen;
}


int RegIconvConvertReadBuf(
    PIV_CONVERT_CTX pivHandle,
    FILE *fp,
    char **pszOutBuf,
    SSIZE_T *pOutBufLen)
{
    int status;
    static SSIZE_T bufsiz = BUFSIZ;
    DWORD bytesRead;

    pivHandle->inchar = pivHandle->inbuf;          /* points to input buffer */
    pivHandle->outchar = pivHandle->outbuf;        /* points to output buffer */
    pivHandle->outbytesleft = bufsiz;   /* no of bytes to be converted */
    bytesRead = fread(pivHandle->inbuf + pivHandle->inbytesleft,
                                 1,
                                 bufsiz - pivHandle->inbytesleft,
                                 fp);
    if (bytesRead == 0)
    {
        return 0;
    }

    pivHandle->inbytesleft += bytesRead;
    if (pivHandle->inbytesleft == 0)
    {
        return 0;
    }

    status = iconv(pivHandle->iconvHandle,
                    (ICONV_IN_TYPE)&pivHandle->inchar, &pivHandle->inbytesleft,
                    &pivHandle->outchar, &pivHandle->outbytesleft);
    if (status == -1 && (errno == EINVAL || errno == E2BIG))
    {
        /* Input conversion stopped due to an incomplete
         * character or shift sequence at the end of the
         * input buffer.
         *
         * Copy data left, to the start of buffer
         */
        memcpy(pivHandle->inbuf, pivHandle->inchar, pivHandle->inbytesleft);
    }
    else if ((status == -1) && (errno == EILSEQ))
    {
        /* Input conversion stopped due to an input byte
         * that does not belong to the input codeset.
         */
        return -1;
    }

    if (pszOutBuf && pOutBufLen)
    {
        memcpy(*pszOutBuf, pivHandle->outbuf, BUFSIZ-pivHandle->outbytesleft);
        *pOutBufLen = BUFSIZ-pivHandle->outbytesleft;
    }
    return BUFSIZ-pivHandle->outbytesleft;
}


int RegIconvConvertGetWriteBuf(
    PIV_CONVERT_CTX pivHandle,
    char **pszOutBuf,
    SSIZE_T *pOutBufLen)
{
    if (!pivHandle)
    {
        errno = EINVAL;
        return -1;
    }

    *pszOutBuf = pivHandle->outbuf;
    *pOutBufLen = BUFSIZ-pivHandle->outbytesleft;
    return 0;
}


int RegIconvConvertWriteBuf(
    PIV_CONVERT_CTX pivHandle,
    FILE *fp)
{
    int status = 0;

    status = fwrite(pivHandle->outbuf, 1, BUFSIZ-pivHandle->outbytesleft, fp);
    if (status != BUFSIZ-pivHandle->outbytesleft)
    {
        return -1;
    }
    return 0;
}
