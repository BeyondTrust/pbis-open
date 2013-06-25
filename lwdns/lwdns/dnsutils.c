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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsutils.c
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
DNSBuildLabelList(
    PCSTR  pszString,
    PDWORD pdwPosition,
    PDNS_DOMAIN_LABEL * ppList
    );

static
DWORD
DNSBuildLabel(
    char * szLabel,
    PDNS_DOMAIN_LABEL * ppLabel
    );

static
DWORD
DNSGetToken(
    PCSTR  pszString,
    char * pszToken,
    PDWORD pdwToken,
    PDWORD pdwPosition
    );

DWORD
DNSAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory)
    {
        dwError = ENOMEM;
        *ppMemory = NULL;
    }
    else
    {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    
    return (dwError);
}

DWORD
DNSReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL)
    {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }
    else
    {
        pNewMemory = realloc(pMemory, dwSize);
    }
    
    if (!pNewMemory)
    {
        dwError = ENOMEM;
        *ppNewMemory = NULL;
    }
    else
    {
        *ppNewMemory = pNewMemory;
    }

    return(dwError);
}

VOID
DNSFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}

DWORD
DNSAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;

    if (!pszInputString || !*pszInputString){
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);

    dwError = DNSAllocateMemory(
                    dwLen+1,
                    (PVOID *)&pszOutputString);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:

    LWDNS_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

VOID
DNSFreeString(
    PSTR pszString
    )
{
    DNSFreeMemory(pszString);
}

VOID
DNSStrToUpper(
     PSTR pszString
     )
{
    if (!pszString)
        return;

    while (*pszString != '\0') {
        *pszString = toupper((int)*pszString);
        pszString++;
    }
}

VOID
DNSStrToLower(
     PSTR pszString
     )
{
    if (!pszString)
        return;

    while (*pszString != '\0') {
        *pszString = tolower((int)*pszString);
        pszString++;
    }
}

DWORD
DNSGenerateIdentifier(
     WORD * pwIdentifier
     )
{
    *pwIdentifier = random();

    return 0;
}

DWORD
DNSGetDomainNameLength(
    PDNS_DOMAIN_NAME pDomainName,
    PDWORD pdwLength
    )
{
    DWORD dwError = 0;
    DWORD dwLength = 0;
    PDNS_DOMAIN_LABEL pDomainLabel = NULL;

    if (!pDomainName) {
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    pDomainLabel = pDomainName->pLabelList;

    while(pDomainLabel) {
        
        dwLength += pDomainLabel->dwLength;
        dwLength++;
        
        pDomainLabel = pDomainLabel->pNext;
    }

    *pdwLength = ++dwLength;
    
cleanup:

    return (dwError);
    
error:

    *pdwLength = 0;
    
    goto cleanup;
}

DWORD
DNSCopyDomainName(
    PBYTE pBuffer,
    PDNS_DOMAIN_NAME pDomainName,
    PDWORD pdwCopied
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pDomainLabel = NULL;
    DWORD dwCopied = 0;
    BYTE endChar = '\0';

    if (!pDomainName) {
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    pDomainLabel = pDomainName->pLabelList;
    while (pDomainLabel)
    {
        BYTE len = (BYTE)pDomainLabel->dwLength;
        
        memcpy(pBuffer + dwCopied, &len, sizeof(len));
        dwCopied += sizeof(BYTE);
        
        memcpy(pBuffer + dwCopied, pDomainLabel->pszLabel, pDomainLabel->dwLength);
        dwCopied += pDomainLabel->dwLength;
        
        pDomainLabel = pDomainLabel->pNext;
    }
    memcpy(pBuffer + dwCopied, &endChar, sizeof(BYTE));
    dwCopied += sizeof(BYTE);

    *pdwCopied =  dwCopied;
    
cleanup:

    return(dwError);

error:

    *pdwCopied = 0;
    
    goto cleanup;
}

DWORD
DNSGenerateKeyName(
    PSTR* ppszKeyName
    )
{
    DWORD dwError = 0;
    PSTR  pszKeyName = NULL;
    CHAR  szGUID[37] = "";
    uuid_t uuid= {0};

    uuid_generate(uuid);

    uuid_unparse(uuid, szGUID);

    dwError = DNSAllocateString(szGUID, &pszKeyName);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppszKeyName = pszKeyName;
    
cleanup:

     return dwError;

error:

    *ppszKeyName = NULL;
    
    LWDNS_SAFE_FREE_STRING(pszKeyName);
    
    goto cleanup;
}

DWORD
DNSDomainNameFromString(
    PCSTR pszDomainName,
    PDNS_DOMAIN_NAME * ppDomainName
    )
{
    DWORD dwError = 0;
    DWORD dwPosition = 0;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    PDNS_DOMAIN_LABEL pLabelList = NULL;

    if (IsNullOrEmptyString(pszDomainName))
    {
        dwError = EINVAL;
        return (dwError);
    }

    dwError = DNSBuildLabelList(
                pszDomainName, 
                &dwPosition, 
                &pLabelList);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    sizeof(DNS_DOMAIN_NAME),
                    (PVOID *)&pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDomainName->pLabelList = pLabelList;

    *ppDomainName = pDomainName;

cleanup:

    return dwError;

error:

    if (pLabelList)
    {
        DNSFreeLabelList(pLabelList);
    }
    
    *ppDomainName = NULL;

    goto cleanup;
}

VOID
DNSFreeDomainName(
    PDNS_DOMAIN_NAME pDomainName
    )
{
    DNSFreeLabelList(pDomainName->pLabelList);
    DNSFreeMemory(pDomainName);
    return;
}

DWORD
DNSMapResponseCode(
    WORD wResponseCode
    )
{
    WORD wnResponseCode = 0;
    BYTE *pByte = NULL;
    wnResponseCode = htons(wResponseCode);
    pByte = (BYTE*)&wnResponseCode;

    //
    // Bit 3, 2, 1, 0 of Byte 2 represent the RCode
    //
    return((DWORD)pByte[1]);
}

DWORD
DNSAppendLabel(
    PDNS_DOMAIN_LABEL pLabelList,
    PDNS_DOMAIN_LABEL pLabel,
    PDNS_DOMAIN_LABEL * ppNewLabelList
    )
{
    PDNS_DOMAIN_LABEL * ppLabelList = NULL;
    DWORD dwError = 0;

    if (pLabelList == NULL)
    {
        *ppNewLabelList = pLabel;
        goto done;
    }

    ppLabelList = &pLabelList;

    while((*ppLabelList)->pNext)
    {
        ppLabelList = &((*ppLabelList)->pNext);
    }

    (*ppLabelList)->pNext = pLabel;
    *ppNewLabelList = pLabelList;
    
done:

    return(dwError);
}

#define TOKEN_LABEL      1
#define TOKEN_SEPARATOR  2
#define TOKEN_EOS        3

static
DWORD
DNSBuildLabelList(
    PCSTR  pszString,
    PDWORD pdwPosition,
    PDNS_DOMAIN_LABEL * ppList
    )
{
    DWORD dwError = 0;
    PDNS_DOMAIN_LABEL pList = NULL;
    PDNS_DOMAIN_LABEL pLabel = NULL;
    DWORD dwToken = 0;
    char szToken[64];

    memset(szToken, 0, 64);
    dwError = DNSGetToken(
                pszString,
                szToken,
                &dwToken,
                pdwPosition);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (dwToken != TOKEN_LABEL) {
        dwError = EINVAL;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSBuildLabel(szToken, &pLabel);
    BAIL_ON_LWDNS_ERROR(dwError);

    memset(szToken, 0, 64);
    dwError = DNSGetToken(
                pszString,
                szToken, 
                &dwToken, 
                pdwPosition);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (dwToken == TOKEN_SEPARATOR)
    {
        dwError = DNSBuildLabelList(
                    pszString,
                    pdwPosition,
                    &pList);
        BAIL_ON_LWDNS_ERROR(dwError);

        pLabel->pNext = pList;
        pList = NULL;
    }
    //else if (dwToken == TOKEN_EOS)
    //{
    //    *ppList = pLabel;
    //}
    
    *ppList = pLabel;

cleanup:

    return(dwError);

error:

    if (pLabel) {
        DNSFreeLabel(pLabel);
    }
    
    if (pList) {
        DNSFreeLabelList(pList);
    }

    goto cleanup;
}

static
DWORD
DNSBuildLabel(
    char * szLabel,
    PDNS_DOMAIN_LABEL * ppLabel
    )
{
    PDNS_DOMAIN_LABEL pLabel = NULL;
    PSTR  pszLabel = NULL;
    DWORD dwError = 0;

    dwError = DNSAllocateMemory(
                sizeof(DNS_DOMAIN_LABEL),
                (PVOID *)&pLabel);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateString(szLabel, &pszLabel);
    BAIL_ON_LWDNS_ERROR(dwError);

    pLabel->pszLabel = pszLabel;
    pLabel->dwLength = (DWORD)strlen(pszLabel);
    
    *ppLabel = pLabel;
    
cleanup:

    return(dwError);

error:

    LWDNS_SAFE_FREE_STRING(pszLabel);

    if (pLabel) {
        DNSFreeLabel(pLabel);
    }
    *ppLabel = NULL;
    
    goto cleanup;
}

#define STATE_BEGIN  0
#define STATE_LABEL  1
#define STATE_FINISH 2

static
DWORD
DNSGetToken(
    PCSTR  pszString,
    char * pszToken,
    PDWORD pdwToken,
    PDWORD pdwPosition
    )
{
    DWORD dwError = 0;
    char  c = 0;
    DWORD dwToken = 0;
    DWORD i = 0;
    DWORD dwState = 0;
    DWORD dwPosition = 0;

    dwPosition = *pdwPosition;
    dwState = STATE_BEGIN;
    while(dwState != STATE_FINISH) {
        c = pszString[dwPosition];
        if (c =='\0') {
            if (dwState == STATE_LABEL) {
                dwToken = TOKEN_LABEL;
                dwState = STATE_FINISH;
                continue;
            }else if (dwState == STATE_BEGIN) {
                dwToken = TOKEN_EOS;
                dwState = STATE_FINISH;
                continue;
            }
        }else if (isalnum((int)c) || c == '-') {
            pszToken[i++] = c;
            dwPosition++;
            dwState = STATE_LABEL;
            continue;
        }else if (c == '.') {
            if (dwState == STATE_LABEL) {
                dwToken = TOKEN_LABEL;
                dwState = STATE_FINISH;
                continue;
            }else if (dwState == STATE_BEGIN) {
                dwToken = TOKEN_SEPARATOR;
                dwPosition++;
                dwState = STATE_FINISH;
                continue;
            }
        }else {
            if (dwState == STATE_LABEL) {
                dwToken = TOKEN_LABEL;
                dwState = STATE_FINISH;
            }else if (dwState == 0) {
                dwError = ERROR_INVALID_PARAMETER;
                dwState = STATE_FINISH;
            }
        }
    }
    *pdwPosition = dwPosition;
    *pdwToken = dwToken;
    
    return(dwError);
}

VOID
DNSFreeLabelList(
    PDNS_DOMAIN_LABEL pLabelList
    )
{
    while(pLabelList)
    {
        PDNS_DOMAIN_LABEL pTemp = pLabelList;
        
        pLabelList = pLabelList->pNext;
        
        DNSFreeLabel(pTemp);
    }
}

VOID
DNSFreeLabel(
    PDNS_DOMAIN_LABEL pLabel
    )
{
    if (pLabel)
    {
        LWDNS_SAFE_FREE_STRING(pLabel->pszLabel);
        DNSFreeMemory(pLabel);
    }
    
    return;
}

#ifndef HAVE_HPUX_OS
DWORD
DNSInet6ValidateAddress(
    PCSTR pszInet6InputAddr
    )
{
    CHAR szTempBuffer[INETV6_ADDRSTRLEN] = {0};
    DWORD dwTempIndex = 0,dwPairCount = 0;
    PSTR pTemp = NULL;
    DWORD dwInputIndex = 0;
    DWORD dwNumbercount = 0;
    BOOLEAN bIsInValid = FALSE;
    CHAR szQuadCountString[INETV6_ADDRSTRLEN] = {0};
    DWORD dwColonCount = 0, dwQuadCount = 0, dwInputStringLength = strlen(pszInet6InputAddr);

    if(dwInputStringLength > INETV6_ADDRSTRLEN)
    {
        bIsInValid = TRUE;
        goto cleanup;
    }

    //check each charecter for hexadecimal number in range of 0 to 9; a to f 
    //and for colon(:)
    strcpy(szTempBuffer,pszInet6InputAddr);

    while(*(szTempBuffer+dwTempIndex) != '\0'){
        if(((*(szTempBuffer+dwTempIndex) >= '0') && (*(szTempBuffer+dwTempIndex) <= '9')) ||
            ((*(szTempBuffer+dwTempIndex) >= 'a') && (*(szTempBuffer+dwTempIndex) <= 'f')) ||
            ((*(szTempBuffer+dwTempIndex) == ':')));
        else {
            bIsInValid = TRUE;
            goto cleanup;
        }  
        dwTempIndex++;
    }

    //copy the input string to dwQuadCount_string to count quad correctness
    strcpy(szQuadCountString, pszInet6InputAddr);

    if(*(szQuadCountString) == ':' && *(szQuadCountString+1) != ':')
        bIsInValid = TRUE;
    if(*(szQuadCountString+dwInputStringLength-2) != ':' && *(szQuadCountString+dwInputStringLength-1) == ':')
        bIsInValid = TRUE;

    while(*(szQuadCountString+dwInputIndex) != '\0')
    {
        if(*(szQuadCountString+dwInputIndex) != ':')
        {
            dwNumbercount++;
            if(dwNumbercount > 4)
            {
                 bIsInValid = TRUE;
                 goto cleanup;
            }
        }
        else
            dwNumbercount = 0;

        dwInputIndex++;
    }

    //copy the input string to dwQuadCount_string to count the number of quads
    strcpy(szQuadCountString, pszInet6InputAddr);

    //tokenize each quad and count it 
    pTemp = strtok(szQuadCountString,":");
    if (pTemp != NULL ) 
    {
	    dwQuadCount++;
    }
    while((pTemp = strtok(NULL,":")) != NULL) {
        dwQuadCount++;
    }

    //copy the input string to temp string to count the colons
    strcpy(szTempBuffer,pszInet6InputAddr); 
    dwInputIndex = 0;

    //count the number of colons in input string
    while(*(pszInet6InputAddr+dwInputIndex) != '\0'){
         if(*(pszInet6InputAddr+dwInputIndex) == ':')
             dwColonCount++;
         dwInputIndex++;
    }

    //cehck for multiple pair of colons example fd50::21::21
    for(dwInputIndex = 0; *(pszInet6InputAddr+dwInputIndex+1) != '\0';dwInputIndex++)
    {
         if(*(pszInet6InputAddr+dwInputIndex) == ':' && *(pszInet6InputAddr+dwInputIndex+1) == ':')
         {
              dwPairCount++;
              dwInputIndex++;
         }
    }
    if(dwPairCount > 1)
        bIsInValid = TRUE;

    //check for :1
    if( dwColonCount == 1)
        bIsInValid = TRUE;
    //check for __
    if(dwQuadCount == 0 || dwColonCount == 0)
        bIsInValid = TRUE;
    //check for a32:5435:5466:6757:7641:ef:765:6532:534f:849a
    if(dwQuadCount > 8 && dwColonCount > 7)
        bIsInValid = TRUE;
    //check for 5466:6757:7641:ef:765:6532:534f::849a
    if(dwQuadCount == 8 && dwColonCount > 7)
        bIsInValid = TRUE;
    //check for 3920:aefc::849a::6757
    if(dwQuadCount < dwColonCount-1 && dwQuadCount != 1)
	    bIsInValid = TRUE;
    //check for 21:21:21
    if(dwQuadCount <= 7 && dwColonCount <= 6)
    {
        if(dwQuadCount == (dwColonCount+1))
            bIsInValid = TRUE;
    }
    //check for : or 1
    if(strlen(pszInet6InputAddr)  <= 1)
        bIsInValid = TRUE;
    //check for :1: 
    if(strstr(pszInet6InputAddr,"::") == NULL && dwColonCount == 2)
        bIsInValid = TRUE; 

cleanup:

    if(bIsInValid == TRUE)
        return DNS_ERROR_INVALID_IP_ADDRESS;
    return FALSE;
}

VOID
DNSInet6AddressReverse(
    PSTR pszInet6Addr
    )
{
    DWORD dwInputLength = strlen(pszInet6Addr) - 1;
    DWORD dwIndex = 0;
    PSTR pInputEnd = pszInet6Addr+dwInputLength;
    CHAR szTemp = '0';

    for( dwIndex = 0 ;dwIndex < dwInputLength/2 ; dwIndex++)
    {
        szTemp = *pInputEnd;
        *pInputEnd = *pszInet6Addr;
        *pszInet6Addr = szTemp;
        pszInet6Addr++;
        pInputEnd--;
    }
}

VOID
DNSInet6Canonicalize(
    PSTR pszInet6InputAddr,
    PSTR pszInet6OutputAddr
    )
{
    PSTR pToken = NULL;
    DWORD dwAddressCounter = 0;
    DWORD dwTokenCount = 0;
    DWORD dwOutputIndex = 0;

    DNSInet6AddressReverse(pszInet6InputAddr);
    //take the first token from the input string
    pToken = strtok(pszInet6InputAddr,":");
    //Take each charecter from token and add charecter with a 
    //dot(.) to output string
    if(pToken != NULL){
        while(*(pToken+dwTokenCount) != '\0'){
            *(pszInet6OutputAddr+(dwOutputIndex++)) = *(pToken+dwTokenCount);
            *(pszInet6OutputAddr+(dwOutputIndex++)) = '.';
            dwTokenCount++;
        }
    }

    //Get the sequence of tokens from input string and
    //break tokens into charectersand add charecter with a
    //dot(.) to output string
    for(dwAddressCounter = 0;dwAddressCounter < 7;dwAddressCounter++)
    {
        dwTokenCount = 0;
        pToken = strtok(NULL,":");
        if(pToken != NULL){
            while(*(pToken+dwTokenCount) != '\0'){
                *(pszInet6OutputAddr+(dwOutputIndex++)) = *(pToken+dwTokenCount);
                *(pszInet6OutputAddr+(dwOutputIndex++)) = '.';
                dwTokenCount++;
            }
        }
    }
    //append end of string to output string
    *(pszInet6OutputAddr+(dwOutputIndex)) = '\0';
}

VOID
DNSInet6FillUpZeros(
    PSTR pszInet6InputAddr,
    PSTR pszInet6OutputAddr
    )
{
    DWORD dwInputIndex = 0;
    DWORD dwOutputIndex = 0;
    CHAR strInput[INETV6_ADDRSTRLEN] = {0};
    CHAR strOutput[INETV6_ADDRSTRLEN] = {0};

    //copy the input string to strInput to ensure any changes
    strcpy(strInput,pszInet6InputAddr);
    //Take the template of ipv6 address filled with all zero's
    strcpy(strOutput,"0000:0000:0000:0000:0000:0000:0000:0000");

    //compute the length of input and output strings
    dwInputIndex = strlen(strInput);
    dwOutputIndex = strlen(strOutput);

    //comparing the input string with output string from backside 
    //so until the input index becomes -1 keep looping
    while(dwInputIndex != -1 ){
        //if ':' is not encountered in input string copy it to output
        if( strInput[dwInputIndex] != ':' )
            strOutput[dwOutputIndex--] = strInput[dwInputIndex--];
        else
            {
                //if colon is encountered then decrement the input 
                //index to point to next nibble
                dwInputIndex--;
                //Until output string reaches the next colon keep 
                //decrement the index so that it will go hand in hand with input string
                while(strOutput[dwOutputIndex--] != ':' );
            }
    }
    //copy the zero added address to output string
    strcpy(pszInet6OutputAddr,strOutput);
}

VOID
DNSInet6ExpandAddress(
    PSTR pszInet6InputAddr
    )
{
    DWORD dwTempStringIndex = 0; 
    PSTR pStartOfColon = NULL;
    DWORD dwInputIndex = 0;
    DWORD dwTotalInputLength = strlen(pszInet6InputAddr);
    CHAR szTempInputSring[INETV6_ADDRSTRLEN] = {0};
    CHAR szTempStringMid[INETV6_ADDRSTRLEN] = {0};
    CHAR szOutput[INETV6_ADDRSTRLEN] = {0};

    while(*(pszInet6InputAddr+dwInputIndex) != '\0')
    {
        if(*(pszInet6InputAddr+dwInputIndex) == ':' && *(pszInet6InputAddr+dwInputIndex+1) == ':')
        {
            pStartOfColon = pszInet6InputAddr+dwInputIndex;
            break;
        }
        dwInputIndex++;
    }
    dwInputIndex = 0;

    if(pStartOfColon != NULL)
    {
        DWORD dwQuadCount = 0;
        PSTR pTempQuadString = NULL;
        CHAR szQuadString[INETV6_ADDRSTRLEN] = {0};
        DWORD dwZeroNumberOfQuads = 0;

        strcpy(szQuadString, pszInet6InputAddr);
        pTempQuadString = strtok(szQuadString,":");
        if (pTempQuadString != NULL ) dwQuadCount++;             //first quad
        while((pTempQuadString = strtok(NULL,":")) != NULL) dwQuadCount++; //rest of quad's

        dwZeroNumberOfQuads = 8 - dwQuadCount;

        if ( pStartOfColon == pszInet6InputAddr )
        {
            strcpy ( szTempStringMid, pStartOfColon+2 ); 
            while(dwZeroNumberOfQuads--)
            {
                 strcpy(pszInet6InputAddr+dwTempStringIndex,"0000:");
                 dwTempStringIndex+=5;
            }
            strcat( pszInet6InputAddr , szTempStringMid );
            strcpy(szTempInputSring,pszInet6InputAddr);
        }
        else if ( *(pszInet6InputAddr+dwTotalInputLength-1) == ':' && 
            *(pszInet6InputAddr+dwTotalInputLength-2) == ':') 
        {
            *(pszInet6InputAddr+dwTotalInputLength-2) = '\0';
            while(dwZeroNumberOfQuads--)
            {
                strcat(pszInet6InputAddr, ":0000");
            }
            strcpy(szTempInputSring, pszInet6InputAddr);
        }
        else
        { 
            strcpy ( szTempStringMid, pStartOfColon+1);
            while(dwZeroNumberOfQuads--)
            {
                strcpy(pStartOfColon,":0000");
                pStartOfColon = pStartOfColon+5;
            }
            strcat(pStartOfColon, szTempStringMid);
            strcpy(szTempInputSring, pszInet6InputAddr);            
        }
        DNSInet6FillUpZeros(szTempInputSring, szOutput);
    }
    else
    {
        DNSInet6FillUpZeros(pszInet6InputAddr, szOutput);
    }

    strcpy(pszInet6InputAddr,szOutput);
}

DWORD
DNSInet6GetPtrAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppSzInet6OutputAddr
    )
{
    CHAR szInput[INETV6_ADDRSTRLEN] = {0};
    CHAR szOutput[CANONICAL_INET6_ADDRSTRLEN] = {0};
    PSTR pSzOutputAddr = NULL;

    strcpy(szInput,pszInet6InputAddr);
    if(DNSInet6ValidateAddress(szInput)){
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }
    pSzOutputAddr = (PSTR) malloc(sizeof(CHAR) * CANONICAL_INET6_ADDRSTRLEN);
    DNSInet6ExpandAddress(szInput);
    DNSInet6Canonicalize(szInput,szOutput);
    strncat(szOutput,"ip6.arpa",8);
    strcpy(pSzOutputAddr,szOutput);

    *ppSzInet6OutputAddr = pSzOutputAddr;
    return 0; 
}

DWORD
DNSInet6GetPtrZoneAddress(
    PSTR pszInet6InputAddr,
    PSTR *ppSzInet6OutputAddr
    )
{
    CHAR szInput[INETV6_ADDRSTRLEN] = {0};
    CHAR szOutput[CANONICAL_INET6_ADDRSTRLEN] = {0};
    PSTR pSzOutputAddr = NULL;
    DWORD dwSubNetMask = SUBNET_MASK;
    DWORD dwIpaddressToChop = dwSubNetMask / HEXADECIMAL_BASE;
 
    strcpy(szInput,pszInet6InputAddr);

    if(DNSInet6ValidateAddress(szInput)){
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }

    pSzOutputAddr = (PSTR) malloc(sizeof(CHAR) * CANONICAL_INET6_ADDRSTRLEN);

    DNSInet6ExpandAddress(szInput);

    DNSInet6Canonicalize(szInput,szOutput);

    strcpy(pSzOutputAddr,szOutput+(dwIpaddressToChop * 8));
    strncat(pSzOutputAddr,"ip6.arpa",8);

    *ppSzInet6OutputAddr = pSzOutputAddr;
    return 0;
}
#endif
