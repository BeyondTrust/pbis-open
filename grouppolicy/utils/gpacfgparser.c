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

#include "includes.h"

static
void
GPAFreeNVPair(
    PGPANVPAIR pNVPair
    )
{
    if (pNVPair->pszName)
        LwFreeString(pNVPair->pszName);

    if (pNVPair->pszValue)
        LwFreeString(pNVPair->pszValue);

    LwFreeMemory(pNVPair);
}

static
void
GPAFreeNVPairList(
    PGPANVPAIR pNVPairList
    )
{
    PGPANVPAIR pNVPair = NULL;

    while (pNVPairList) {
        pNVPair = pNVPairList;
        pNVPairList = pNVPairList->pNext;
        GPAFreeNVPair(pNVPair);
    }

}

static
void
GPAFreeSection(
    PGPACFGSECTION pSection
    )
{
    if (pSection->pszName)
        LwFreeString(pSection->pszName);
    if (pSection->pNVPairList)
        GPAFreeNVPairList(pSection->pNVPairList);
    LwFreeMemory(pSection);
}

static
PGPANVPAIR
ReverseNVPairList(
    PGPANVPAIR pNVPairList
    )
{
    PGPANVPAIR pP = NULL;
    PGPANVPAIR pQ = pNVPairList;
    PGPANVPAIR pR = NULL;

    while (pQ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

static
PGPACFGSECTION
ReverseSectionsAndNVPairs(
    PGPACFGSECTION pSectionList
    )
{
    PGPACFGSECTION pP = NULL;
    PGPACFGSECTION pQ = pSectionList;
    PGPACFGSECTION pR = NULL;

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

void
GPAFreeConfigSectionList(
    PGPACFGSECTION pSectionList
    )
{
    PGPACFGSECTION pSection = NULL;
    while (pSectionList) {
        pSection = pSectionList;
        pSectionList = pSectionList->pNext;
        GPAFreeSection(pSection);
    }
}

static
CENTERROR
ParseHeader(
    FILE* fp,
    PDWORD pdwSignature)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uint16_t wSignature = 0;

    if (1 != fread(&wSignature, sizeof(wSignature), 1, fp)) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    *pdwSignature = GPA_CONVERT_ENDIAN_WORD(wSignature);

error:

    return ceError;
}

static
CENTERROR
ReadNextDoubleByteLine(
    FILE* fp,
    PSTR pszBuf,
    DWORD dwMaxLen,
    PBOOLEAN pbEndOfFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPADBLBYTE dbBuf;
    PSTR pszTmp = pszBuf;
    DWORD iBuf = 0;

    *pbEndOfFile = 0;

    while ((iBuf < dwMaxLen)) {

        if (1 != fread(&dbBuf, sizeof(dbBuf), 1, fp)) {

            if (feof(fp)) {

                *pbEndOfFile = 1;
                break;

            } else {

                ceError = LwMapErrnoToLwError(ceError);
                BAIL_ON_GPA_ERROR(ceError);

            }

        }

        if ((*pszTmp++ = dbBuf.b1) == '\n')
            break;

        iBuf++;
    }

    if (iBuf == dwMaxLen) {
        ceError = CENTERROR_CFG_NOT_ENOUGH_BUFFER;
        BAIL_ON_GPA_ERROR(ceError);
    }

    *pszTmp = '\0';

error:

    return ceError;
}

CENTERROR
GPAReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPADynamicArray buffer;
    const char nullTerm = '\0';

    *pbEndOfFile = 0;
    *output = NULL;
    memset(&buffer, 0, sizeof(buffer));

    ceError = GPASetCapacity(&buffer, 1, 100);
    BAIL_ON_GPA_ERROR(ceError);

    while(1)
    {
        if(fgets(buffer.data + buffer.size,
                buffer.capacity - buffer.size, fp) ==  NULL)
        {
            if (feof(fp)) {
                *pbEndOfFile = 1;
            } else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_GPA_ERROR(ceError);
            }
        }
        buffer.size += strlen(buffer.data + buffer.size);

        if(*pbEndOfFile)
            break;
        if(buffer.size == buffer.capacity - 1 &&
                ((char *)buffer.data)[buffer.size - 1] != '\n')
        {
            ceError = GPASetCapacity(&buffer, 1, buffer.capacity * 2);
            BAIL_ON_GPA_ERROR(ceError);
        }
        else
            break;
    }

    ceError = GPAArrayAppend(&buffer, 1, &nullTerm, 1);
    BAIL_ON_GPA_ERROR(ceError);

    *output = buffer.data;
    buffer.data = NULL;

error:
    GPAArrayFree(&buffer);
    return ceError;
}

CENTERROR
GPAParseConfigFile(
    PCSTR pszFilePath,
    PGPACFGSECTION* ppCfgSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSectionList = NULL;
    PGPACFGSECTION pSection = NULL;
    PGPANVPAIR pNVPair = NULL;
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
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (bWindowsDoubleByteFormat) {
        ceError = ParseHeader(fp, &dwSignature);
        BAIL_ON_GPA_ERROR(ceError);

        if (dwSignature != 0xFEFF) {
            ceError = CENTERROR_CFG_INVALID_SIGNATURE;
            BAIL_ON_GPA_ERROR(ceError);
        }
    }

    while (!bEOF) {
        LW_SAFE_FREE_STRING(szBuf);

        if (bWindowsDoubleByteFormat) {

            staticBuffer[0] = '\0';
            ceError = ReadNextDoubleByteLine(fp, staticBuffer, 1024, &bEOF);
            BAIL_ON_GPA_ERROR(ceError);

            //ceError = CTStrdup(staticBuffer, &szBuf);
            ceError = LwAllocateString(staticBuffer, &szBuf);
            BAIL_ON_GPA_ERROR(ceError);

        } else {

            ceError = GPAReadNextLine(fp, &szBuf, &bEOF);
            BAIL_ON_GPA_ERROR(ceError);

        }

        LwStripWhitespace(szBuf,1,1);

        if (!(dwLen=strlen(szBuf)))
            continue;

        /* Skip comments for now */
        if (szBuf[0] == ';')
            continue;

        if ((szBuf[0] == '['       &&
            szBuf[dwLen-1] == ']') ||
            (szBuf[0] == '#' && (!pSection || pSection->pszName[0] == '#'))) {

            if (pSection) {
                pSection->pNext = pSectionList;
                pSectionList = pSection;
                pSection = NULL;
            }

            ceError = LwAllocateMemory(sizeof(GPACFGSECTION), (PVOID*)&pSection);
            BAIL_ON_GPA_ERROR(ceError);

            if (szBuf[0] == '#') {
                szBuf[dwLen] = '\0'; 
                ceError = LwAllocateString(szBuf, &pSection->pszName);
            } else {
                szBuf[dwLen-1] = '\0'; 
                ceError = LwAllocateString(szBuf+1, &pSection->pszName);
            }
            BAIL_ON_GPA_ERROR(ceError);

            LwStripWhitespace(pSection->pszName,1,1);

        } else {
            if (szBuf[0] == '#' && pSection) {
                szBuf[dwLen] = '\0';

                ceError = LwAllocateString(szBuf, &pszName);
                BAIL_ON_GPA_ERROR(ceError);

                strncpy(pszName, szBuf, strlen(szBuf));
            } else {
                if (!pSection) {
                    ceError = CENTERROR_VALUE_NOT_IN_SECTION;
                    BAIL_ON_GPA_ERROR(ceError);
                }

                if ((pszTmp = strchr(szBuf, '=')) == NULL) {
                    ceError = CENTERROR_BAD_TOKEN;
                    BAIL_ON_GPA_ERROR(ceError);
                }

                if (pszTmp == szBuf) {
                    ceError = CENTERROR_BAD_TOKEN;
                    BAIL_ON_GPA_ERROR(ceError);
                }

                ceError = LwAllocateMemory(pszTmp-szBuf+1, (PVOID*)&pszName);
                BAIL_ON_GPA_ERROR(ceError);

                strncpy(pszName, szBuf, pszTmp-szBuf);

                pszTmp++;
                while (*pszTmp != '\0' && isspace((int)*pszTmp))
                    pszTmp++;

                if (*pszTmp != '\0') {
                    ceError = LwAllocateString(pszTmp, &pszValue);
                    BAIL_ON_GPA_ERROR(ceError);
                }
            }

            ceError = LwAllocateMemory(sizeof(GPANVPAIR), (PVOID*)&pNVPair);
            BAIL_ON_GPA_ERROR(ceError);

            LwStripWhitespace(pszName,1,1);
            LwStripWhitespace(pszValue,1,1);

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
    pSectionList = NULL;

    fclose(fp); fp = NULL;

error:

    LW_SAFE_FREE_STRING(szBuf);

    if (fp) {
        fclose(fp);
    }

    if (pszName)
        LwFreeString(pszName);

    if (pszValue)
        LwFreeString(pszValue);

    if (pNVPair)
        GPAFreeNVPair(pNVPair);

    if (pSection)
        GPAFreeSection(pSection);

    if (pSectionList)
        GPAFreeConfigSectionList(pSectionList);

    return ceError;
}

CENTERROR
GPASaveConfigSectionListToFile(
    FILE* fp,
    PGPACFGSECTION pSectionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPANVPAIR pNVPair = NULL;

    while (pSectionList) {

        if (IsNullOrEmptyString(pSectionList->pszName)) {
            ceError = CENTERROR_CFG_INVALID_SECTION_NAME;
            BAIL_ON_GPA_ERROR(ceError);
        }

        if (pSectionList->pszName[0] == '#') {
            ceError = GPAFilePrintf(fp, "%s\n", pSectionList->pszName);
            BAIL_ON_GPA_ERROR(ceError);
        } else {        
            ceError = GPAFilePrintf(fp, "[%s]\n", pSectionList->pszName);
            BAIL_ON_GPA_ERROR(ceError);

            pNVPair = pSectionList->pNVPairList;
            while (pNVPair) {

                if (IsNullOrEmptyString(pNVPair->pszName)) {
                    ceError = CENTERROR_CFG_INVALID_NVPAIR_NAME;
                    BAIL_ON_GPA_ERROR(ceError);
                }

                if (pNVPair->pszName[0] == '#') {
                    GPAFilePrintf( fp, "    %s\n",
                                  pNVPair->pszName);
                    BAIL_ON_GPA_ERROR(ceError);
                } else {
                    GPAFilePrintf( fp, "    %s = %s\n",
                                  pNVPair->pszName,
                                  (IsNullOrEmptyString(pNVPair->pszValue) ? "" : pNVPair->pszValue));
                    BAIL_ON_GPA_ERROR(ceError);
                }

                pNVPair = pNVPair->pNext;
            }
        }

        pSectionList = pSectionList->pNext;

    }

error:

    return ceError;
}

CENTERROR
GPASaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PGPACFGSECTION pSectionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*PGPANVPAIR pNVPair = NULL;*/
    FILE* fp = NULL;
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;

    ceError = LwAllocateMemory(strlen(pszConfigFilePath)+strlen(".gpagent")+1,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_GPA_ERROR(ceError);

    sprintf(pszTmpPath, "%s.gpagent", pszConfigFilePath);

    if ((fp = fopen(pszTmpPath, "w")) == NULL) {
        ceError = LwMapErrnoToLwError(ceError);
        BAIL_ON_GPA_ERROR(ceError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_GPA_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    ceError = GPASaveConfigSectionListToFile(fp, pSectionList);
    BAIL_ON_GPA_ERROR(ceError);

    fclose(fp); fp = NULL;

    ceError = LwMoveFile(pszTmpPath, pszConfigFilePath);
    BAIL_ON_GPA_ERROR(ceError);

    if (pszTmpPath)
        LwFreeString(pszTmpPath);

    return ceError;

error:

    if (bRemoveFile)
        LwRemoveFile(pszTmpPath);

    if (fp)
        fclose(fp);

    if (pszTmpPath)
        LwFreeString(pszTmpPath);

    return ceError;
}

CENTERROR
GPACreateConfigSection(
    PGPACFGSECTION* ppSectionList,
    PGPACFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSectionList = NULL;
    PGPACFGSECTION pSection = NULL;
    PGPACFGSECTION pTmpSection = NULL;

    if (!ppSectionList || IsNullOrEmptyString(pszSectionName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_GPA_ERROR(ceError);
    }

    pSectionList = *ppSectionList;

    pSection = pSectionList;
    while (pSection)
    {
        if (!strcmp(pSection->pszName, pszSectionName))
            break;

        pSection = pSection->pNext;
    }

    if (!pSection)
    {
        ceError = LwAllocateMemory(sizeof(GPACFGSECTION), (PVOID*)&pSection);
        BAIL_ON_GPA_ERROR(ceError);

        ceError = LwAllocateString(pszSectionName, &pSection->pszName);
        BAIL_ON_GPA_ERROR(ceError);

        if (!pSectionList)
        {
            pSectionList = pSection;
        }
        else
        {
            pTmpSection = pSectionList;
            while (pTmpSection->pNext)
                pTmpSection = pTmpSection->pNext;

            pTmpSection->pNext = pSection;
        }
    }

    *pCreatedSection = pSection;
    pSection = NULL;

    *ppSectionList = pSectionList;

    return ceError;

error:

    if (pSection)
        GPAFreeSection(pSection);

    *pCreatedSection = NULL;

    return ceError;
}

CENTERROR
GPAGetConfigValueBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSection = NULL;

    while (pSectionList) {
        if (!strcmp(pSectionList->pszName, pszSectionName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        ceError = CENTERROR_CFG_SECTION_NOT_FOUND;
        BAIL_ON_GPA_ERROR(ceError);
    }

    ceError = GPAGetConfigValueBySection(pSection,
                                        pszName,
                                        ppszValue);
    goto error; /* Not uncommon, no need to log error. */

error:

    return ceError;
}

CENTERROR
GPASetConfigValueBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSection = NULL;
    PGPANVPAIR pNVPair = NULL;
    /*PGPANVPAIR pTmpNVPair = NULL;*/

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_GPA_ERROR(ceError);
    }

    pSection = pSectionList;

    while (pSection)
    {
        if (!strcmp(pSection->pszName, pszSectionName))
        {
            break;
        }
        pSection = pSection->pNext;
    }

    if (!pSection)
    {
        ceError = GPACreateConfigSection(&pSectionList, &pSection, pszSectionName);
        BAIL_ON_GPA_ERROR(ceError);
    }

    ceError = GPASetConfigValueBySection(pSection,
                                        pszName,
                                        pszValue);
    BAIL_ON_GPA_ERROR(ceError);

    return ceError;

error:

    if (pNVPair)
    {
        GPAFreeNVPair(pNVPair);
    }

    return ceError;
}

CENTERROR
GPADeleteConfigSection(
    PGPACFGSECTION* ppSectionList,
    PCSTR pszSectionName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSection = NULL;
    PGPACFGSECTION pIterSection = *ppSectionList;
    PGPACFGSECTION pPrevSection = NULL;

    if (IsNullOrEmptyString(pszSectionName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_GPA_ERROR(ceError);
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

        GPAFreeSection(pSection);
    }

error:

    return ceError;
}


CENTERROR
GPADeleteNameValuePairBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPANVPAIR pNVPair = NULL;
    PGPANVPAIR pPrevNVPair = NULL;

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

        GPAFreeNVPair(pNVPair);
    }

/*error:*/

    return ceError;
}

CENTERROR
GPADeleteNameValuePairBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pSection = NULL;

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_GPA_ERROR(ceError);
    }

    while (pSectionList) {
        if (!strcmp(pszSectionName, pszName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        ceError = CENTERROR_CFG_SECTION_NOT_FOUND;
        BAIL_ON_GPA_ERROR(ceError);
    }

    ceError = GPADeleteNameValuePairBySection(pSection, pszName);
    BAIL_ON_GPA_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
GPAGetConfigValueBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPANVPAIR pNVPair = NULL;

    pNVPair = pSection->pNVPairList;
    while (pNVPair) {
        if (!strcmp(pNVPair->pszName, pszName))
            break;

        pNVPair = pNVPair->pNext;
    }

    if (!pNVPair) {
        ceError = CENTERROR_CFG_VALUE_NOT_FOUND;
        goto error; /* Not uncommon, no need to log error. */
    }

    if (pNVPair->pszValue) {

        ceError = LwAllocateString(pNVPair->pszValue, ppszValue);
        BAIL_ON_GPA_ERROR(ceError);

    } else {

        *ppszValue = NULL;

    }

error:

    return ceError;
}

CENTERROR
GPASetConfigValueBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPANVPAIR pNVPair = NULL;
    PGPANVPAIR pTmpNVPair = NULL;

    pTmpNVPair = pSection->pNVPairList;
    while (pTmpNVPair) {
        if (!strcmp(pTmpNVPair->pszName, pszName)) {
            pNVPair = pTmpNVPair;
            break;
        }
        pTmpNVPair = pTmpNVPair->pNext;
    }

    if (pNVPair == NULL) {

        ceError = LwAllocateMemory(sizeof(GPANVPAIR), (PVOID*)&pNVPair);
        BAIL_ON_GPA_ERROR(ceError);

        ceError = LwAllocateString(pszName, &pNVPair->pszName);
        BAIL_ON_GPA_ERROR(ceError);

        LwStripWhitespace(pNVPair->pszName,1,1);

        if (!IsNullOrEmptyString(pszValue)) {
            ceError = LwAllocateString(pszValue, &pNVPair->pszValue);
            BAIL_ON_GPA_ERROR(ceError);
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

            LwFreeString(pNVPair->pszValue);
            pNVPair->pszValue = NULL;

            if (!IsNullOrEmptyString(pszValue)) {
                ceError = LwAllocateString(pszValue, &pNVPair->pszValue);
                BAIL_ON_GPA_ERROR(ceError);
            }

        }

    }

    return ceError;

error:

    if (pNVPair) {
        GPAFreeNVPair(pNVPair);
    }

    return ceError;
}
