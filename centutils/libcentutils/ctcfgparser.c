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

static
void
CTFreeNVPair(
    PNVPAIR pNVPair
    )
{
    if (pNVPair->pszName)
        CTFreeString(pNVPair->pszName);

    if (pNVPair->pszValue)
        CTFreeString(pNVPair->pszValue);

    CTFreeMemory(pNVPair);
}

static
void
CTFreeNVPairList(
    PNVPAIR pNVPairList
    )
{
    PNVPAIR pNVPair = NULL;

    while (pNVPairList) {
        pNVPair = pNVPairList;
        pNVPairList = pNVPairList->pNext;
        CTFreeNVPair(pNVPair);
    }

}

static
void
CTFreeSection(
    PCFGSECTION pSection
    )
{
    if (pSection->pszName)
        CTFreeString(pSection->pszName);
    if (pSection->pNVPairList)
        CTFreeNVPairList(pSection->pNVPairList);
    CTFreeMemory(pSection);
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

void
CTFreeConfigSectionList(
    PCFGSECTION pSectionList
    )
{
    PCFGSECTION pSection = NULL;
    while (pSectionList) {
        pSection = pSectionList;
        pSectionList = pSectionList->pNext;
        CTFreeSection(pSection);
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
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pdwSignature = CONVERT_ENDIAN_WORD(wSignature);

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

                ceError = CTMapSystemError(ceError);
                BAIL_ON_CENTERIS_ERROR(ceError);

            }

        }

        if ((*pszTmp++ = dbBuf.b1) == '\n')
            break;

        iBuf++;
    }

    if (iBuf == dwMaxLen) {
        ceError = CENTERROR_CFG_NOT_ENOUGH_BUFFER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pszTmp = '\0';

error:

    return ceError;
}

CENTERROR
CTParseConfigFile(
    PCSTR pszFilePath,
    PCFGSECTION* ppCfgSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (bWindowsDoubleByteFormat) {
        ceError = ParseHeader(fp, &dwSignature);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (dwSignature != 0xFEFF) {
            ceError = CENTERROR_CFG_INVALID_SIGNATURE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }

    while (!bEOF) {
        CT_SAFE_FREE_STRING(szBuf);

        if (bWindowsDoubleByteFormat) {

            staticBuffer[0] = '\0';
            ceError = ReadNextDoubleByteLine(fp, staticBuffer, 1024, &bEOF);
            BAIL_ON_CENTERIS_ERROR(ceError);
            ceError = CTStrdup(staticBuffer, &szBuf);
            BAIL_ON_CENTERIS_ERROR(ceError);

        } else {

            ceError = CTReadNextLine(fp, &szBuf, &bEOF);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }

        CTStripWhitespace(szBuf);

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

            ceError = CTAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
            BAIL_ON_CENTERIS_ERROR(ceError);

            if (szBuf[0] == '#') {
                szBuf[dwLen] = '\0'; 
                ceError = CTAllocateString(szBuf, &pSection->pszName);
            } else {
                szBuf[dwLen-1] = '\0'; 
                ceError = CTAllocateString(szBuf+1, &pSection->pszName);
            }
            BAIL_ON_CENTERIS_ERROR(ceError);

            CTStripWhitespace(pSection->pszName);

        } else {
            if (szBuf[0] == '#' && pSection) {
                szBuf[dwLen] = '\0';

                ceError = CTAllocateString(szBuf, &pszName);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strncpy(pszName, szBuf, strlen(szBuf));
            } else {
                if (!pSection) {
                    ceError = CENTERROR_VALUE_NOT_IN_SECTION;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                if ((pszTmp = strchr(szBuf, '=')) == NULL) {
                    ceError = CENTERROR_BAD_TOKEN;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                if (pszTmp == szBuf) {
                    ceError = CENTERROR_BAD_TOKEN;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                ceError = CTAllocateMemory(pszTmp-szBuf+1, (PVOID*)&pszName);
                BAIL_ON_CENTERIS_ERROR(ceError);

                strncpy(pszName, szBuf, pszTmp-szBuf);

                pszTmp++;
                while (*pszTmp != '\0' && isspace((int)*pszTmp))
                    pszTmp++;

                if (*pszTmp != '\0') {
                    ceError = CTAllocateString(pszTmp, &pszValue);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }
            }

            ceError = CTAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
            BAIL_ON_CENTERIS_ERROR(ceError);

            CTStripWhitespace(pszName);
            CTStripWhitespace(pszValue);

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

    CT_SAFE_FREE_STRING(szBuf);

    if (fp) {
        fclose(fp);
    }

    if (pszName)
        CTFreeString(pszName);

    if (pszValue)
        CTFreeString(pszValue);

    if (pNVPair)
        CTFreeNVPair(pNVPair);

    if (pSection)
        CTFreeSection(pSection);

    if (pSectionList)
        CTFreeConfigSectionList(pSectionList);

    return ceError;
}

CENTERROR
CTSaveConfigSectionListToFile(
    FILE* fp,
    PCFGSECTION pSectionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PNVPAIR pNVPair = NULL;

    while (pSectionList) {

        if (IsNullOrEmptyString(pSectionList->pszName)) {
            ceError = CENTERROR_CFG_INVALID_SECTION_NAME;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (pSectionList->pszName[0] == '#') {
            ceError = CTFilePrintf(fp, "%s\n", pSectionList->pszName);
            BAIL_ON_CENTERIS_ERROR(ceError);
        } else {        
            ceError = CTFilePrintf(fp, "[%s]\n", pSectionList->pszName);
            BAIL_ON_CENTERIS_ERROR(ceError);

            pNVPair = pSectionList->pNVPairList;
            while (pNVPair) {

                if (IsNullOrEmptyString(pNVPair->pszName)) {
                    ceError = CENTERROR_CFG_INVALID_NVPAIR_NAME;
                    BAIL_ON_CENTERIS_ERROR(ceError);
                }

                if (pNVPair->pszName[0] == '#') {
                    CTFilePrintf( fp, "    %s\n",
                                  pNVPair->pszName);
                    BAIL_ON_CENTERIS_ERROR(ceError);
                } else {
                    CTFilePrintf( fp, "    %s = %s\n",
                                  pNVPair->pszName,
                                  (IsNullOrEmptyString(pNVPair->pszValue) ? "" : pNVPair->pszValue));
                    BAIL_ON_CENTERIS_ERROR(ceError);
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
CTSaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PCFGSECTION pSectionList
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*PNVPAIR pNVPair = NULL;*/
    FILE* fp = NULL;
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;

    ceError = CTAllocateMemory(strlen(pszConfigFilePath)+strlen(".gpagent")+1,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(pszTmpPath, "%s.gpagent", pszConfigFilePath);

    if ((fp = fopen(pszTmpPath, "w")) == NULL) {
        ceError = CTMapSystemError(ceError);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    ceError = CTSaveConfigSectionListToFile(fp, pSectionList);
    BAIL_ON_CENTERIS_ERROR(ceError);

    fclose(fp); fp = NULL;

    ceError = CTMoveFile(pszTmpPath, pszConfigFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;

error:

    if (bRemoveFile)
        CTRemoveFile(pszTmpPath);

    if (fp)
        fclose(fp);

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;
}

CENTERROR
CTCreateConfigSection(
    PCFGSECTION* ppSectionList,
    PCFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSectionList = NULL;
    PCFGSECTION pSection = NULL;
    PCFGSECTION pTmpSection = NULL;

    if (!ppSectionList || IsNullOrEmptyString(pszSectionName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
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
        ceError = CTAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(pszSectionName, &pSection->pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);

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
        CTFreeSection(pSection);

    *pCreatedSection = NULL;

    return ceError;
}

CENTERROR
CTGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSection = NULL;

    while (pSectionList) {
        if (!strcmp(pSectionList->pszName, pszSectionName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        ceError = CENTERROR_CFG_SECTION_NOT_FOUND;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTGetConfigValueBySection(pSection,
                                        pszName,
                                        ppszValue);
    goto error; /* Not uncommon, no need to log error. */

error:

    return ceError;
}

CENTERROR
CTSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSection = NULL;
    PNVPAIR pNVPair = NULL;
    /*PNVPAIR pTmpNVPair = NULL;*/

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
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
        ceError = CTCreateConfigSection(&pSectionList, &pSection, pszSectionName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTSetConfigValueBySection(pSection,
                                        pszName,
                                        pszValue);
    BAIL_ON_CENTERIS_ERROR(ceError);

    return ceError;

error:

    if (pNVPair)
    {
        CTFreeNVPair(pNVPair);
    }

    return ceError;
}

CENTERROR
CTDeleteConfigSection(
    PCFGSECTION* ppSectionList,
    PCSTR pszSectionName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSection = NULL;
    PCFGSECTION pIterSection = *ppSectionList;
    PCFGSECTION pPrevSection = NULL;

    if (IsNullOrEmptyString(pszSectionName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
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

        CTFreeSection(pSection);
    }

error:

    return ceError;
}


CENTERROR
CTDeleteNameValuePairBySection(
    PCFGSECTION pSection,
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

        CTFreeNVPair(pNVPair);
    }

/*error:*/

    return ceError;
}

CENTERROR
CTDeleteNameValuePairBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSection = NULL;

    if (IsNullOrEmptyString(pszSectionName) ||
        IsNullOrEmptyString(pszName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
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
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTDeleteNameValuePairBySection(pSection, pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

CENTERROR
CTGetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PNVPAIR pNVPair = NULL;

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

        ceError = CTAllocateString(pNVPair->pszValue, ppszValue);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        *ppszValue = NULL;

    }

error:

    return ceError;
}

CENTERROR
CTSetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
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

        ceError = CTAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(pszName, &pNVPair->pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        CTStripWhitespace(pNVPair->pszName);

        if (!IsNullOrEmptyString(pszValue)) {
            ceError = CTAllocateString(pszValue, &pNVPair->pszValue);
            BAIL_ON_CENTERIS_ERROR(ceError);
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

            CTFreeString(pNVPair->pszValue);
            pNVPair->pszValue = NULL;

            if (!IsNullOrEmptyString(pszValue)) {
                ceError = CTAllocateString(pszValue, &pNVPair->pszValue);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        }

    }

    return ceError;

error:

    if (pNVPair) {
        CTFreeNVPair(pNVPair);
    }

    return ceError;
}
