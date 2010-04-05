/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "includes.h"

static
void
LWFreeNVPair(
    PNVPAIR pNVPair
    );

static
void
LWFreeNVPairList(
    PNVPAIR pNVPairList
    );

static
void
LWFreeNVPairList(
    PNVPAIR pNVPairList
    );

static
PNVPAIR
ReverseNVPairList(
    PNVPAIR pNVPairList
    );

static
PCFGSECTION
ReverseSectionsAndNVPairs(
    PCFGSECTION pSectionList
    );

static
DWORD
ParseHeader(
    FILE* fp,
    PDWORD pdwSignature);

static
DWORD
ReadNextDoubleByteLine(
    FILE* fp,
    PSTR pszBuf,
    DWORD dwMaxLen,
    PBOOLEAN pbEndOfFile
    );

static
void
LWFreeSection(
    PCFGSECTION pSection
    );

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
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (bWindowsDoubleByteFormat) {
        dwError = ParseHeader(fp, &dwSignature);
        BAIL_ON_LWUTIL_ERROR(dwError);

        if (dwSignature != 0xFEFF) {
            dwError = LWUTIL_ERROR_INVALID_TAG;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }
    }

    while (!bEOF) {
        LW_SAFE_FREE_STRING(szBuf);

        if (bWindowsDoubleByteFormat) {

            staticBuffer[0] = '\0';
            dwError = ReadNextDoubleByteLine(fp, staticBuffer, 1024, &bEOF);
            BAIL_ON_LWUTIL_ERROR(dwError);
            dwError = LWAllocateString(staticBuffer, &szBuf);
            BAIL_ON_LWUTIL_ERROR(dwError);

        } else {

            dwError = LWReadNextLine(fp, &szBuf, &bEOF);
            BAIL_ON_LWUTIL_ERROR(dwError);

        }

        LWStripWhitespace(szBuf);

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

            dwError = LWAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
            BAIL_ON_LWUTIL_ERROR(dwError);

            szBuf[dwLen-1] = '\0';

            dwError = LWAllocateString(szBuf+1, &pSection->pszName);
            BAIL_ON_LWUTIL_ERROR(dwError);

            LWStripWhitespace(pSection->pszName);

        } else {

            if (!pSection) {
                dwError = LWUTIL_ERROR_NO_SUCH_ATTRIBUTE;
                BAIL_ON_LWUTIL_ERROR(dwError);
            }

            if ((pszTmp = strchr(szBuf, '=')) == NULL) {
                dwError = LWUTIL_ERROR_INVALID_TAG;
                BAIL_ON_LWUTIL_ERROR(dwError);
            }

            if (pszTmp == szBuf) {
                dwError = LWUTIL_ERROR_INVALID_TAG;
                BAIL_ON_LWUTIL_ERROR(dwError);
            }

            dwError = LWAllocateMemory(pszTmp-szBuf+1, (PVOID*)&pszName);
            BAIL_ON_LWUTIL_ERROR(dwError);

            strncpy(pszName, szBuf, pszTmp-szBuf);

            pszTmp++;
            while (*pszTmp != '\0' && isspace((int)*pszTmp))
                pszTmp++;

            if (*pszTmp != '\0') {
                dwError = LWAllocateString(pszTmp, &pszValue);
                BAIL_ON_LWUTIL_ERROR(dwError);
            }

            dwError = LWAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
            BAIL_ON_LWUTIL_ERROR(dwError);

            LWStripWhitespace(pszName);
            LWStripWhitespace(pszValue);

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
            dwError = LWUTIL_ERROR_INVALID_RECORD_TYPE;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        if (fprintf(fp, "[%s]\r\n", pSectionList->pszName) < 0) {
            dwError = errno;
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        pNVPair = pSectionList->pNVPairList;
        while (pNVPair) {

            if (IsNullOrEmptyString(pNVPair->pszName)) {
                dwError = LWUTIL_ERROR_INVALID_ATTRIBUTE_TYPE;
                BAIL_ON_LWUTIL_ERROR(dwError);
            }

            if (fprintf(fp, "    %s = %s\r\n",
                        pNVPair->pszName,
                        (IsNullOrEmptyString(pNVPair->pszValue) ? "" : pNVPair->pszValue)) < 0) {
                dwError = errno;
                BAIL_ON_LWUTIL_ERROR(dwError);
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

    dwError = LWAllocateMemory(strlen(pszConfigFilePath)+strlen(".macadutil")+1,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sprintf(pszTmpPath, "%s.macadutil", pszConfigFilePath);

    if ((fp = fopen(pszTmpPath, "w")) == NULL) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (fcntl(fileno(fp), F_SETFD, FD_CLOEXEC) < 0) {
        dwError = errno;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    bRemoveFile = TRUE;

    dwError = LWSaveConfigSectionListToFile(fp, pSectionList);
    BAIL_ON_LWUTIL_ERROR(dwError);

    fclose(fp); fp = NULL;

    dwError = LWMoveFile(pszTmpPath, pszConfigFilePath);
    BAIL_ON_LWUTIL_ERROR(dwError);
    
    bRemoveFile = FALSE;

cleanup:

    if (bRemoveFile)
    {
        LWRemoveFile(pszTmpPath);
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
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
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
        dwError = LWAllocateMemory(sizeof(CFGSECTION), (PVOID*)&pSection);
        BAIL_ON_LWUTIL_ERROR(dwError);

        dwError = LWAllocateString(pszSectionName, &pSection->pszName);
        BAIL_ON_LWUTIL_ERROR(dwError);

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
        dwError = LWUTIL_ERROR_INVALID_RECORD_TYPE;
        BAIL_ON_LWUTIL_ERROR(dwError);
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
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    while (pSectionList) {
        if (!strcmp(pSectionList->pszName, pszSectionName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        dwError = LWUTIL_ERROR_INVALID_RECORD_TYPE;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWSetConfigValueBySection(pSection,
                                        pszName,
                                        pszValue);
    BAIL_ON_LWUTIL_ERROR(dwError);

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
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
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
        dwError = LWUTIL_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    while (pSectionList) {
        if (!strcmp(pszSectionName, pszName)) {
            pSection = pSectionList;
            break;
        }
        pSectionList = pSectionList->pNext;
    }

    if (!pSection) {
        dwError = LWUTIL_ERROR_INVALID_RECORD_TYPE;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LWDeleteNameValuePairBySection(pSection, pszName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;
    
error:

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
        dwError = LWUTIL_ERROR_INVALID_ATTRIBUTE_TYPE;
        goto cleanup; /* Not uncommon, no need to log error. */
    }

    if (pNVPair->pszValue) {

        dwError = LWAllocateString(pNVPair->pszValue, ppszValue);
        BAIL_ON_LWUTIL_ERROR(dwError);

    }

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

        dwError = LWAllocateMemory(sizeof(NVPAIR), (PVOID*)&pNVPair);
        BAIL_ON_LWUTIL_ERROR(dwError);

        dwError = LWAllocateString(pszName, &pNVPair->pszName);
        BAIL_ON_LWUTIL_ERROR(dwError);

        LWStripWhitespace(pNVPair->pszName);

        if (!IsNullOrEmptyString(pszValue)) {
            dwError = LWAllocateString(pszValue, &pNVPair->pszValue);
            BAIL_ON_LWUTIL_ERROR(dwError);
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

            LWFreeString(pNVPair->pszValue);
            pNVPair->pszValue = NULL;

            if (!IsNullOrEmptyString(pszValue)) {
                dwError = LWAllocateString(pszValue, &pNVPair->pszValue);
                BAIL_ON_LWUTIL_ERROR(dwError);
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

static
void
LWFreeNVPair(
    PNVPAIR pNVPair
    )
{
    if (pNVPair->pszName)
        LWFreeString(pNVPair->pszName);

    if (pNVPair->pszValue)
        LWFreeString(pNVPair->pszValue);

    LWFreeMemory(pNVPair);
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
    if (pSection->pszName)
        LWFreeString(pSection->pszName);
    if (pSection->pNVPairList)
        LWFreeNVPairList(pSection->pNVPairList);
    LWFreeMemory(pSection);
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
        BAIL_ON_LWUTIL_ERROR(dwError);
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
                BAIL_ON_LWUTIL_ERROR(dwError);

            }

        }

        if ((*pszTmp++ = dbBuf.b1) == '\n')
            break;

        iBuf++;
    }

    if (iBuf == dwMaxLen) {
        dwError = LWUTIL_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    *pszTmp = '\0';

cleanup:

    return dwError;
    
error:

    goto cleanup;
}
