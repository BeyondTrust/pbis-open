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
