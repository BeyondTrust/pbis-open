/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "../includes.h"


DWORD
LWSetCapacity(
    DynamicArray *array,
    size_t itemSize,
    size_t capacity
    )
{
    DWORD dwError = 0;

    /* Resize the array */
    dwError = LwReallocMemory(array->data, &array->data, capacity * itemSize);
    BAIL_ON_MAC_ERROR(dwError);
    
    array->capacity = capacity;
    if (array->size > capacity)
    {
        array->size = capacity;
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWArrayInsert(
    DynamicArray *array,
    int insertPos,
    int itemSize,
    const void *data,
    size_t dataLen
    )
{
    DWORD dwError = 0;

    if (array->size + dataLen > array->capacity)
    {
        /* Resize the array */
        dwError = LWSetCapacity(array, itemSize, array->capacity + dataLen + array->capacity);
        BAIL_ON_MAC_ERROR(dwError);
    }
    
    /* Make room for the new value */
    memmove((char *)array->data + (insertPos + dataLen)*itemSize,
            (char *)array->data + insertPos*itemSize,
            (array->size - insertPos)*itemSize);
    
    memcpy((char *)array->data + insertPos*itemSize, data, dataLen*itemSize);
    
    array->size += dataLen;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWArrayAppend(
    DynamicArray* array,
    int           itemSize,
    const void *  data,
    size_t        dataLen
    )
{
    return LWArrayInsert(
                array,
                array->size,
                itemSize,
                data,
                dataLen);
}

static
void
LWFreeNVPair(
    PNVPAIR pNVPair
    )
{
    LW_SAFE_FREE_STRING(pNVPair->pszName);
    LW_SAFE_FREE_STRING(pNVPair->pszValue);

    LwFreeMemory(pNVPair);
}

static
void
LWFreeNVPairList(
    PNVPAIR pNVPairList
    )
{
    PNVPAIR pNVPair = NULL;

    while (pNVPairList) {
        pNVPair = pNVPairList;
        pNVPairList = pNVPairList->pNext;
        LWFreeNVPair(pNVPair);
    }
}

static
void
LWFreeSection(
    PCFGSECTION pSection
    )
{
    LW_SAFE_FREE_STRING(pSection->pszName);

    if (pSection->pNVPairList)
        LWFreeNVPairList(pSection->pNVPairList);
    LwFreeMemory(pSection);
}

void
LWFreeConfigSectionList(
    PCFGSECTION pSectionList
    )
{
    PCFGSECTION pSection = NULL;
    while (pSectionList) {
        pSection = pSectionList;
        pSectionList = pSectionList->pNext;
        LWFreeSection(pSection);
    }
}

static
VOID
ArrayFree(
    DynamicArray* array
    )
{
    if (array->data)
    {
        LwFreeMemory(array->data);
    }
    
    array->size = 0;
    array->capacity = 0;
}

static
DWORD
ReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    )
{
    DWORD dwError = 0;
    DynamicArray buffer;
    const char nullTerm = '\0';

    *pbEndOfFile = 0;
    *output = NULL;
    memset(&buffer, 0, sizeof(buffer));
    dwError = LWSetCapacity(&buffer, 1, 100);
    BAIL_ON_MAC_ERROR(dwError);

    while(1)
    {
        if(fgets((char*)buffer.data + buffer.size,
                buffer.capacity - buffer.size, fp) ==  NULL)
        {
            if (feof(fp)) {
                *pbEndOfFile = 1;
            } else {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }
        }
        buffer.size += strlen((char*)buffer.data + buffer.size);

        if(*pbEndOfFile)
            break;
        if(buffer.size == buffer.capacity - 1 &&
                ((char *)buffer.data)[buffer.size - 1] != '\n')
        {
            dwError = LWSetCapacity(&buffer, 1, buffer.capacity * 2);
            BAIL_ON_MAC_ERROR(dwError);
        }
        else
            break;
    }

    dwError = LWArrayAppend(&buffer, 1, &nullTerm, 1);
    BAIL_ON_MAC_ERROR(dwError);
    
    *output = (PSTR)buffer.data;
    buffer.data = NULL;

cleanup:

    ArrayFree(&buffer);
    
    return dwError;
    
error:

    goto cleanup;
}

static
PNVPAIR
ReverseNVPairList(
    PNVPAIR pNVPairList
    )
{
    PNVPAIR pP = NULL;
    PNVPAIR pQ = pNVPairList;
    PNVPAIR pR = NULL;

    while (pQ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

static
PCFGSECTION
ReverseSectionsAndNVPairs(
    PCFGSECTION pSectionList
    )
{
    PCFGSECTION pP = NULL;
    PCFGSECTION pQ = pSectionList;
    PCFGSECTION pR = NULL;

    while (pQ) {

        if (pQ->pNVPairList) {
            pQ->pNVPairList = ReverseNVPairList(pQ->pNVPairList);
        }

        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

static
DWORD
ParseHeader(
    FILE* fp,
    PDWORD pdwSignature)
{
    DWORD dwError = 0;
    uint16_t wSignature = 0;

    if (1 != fread(&wSignature, sizeof(wSignature), 1, fp)) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *pdwSignature = CONVERT_ENDIAN_WORD(wSignature);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}


static
DWORD
ReadNextDoubleByteLine(
    FILE* fp,
    PSTR pszBuf,
    DWORD dwMaxLen,
    PBOOLEAN pbEndOfFile
    )
{
    DWORD dwError = 0;
    DBLBYTE dbBuf;
    PSTR pszTmp = pszBuf;
    DWORD iBuf = 0;

    *pbEndOfFile = 0;

    while ((iBuf < dwMaxLen)) {

        if (1 != fread(&dbBuf, sizeof(dbBuf), 1, fp)) {

            if (feof(fp)) {

                *pbEndOfFile = 1;
                break;

            } else {

                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);

            }

        }

        if ((*pszTmp++ = dbBuf.b1) == '\n')
            break;

        iBuf++;
    }

    if (iBuf == dwMaxLen) {
        dwError = MAC_AD_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    *pszTmp = '\0';

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWParseConfigFile(
    PCSTR pszFilePath,
    PCFGSECTION* ppCfgSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSectionList = NULL;
    PCFGSECTION pSection = NULL;
    PNVPAIR pNVPair = NULL;
    FILE* fp = NULL;
    CHAR staticBuffer[1024+1];
    PSTR szBuf = NULL;
    DWORD dwLen = 0;
    PSTR pszTmp = NULL;
    PSTR pszName = NULL;
    PSTR pszValue = NULL;
    DWORD dwSignature = 0;
    /*DWORD nRead = 0;*/
    BOOLEAN bEOF = FALSE;

    if ((fp = fopen(pszFilePath, "r")) == NULL) {
        dwError = errno;
        goto error;
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (bWindowsDoubleByteFormat) {
        dwError = ParseHeader(fp, &dwSignature);
        BAIL_ON_MAC_ERROR(dwError);

        if (dwSignature != 0xFEFF) {
            dwError = MAC_AD_ERROR_INVALID_TAG;
            BAIL_ON_MAC_ERROR(dwError);
        }
    }

    while (!bEOF) {
        LW_SAFE_FREE_STRING(szBuf);

        if (bWindowsDoubleByteFormat) {

            staticBuffer[0] = '\0';
            dwError = ReadNextDoubleByteLine(fp, staticBuffer, 1024, &bEOF);
            BAIL_ON_MAC_ERROR(dwError);
            dwError = LwAllocateString(staticBuffer, &szBuf);
            BAIL_ON_MAC_ERROR(dwError);

        } else {

            dwError = ReadNextLine(fp, &szBuf, &bEOF);
            BAIL_ON_MAC_ERROR(dwError);

        }

        LwStripWhitespace(szBuf, TRUE, TRUE);

        if (!(dwLen=strlen(szBuf)))
            continue;

        /* Skip comments for now */
        if (szBuf[0] == '#' ||
            szBuf[0] == ';')
            continue;

        if (szBuf[0]       == '[' &&
            szBuf[dwLen-1] == ']') {

            if (pSection) {
                pSection->pNext = pSectionList;
                pSectionList = pSection;
                pSection = NULL;
            }

            dwError = LwAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
            BAIL_ON_MAC_ERROR(dwError);

            szBuf[dwLen-1] = '\0';

            dwError = LwAllocateString(szBuf+1, &pSection->pszName);
            BAIL_ON_MAC_ERROR(dwError);

            LwStripWhitespace(pSection->pszName, TRUE, TRUE);

        } else {

            if (!pSection) {
                dwError = MAC_AD_ERROR_NO_SUCH_ATTRIBUTE;
                BAIL_ON_MAC_ERROR(dwError);
            }

            if ((pszTmp = strchr(szBuf, '=')) == NULL) {
                continue;
            }

            if (pszTmp == szBuf) {
                dwError = MAC_AD_ERROR_INVALID_TAG;
                BAIL_ON_MAC_ERROR(dwError);
            }

            dwError = LwAllocateMemory(pszTmp-szBuf+1, (PVOID*)&pszName);
            BAIL_ON_MAC_ERROR(dwError);

            strncpy(pszName, szBuf, pszTmp-szBuf);

            pszTmp++;
            while (*pszTmp != '\0' && isspace((int)*pszTmp))
                pszTmp++;

            if (*pszTmp != '\0') {
                dwError = LwAllocateString(pszTmp, &pszValue);
                BAIL_ON_MAC_ERROR(dwError);
            }

            dwError = LwAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
            BAIL_ON_MAC_ERROR(dwError);

            LwStripWhitespace(pszName, TRUE, TRUE);
            LwStripWhitespace(pszValue, TRUE, TRUE);

            pNVPair->pszName = pszName; pszName = NULL;
            pNVPair->pszValue = pszValue; pszValue = NULL;

            pNVPair->pNext = pSection->pNVPairList;
            pSection->pNVPairList = pNVPair;
            pNVPair = NULL;

        }
    }

    if (pSection) {
        pSection->pNext = pSectionList;
        pSectionList = pSection;
        pSection = NULL;
    }

    pSectionList = ReverseSectionsAndNVPairs(pSectionList);

    *ppCfgSectionList = pSectionList;

    fclose(fp); fp = NULL;

cleanup:

    LW_SAFE_FREE_STRING(szBuf);

    if (fp) {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszName);
    LW_SAFE_FREE_STRING(pszValue);

    return dwError;
    
error:

    *ppCfgSectionList = NULL;

    if (pSectionList)
    {
       LWFreeConfigSectionList(pSectionList);
    }

    if (pSection)
    {
        LWFreeSection(pSection);
    }

    if (pNVPair)
    {
        LWFreeNVPair(pNVPair);
    }

    goto cleanup;
}

DWORD
LWSaveConfigSectionListToFile(
    FILE* fp,
    PCFGSECTION pSectionList
    )
{
    DWORD dwError = 0;
    PNVPAIR pNVPair = NULL;

    while (pSectionList) {

        if (IsNullOrEmptyString(pSectionList->pszName)) {
            dwError = MAC_AD_ERROR_INVALID_RECORD_TYPE;
            goto error;
        }

        if (fprintf(fp, "[%s]\r\n", pSectionList->pszName) < 0) {
            dwError = errno;
            BAIL_ON_MAC_ERROR(dwError);
        }

        pNVPair = pSectionList->pNVPairList;
        while (pNVPair) {

            if (IsNullOrEmptyString(pNVPair->pszName)) {
                dwError = MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE;
                BAIL_ON_MAC_ERROR(dwError);
            }

            if (fprintf(fp, "    %s = %s\r\n",
                        pNVPair->pszName,
                        (IsNullOrEmptyString(pNVPair->pszValue) ? "" : pNVPair->pszValue)) < 0) {
                dwError = errno;
                BAIL_ON_MAC_ERROR(dwError);
            }

            pNVPair = pNVPair->pNext;
        }

        pSectionList = pSectionList->pNext;

    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWSaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PCFGSECTION pSectionList
    )
{
    DWORD dwError = 0;
    /*PNVPAIR pNVPair = NULL;*/
    FILE* fp = NULL;
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;

    dwError = LwAllocateMemory(strlen(pszConfigFilePath)+strlen(".macadutil")+1,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_MAC_ERROR(dwError);

    sprintf(pszTmpPath, "%s.macadutil", pszConfigFilePath);

    if ((fp = fopen(pszTmpPath, "w")) == NULL) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        dwError = errno;
        BAIL_ON_MAC_ERROR(dwError);
    }

    bRemoveFile = TRUE;

    dwError = LWSaveConfigSectionListToFile(fp, pSectionList);
    BAIL_ON_MAC_ERROR(dwError);

    fclose(fp); fp = NULL;

    dwError = LwMoveFile(pszTmpPath, pszConfigFilePath);
    BAIL_ON_MAC_ERROR(dwError);
    
    bRemoveFile = FALSE;

cleanup:

    if (bRemoveFile)
    {
        LwRemoveFile(pszTmpPath);
    }

    if (fp)
    {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszTmpPath);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWCreateConfigSection(
    PCFGSECTION* ppSectionList,
    PCFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSectionList = NULL;
    PCFGSECTION pSection = NULL;
    PCFGSECTION pTmpSection = NULL;

    if (!ppSectionList || IsNullOrEmptyString(pszSectionName)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    pSectionList = *ppSectionList;

    pSection = pSectionList;
    while (pSection)
    {
        if (!strcmp(pSection->pszName, pszSectionName))
        {
            break;
        }
    }

    if (!pSection) {
        dwError = LwAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LwAllocateString(pszSectionName, &pSection->pszName);
        BAIL_ON_MAC_ERROR(dwError);

        if (!pSectionList) {

            pSectionList = pSection;

        } else {

            pTmpSection = pSectionList;
            while (pTmpSection->pNext)
                pTmpSection = pTmpSection->pNext;

            pTmpSection->pNext = pSection;

        }
    }

    *pCreatedSection = pSection;
    *ppSectionList = pSectionList;

cleanup:

    return dwError;
    
error:

    *pCreatedSection = NULL;

    if (pSection)
    {
        LWFreeSection(pSection);
    }
    
    goto cleanup;
}

DWORD
LWGetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    DWORD dwError = 0;
    PNVPAIR pNVPair = NULL;
    
    *ppszValue = NULL;

    pNVPair = pSection->pNVPairList;
    while (pNVPair) {
        if (!strcmp(pNVPair->pszName, pszName))
            break;

        pNVPair = pNVPair->pNext;
    }

    if (!pNVPair) {
        dwError = MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE;
        goto cleanup; /* Not uncommon, no need to log error. */
    }

    if (pNVPair->pszValue) {

        dwError = LwAllocateString(pNVPair->pszValue, ppszValue);
        BAIL_ON_MAC_ERROR(dwError);

    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSection = NULL;

    while (pSectionList) {
        if (!strcmp(pSectionList->pszName, pszSectionName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        dwError = MAC_AD_ERROR_INVALID_RECORD_TYPE;
        goto error;
    }

    dwError = LWGetConfigValueBySection(pSection,
                                        pszName,
                                        ppszValue);
    goto cleanup; /* Not uncommon, no need to log error. */

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWSetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    PNVPAIR pNVPair = NULL;
    PNVPAIR pTmpNVPair = NULL;

    pTmpNVPair = pSection->pNVPairList;
    while (pTmpNVPair) {
        if (!strcmp(pTmpNVPair->pszName, pszName)) {
            pNVPair = pTmpNVPair;
            break;
        }
        pTmpNVPair = pTmpNVPair->pNext;
    }

    if (pNVPair == NULL) {

        dwError = LwAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
        BAIL_ON_MAC_ERROR(dwError);

        dwError = LwAllocateString(pszName, &pNVPair->pszName);
        BAIL_ON_MAC_ERROR(dwError);

        LwStripWhitespace(pNVPair->pszName, TRUE, TRUE);

        if (!IsNullOrEmptyString(pszValue)) {
            dwError = LwAllocateString(pszValue, &pNVPair->pszValue);
            BAIL_ON_MAC_ERROR(dwError);
        }

        if (!pSection->pNVPairList) {
            pSection->pNVPairList = pNVPair;
            pNVPair = NULL;

        } else {

            pTmpNVPair = pSection->pNVPairList;
            while (pTmpNVPair->pNext != NULL)
                pTmpNVPair = pTmpNVPair->pNext;

            pTmpNVPair->pNext = pNVPair;
            pNVPair = NULL;

        }

    } else {

        if (pNVPair->pszValue) {

            LW_SAFE_FREE_STRING(pNVPair->pszValue);
            pNVPair->pszValue = NULL;

            if (!IsNullOrEmptyString(pszValue)) {
                dwError = LwAllocateString(pszValue, &pNVPair->pszValue);
                BAIL_ON_MAC_ERROR(dwError);
            }

        }

    }

cleanup:

    return dwError;
    
error:

    if (pNVPair) {
        LWFreeNVPair(pNVPair);
    }

    goto cleanup;
}

DWORD
LWSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSection = NULL;

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (pSectionList) {
        if (!strcmp(pSectionList->pszName, pszSectionName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        dwError = MAC_AD_ERROR_INVALID_RECORD_TYPE;
        goto error;
    }

    dwError = LWSetConfigValueBySection(pSection,
                                        pszName,
                                        pszValue);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWDeleteConfigSection(
    PCFGSECTION* ppSectionList,
    PCSTR pszSectionName
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSection = NULL;
    PCFGSECTION pIterSection = *ppSectionList;
    PCFGSECTION pPrevSection = NULL;

    if (IsNullOrEmptyString(pszSectionName)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (pIterSection) {
        if (!strcmp(pszSectionName, pIterSection->pszName)) {
            pSection = pIterSection;
            break;
        }
        pPrevSection = pIterSection;
        pIterSection = pIterSection->pNext;
    }

    if (pSection) {

        if (!pPrevSection) {

            *ppSectionList = pSection->pNext;

        } else {

            pPrevSection->pNext = pSection->pNext;

        }

        LWFreeSection(pSection);
    }

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWDeleteNameValuePairBySection(
    PCFGSECTION pSection,
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    PNVPAIR pNVPair = NULL;
    PNVPAIR pPrevNVPair = NULL;

    pNVPair = pSection->pNVPairList;
    while (pNVPair) {
        if (!strcmp(pNVPair->pszName, pszName)) {
            break;
        }
        pPrevNVPair = pNVPair;
        pNVPair = pNVPair->pNext;
    }

    if (pNVPair != NULL) {

        if (pPrevNVPair) {
            pPrevNVPair->pNext = pNVPair->pNext;
        } else {
            pSection->pNVPairList = pNVPair->pNext;
        }

        LWFreeNVPair(pNVPair);
    }

    return dwError;
}

DWORD
LWDeleteNameValuePairBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    PCFGSECTION pSection = NULL;

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        dwError = MAC_AD_ERROR_INVALID_PARAMETER;
        BAIL_ON_MAC_ERROR(dwError);
    }

    while (pSectionList) {
        if (!strcmp(pszSectionName, pszName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        dwError = MAC_AD_ERROR_INVALID_RECORD_TYPE;
        goto error;
    }

    dwError = LWDeleteNameValuePairBySection(pSection, pszName);
    BAIL_ON_MAC_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

