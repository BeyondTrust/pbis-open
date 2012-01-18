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
 *        regiconv.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser file I/O utf-16/utf-8 decoding layer header file
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#ifndef REGICONV_H
#define REGICONV_H


#define REGICONV_ENCODING_TO   "utf-8"
#define REGICONV_ENCODING_FROM "ucs-2le"
#define REGICONV_ENCODING_UTF8 REGICONV_ENCODING_TO
#define REGICONV_ENCODING_UCS2 REGICONV_ENCODING_FROM

typedef struct _IV_CONVERT_CTX  IV_CONVERT_CTX, *PIV_CONVERT_CTX;

int RegIconvConvertOpen(
    PIV_CONVERT_CTX *ppivHandle,
    char *ivToCode,
    char *ivFromCode);

void RegIconvConvertClose(
    PIV_CONVERT_CTX pivHandle);

int RegIconvConvertBuffer(
    PIV_CONVERT_CTX pivHandle,
    PBYTE pszInBuf,
    SSIZE_T inBufLen,
    PCHAR pszOutBuf,
    SSIZE_T *pInBufUsed,
    SSIZE_T *pOutBufLen);

int RegIconvConvertReadBuf(
    PIV_CONVERT_CTX pivHandle,
    FILE *fp,
    char **pszOutBuf,
    ssize_t *pOutBufLen);

int RegIconvConvertWriteBuf(
    PIV_CONVERT_CTX pivHandle,
    FILE *fp);

int RegIconvConvertGetWriteBuf(
    PIV_CONVERT_CTX pivHandle,
    char **pszOutBuf,
    ssize_t *pOutBufLen);

#endif
