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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwps-sid.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "lwps-utils.h"
#include "lwps-sid_p.h"
#include "lwps-sid.h"

DWORD
LwpsAllocSecurityIdentifierFromBinary(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PLWPS_SECURITY_IDENTIFIER* ppSecurityIdentifier
    )
{
    DWORD dwError = 0;
    
    PLWPS_SECURITY_IDENTIFIER pSID = NULL;
    
    dwError = LwpsAllocateMemory(
                   sizeof(LWPS_SECURITY_IDENTIFIER), 
                  (PVOID*)&pSID);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsAllocateMemory(
                  dwSidBytesLength * sizeof(UCHAR), 
                  (PVOID*)&(pSID->pucSidBytes));
    BAIL_ON_LWPS_ERROR(dwError);
        
    pSID->dwByteLength = dwSidBytesLength;
    
    memcpy(pSID->pucSidBytes, pucSidBytes, dwSidBytesLength);
    
    *ppSecurityIdentifier = pSID;
    
cleanup:

    return dwError;

error:
    
    if (pSID) 
    {
       LwpsFreeSecurityIdentifier(pSID);
    }
    *ppSecurityIdentifier = NULL;
        
    goto cleanup;
}

DWORD
LwpsAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLWPS_SECURITY_IDENTIFIER* ppSecurityIdentifier
    )
{
    DWORD dwError = 0;
    PLWPS_SECURITY_IDENTIFIER pSID = NULL;
    
    dwError = LwpsAllocateMemory(
                   sizeof(LWPS_SECURITY_IDENTIFIER), 
                   (PVOID*)&pSID);
    BAIL_ON_LWPS_ERROR(dwError);
   
    
    dwError = LwpsStringToBytes(
                    pszSidString, 
                    &(pSID->pucSidBytes), 
                    &(pSID->dwByteLength));
    BAIL_ON_LWPS_ERROR(dwError);
    
    *ppSecurityIdentifier = pSID;
    
cleanup:

    return dwError;

error:
    
    if (pSID) {
       LwpsFreeSecurityIdentifier(pSID);
    }
    *ppSecurityIdentifier = NULL;
        
    goto cleanup;
}

VOID
LwpsFreeSecurityIdentifier(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier
    )
{
    LWPS_SAFE_FREE_MEMORY(pSecurityIdentifier->pucSidBytes);
    
    LwpsFreeMemory(pSecurityIdentifier);
}



DWORD
LwpsGetSecurityIdentifierRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    )
{
    DWORD dwError = 0;
    DWORD dwRid = 0;
     
    UCHAR* pucSidBytes = NULL;
    DWORD dwByteLength = 0;
     
    if(!pSecurityIdentifier || 
       !pSecurityIdentifier->pucSidBytes || 
       pSecurityIdentifier->dwByteLength < SECURITY_IDENTIFIER_MINIMUM_SIZE)
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
     
    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;
     
    //verify the SID is version 1.
    if(pucSidBytes[0] != 1)
    {
        dwError = LWPS_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LWPS_ERROR(dwError);
    }
     
    //verify the number of bytes is plausible
    if((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0) 
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    memcpy(&dwRid, pucSidBytes+dwByteLength-sizeof(DWORD), sizeof(DWORD));
    
    #if defined(WORDS_BIGENDIAN)
        dwRid = LW_ENDIAN_SWAP32(dwRid);
    #endif
    
    *pdwRid = dwRid;
    
    return dwError;
    
error:
 
    *pdwRid = 0;

    return dwError;
    
    
}

DWORD
LwpsSetSecurityIdentifierRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    )
{
    DWORD dwError = 0;
    DWORD dwRidLocal = dwRid;
    UCHAR* pucSidBytes = NULL;
    DWORD dwByteLength = 0;
     
    if(!pSecurityIdentifier || 
       !pSecurityIdentifier->pucSidBytes || 
       pSecurityIdentifier->dwByteLength < SECURITY_IDENTIFIER_MINIMUM_SIZE)
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
     
    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;
     
    //verify the SID is version 1.
    if(pucSidBytes[0] != 1)
    {
        dwError = LWPS_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LWPS_ERROR(dwError);
    }
     
    //verify the number of bytes is plausible
    if((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0) 
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    #if defined(WORDS_BIGENDIAN)
        dwRidLocal = LW_ENDIAN_SWAP32(dwRidLocal);
    #endif
        
    memcpy(pucSidBytes+dwByteLength-sizeof(DWORD), &dwRidLocal, sizeof(DWORD));
    
    
error:
 
    return dwError;
    
    
}

//The UID is a DWORD constructued using
//a non-cryptographic, 2-way hash of 
//the User SID and Domain SID.
DWORD
LwpsGetSecurityIdentifierHashedRid(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    )
{
    DWORD i = 0;
    DWORD dwError = 0;
    
    //dwAuthorityCount includes the final RID
    DWORD dwAuthorityCount = 0;
    PDWORD pdwAuthorities = NULL;
    DWORD dwHash = 0;
    
    UCHAR* pucSidBytes = NULL;
    DWORD dwByteLength = 0;
    
    if(!pSecurityIdentifier || 
       !pSecurityIdentifier->pucSidBytes || 
       pSecurityIdentifier->dwByteLength < SECURITY_IDENTIFIER_MINIMUM_SIZE)
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;
    
    //verify the SID is version 1.
    if(pucSidBytes[0] != 1)
    {
        dwError = LWPS_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    //verify the number of bytes is plausible
    if((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0) 
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
  
    dwAuthorityCount = 
        ((dwByteLength - SECURITY_IDENTIFIER_HEADER_SIZE) / 
        sizeof(DWORD));
    
    dwError = LwpsAllocateMemory(
              dwAuthorityCount * sizeof(DWORD), 
              (PVOID*)&pdwAuthorities);
    BAIL_ON_LWPS_ERROR(dwError);
    
    memcpy((PVOID)pdwAuthorities, 
           (PVOID)(pucSidBytes + SECURITY_IDENTIFIER_HEADER_SIZE),
           dwByteLength - SECURITY_IDENTIFIER_HEADER_SIZE);
    
    for(i = 0; i < dwAuthorityCount; i++)
    {
        #if defined(WORDS_BIGENDIAN)
            pdwAuthorities[i] = LW_ENDIAN_SWAP32(pdwAuthorities[i]);
        #endif
    } 
    
    LwpsUidHashCalc(pdwAuthorities, dwAuthorityCount, &dwHash);
    
    *dwHashedRid = dwHash;
    
cleanup:

    LWPS_SAFE_FREE_MEMORY(pdwAuthorities);
    
    return dwError;

error:
            
    *dwHashedRid = 0;

    goto cleanup;
}

DWORD
LwpsGetSecurityIdentifierBinary(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    )
{
    DWORD dwError = 0;
    UCHAR* pucSidBytes = NULL;
    
    if (pSecurityIdentifier->dwByteLength <= 0 ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsAllocateMemory(
                    pSecurityIdentifier->dwByteLength * sizeof(UCHAR), 
                    (PVOID*)&pucSidBytes);
    BAIL_ON_LWPS_ERROR(dwError);
    
    memcpy(pucSidBytes, 
           pSecurityIdentifier->pucSidBytes, 
           pSecurityIdentifier->dwByteLength);
    
    *ppucSidBytes = pucSidBytes;
    *pdwSidBytesLength = pSecurityIdentifier->dwByteLength;
    
cleanup:

    return dwError;

error:  
    
    LWPS_SAFE_FREE_MEMORY(pucSidBytes);

    *ppucSidBytes = NULL;
    *pdwSidBytesLength = 0;
        
    goto cleanup;
}

DWORD
LwpsGetSecurityIdentifierString(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    )
{
    DWORD dwError = 0;
    PSTR pszSidStr = NULL;
    
    if (pSecurityIdentifier->dwByteLength < 8 ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
       dwError = LWPS_ERROR_INVALID_SID;
       BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsSidBytesToString(pSecurityIdentifier->pucSidBytes, 
            pSecurityIdentifier->dwByteLength, &(pszSidStr));
    BAIL_ON_LWPS_ERROR(dwError);

    
    *ppszSidStr = pszSidStr;
    
cleanup:

    return dwError;

error:

    LWPS_SAFE_FREE_STRING(pszSidStr);

    ppszSidStr = NULL;
    
    goto cleanup;
}

DWORD
LwpsGetDomainSecurityIdentifier(
    PLWPS_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLWPS_SECURITY_IDENTIFIER* ppDomainSID
    )
{
    DWORD dwError = 0;
    PLWPS_SECURITY_IDENTIFIER pDomainSID = NULL;
    UCHAR* pucDomainSID = NULL;
    DWORD dwDomainSIDByteLength = 0;
    
    if ((pSecurityIdentifier->dwByteLength <= 
         SECURITY_IDENTIFIER_MINIMUM_SIZE + sizeof(DWORD)) ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwDomainSIDByteLength = pSecurityIdentifier->dwByteLength - sizeof(DWORD);
    dwError = LwpsAllocateMemory(
                dwDomainSIDByteLength,
                (PVOID*)&pucDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);
    
    memcpy(pucDomainSID,
           pSecurityIdentifier->pucSidBytes,
           dwDomainSIDByteLength);
    
    pucDomainSID[1]--; //decrement word count
    
    dwError = LwpsAllocSecurityIdentifierFromBinary(
                pucDomainSID,
                dwDomainSIDByteLength,
                &pDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);
    
    *ppDomainSID = pDomainSID;
    
cleanup:

    LWPS_SAFE_FREE_MEMORY(pucDomainSID);

    return dwError;

error:  
    
    if (pDomainSID)
    {
        LwpsFreeSecurityIdentifier(pDomainSID);
    }

    *ppDomainSID = NULL;
        
    goto cleanup;
}

DWORD
LwpsHexStrToByteArray(
    PCSTR   pszHexString,
    UCHAR** ppucByteArray,
    DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = strlen(pszHexString);
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = dwHexChars / 2;
    
    if ((dwHexChars & 0x00000001) != 0) 
    {
       dwError = LWPS_ERROR_INVALID_PARAMETER;
       BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsAllocateMemory(
                  sizeof(UCHAR)*(dwByteArrayLength), 
                  (PVOID*)&pucByteArray
                  );
    BAIL_ON_LWPS_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];
        
        UCHAR ucHi = 0;
        UCHAR ucLow = 0;
      
        dwError = HexCharToByte(hexHi, &ucHi);
        BAIL_ON_LWPS_ERROR(dwError);
      
        dwError = HexCharToByte(hexLow, &ucLow);
        BAIL_ON_LWPS_ERROR(dwError);
      
        pucByteArray[i] = (ucHi * 16) + ucLow;
    }
    
    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;
    
cleanup:
    
    return dwError;

error:
    
    LWPS_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;
    
    goto cleanup;
}

DWORD
LwpsByteArrayToHexStr(
    UCHAR* pucByteArray,
    DWORD dwByteArrayLength,
    PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;
    
    dwError = LwpsAllocateMemory(
                (dwByteArrayLength*2 + 1) * sizeof(CHAR), 
                (PVOID*)&pszHexString);
    BAIL_ON_LWPS_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf((char*)pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }
    
    *ppszHexString = pszHexString;
    
cleanup:

    return dwError;

error:

    *ppszHexString = NULL;
    goto cleanup;
}


DWORD
HexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    )
{
    DWORD dwError = 0;
    UCHAR ucByte = 0;
    
    if (cHexChar >= '0' && cHexChar <= '9')
    {
       ucByte = (UCHAR)(cHexChar - '0');
    }
    else if (cHexChar >= 'a' && cHexChar <= 'f')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'a');
    }
    else if (cHexChar >= 'A' && cHexChar <= 'F')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'A');
    }
    else 
    {
       dwError = LWPS_ERROR_INVALID_PARAMETER;
       BAIL_ON_LWPS_ERROR(dwError);
    }
    
    *pucByte = ucByte;
    
cleanup:

    return dwError;

error:
    
    *pucByte = 0;

    goto cleanup;
}
    
void
LwpsUidHashCalc(
    PDWORD pdwAuthorities,
    DWORD dwAuthorityCount,
    PDWORD pdwHash
    )
{
    DWORD dwHash = 0;
    DWORD dwHashTemp = 0;
    
    // xor the last three (non rid) subauths
    if(dwAuthorityCount > 3)
    {
        dwHash ^= pdwAuthorities[dwAuthorityCount - 4];
        dwHash ^= pdwAuthorities[dwAuthorityCount - 3];
        dwHash ^= pdwAuthorities[dwAuthorityCount - 2];
    }

    dwHashTemp = dwHash;
    
    // squish into 12 bits
    dwHash = (dwHashTemp & 0xFFF00000) >> 20;
    dwHash += (dwHashTemp & 0x000FFF00) >> 8;
    dwHash += (dwHashTemp & 0x000000FF);
    dwHash &= 0x0000FFF;
    
    

    // now, combine with 19 bits of the RID
    dwHash <<= 19;
    dwHash += (pdwAuthorities[dwAuthorityCount - 1] & 0x0007FFFF);
    
    *pdwHash = dwHash;
    
}

DWORD
LwpsStringToBytes(
    PCSTR pszSidString,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    )
{
    DWORD dwError = 0;
    PSTR pszSidCopy = NULL;
    PSTR pszToken = NULL;
    DWORD dwTokenCount = 0;
    DWORD dwParseMode = PARSE_MODE_OPEN;
    PCSTR pszDelim = "-";
    PSTR pszStrTokState = NULL;
    DWORD dwSidStringLength = 0;
    DWORD dwRevision = 0;
    UINT64 uiAuth = 0;
    DWORD* pdwTail = NULL;
    INT iTailCount = 1;
    DWORD dwSidBytesLength = 0;
    UCHAR* pucSidBytes = NULL;
    DWORD i = 0;
    
    if (IsNullOrEmptyString(pszSidString))
    {
       dwError = LWPS_ERROR_INVALID_SID;
       BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsAllocateString(
                 pszSidString, 
                 &pszSidCopy);
    BAIL_ON_LWPS_ERROR(dwError);

    dwSidStringLength = strlen(pszSidString);
       
    //expecting to find S-<revision>-<authority>[-<tailA>-<tailB>-...<tailN>]
    //This will find the number of talk tokens, i.e., the number of '-'
    //characters in the bracketed portion above.
    for (i = 0, iTailCount = -2; i < dwSidStringLength; i++)
    {
        if (pszSidCopy[i] == pszDelim[0])
        {
           iTailCount++;
        }
    }
    if (iTailCount <= 0) 
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsAllocateMemory(
                   iTailCount * sizeof(DWORD),
                   (PVOID*)&pdwTail);
    BAIL_ON_LWPS_ERROR(dwError);
    
    pszToken = strtok_r(pszSidCopy, pszDelim, &pszStrTokState);
    
    for (dwTokenCount = 0; 
        pszToken != NULL; 
        pszToken = strtok_r(NULL, pszDelim,  &pszStrTokState), dwTokenCount++)
    {
        PSTR pszEndPtr = NULL;

        switch(dwParseMode)
        {
        case PARSE_MODE_OPEN:
            if (strcmp(pszToken, "S") == 0)
            {
                dwParseMode = PARSE_MODE_REVISION;
            }
            else 
            {
                dwError = LWPS_ERROR_INVALID_SID;
            }
            BAIL_ON_LWPS_ERROR(dwError);
            break;
        case PARSE_MODE_REVISION:

            dwRevision = (DWORD)strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LWPS_ERROR_DATA_ERROR;
               BAIL_ON_LWPS_ERROR(dwError);
            }

            if (dwRevision > 0)
            {
                dwParseMode = PARSE_MODE_AUTHORITY;
            }
            else
            {
                dwError = LWPS_ERROR_INVALID_SID;
            }
            BAIL_ON_LWPS_ERROR(dwError);

            break;

        case PARSE_MODE_AUTHORITY:

            uiAuth = (DWORD) strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LWPS_ERROR_DATA_ERROR;
               BAIL_ON_LWPS_ERROR(dwError);
            }

            dwParseMode = PARSE_MODE_TAIL;
            break;
        case PARSE_MODE_TAIL:

            pdwTail[dwTokenCount - 3] =
                   (DWORD) strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LWPS_ERROR_DATA_ERROR;
               BAIL_ON_LWPS_ERROR(dwError);
            }

            BAIL_ON_LWPS_ERROR(dwError);

            break;

        default:
            dwError = LWPS_ERROR_INVALID_SID;
            BAIL_ON_LWPS_ERROR(dwError);
        }
       
    }
    
    
    //see comments in lwps-sid_p.h
    dwSidBytesLength = 1 + 1 + 6 + 4*(iTailCount);
    
    dwError = LwpsAllocateMemory(
                    dwSidBytesLength * sizeof(UCHAR),
                    (PVOID*)&pucSidBytes);
    BAIL_ON_LWPS_ERROR(dwError);

    pucSidBytes[0] = (UCHAR)dwRevision;
    pucSidBytes[1] = (UCHAR)iTailCount;
    
    uiAuth = LW_ENDIAN_SWAP64(uiAuth);
    
    memcpy((PVOID)(pucSidBytes+2), (PVOID)((UCHAR*)(&uiAuth) + 2), 6);
    
    for (i = 0; i < iTailCount; i++)
    {
        
        #if defined(WORDS_BIGENDIAN)
            pdwTail[i] = LW_ENDIAN_SWAP32(pdwTail[i]);
        #endif
        
        memcpy((PVOID)(pucSidBytes+8+(sizeof(DWORD)*i)), 
               (PVOID)&(pdwTail[i]), 
               sizeof(DWORD));
    }
    
    *ppucSidBytes = pucSidBytes;
    *pdwSidBytesLength = dwSidBytesLength;
    
cleanup:

    LWPS_SAFE_FREE_MEMORY(pszSidCopy);
    LWPS_SAFE_FREE_MEMORY(pdwTail);
    
    return dwError;

error:

    LWPS_SAFE_FREE_MEMORY(pucSidBytes);

    *ppucSidBytes = NULL;
    *pdwSidBytesLength = 0;
   
    goto cleanup;
}

DWORD
LwpsSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD  dwSidBytesLength,
    PSTR*  ppszSidString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszSidString = NULL;

    //revision string is an integer, "0" - "255"
    CHAR pszRevision[16];
    //Auth string is an integer, "0" - "255", usually, "5"
    CHAR pszAuth[32];  
    //This is a count of the number of DWORDs which constitute the combined
    //<domain_computer_id> and trailing <rid>
    DWORD dwWordCount = 0;
    
    //the minimum binary SID length is 8
    if ((pucSidBytes == NULL) ||
        (dwSidBytesLength < 8))
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    
    sprintf((char*)pszRevision, "%u", pucSidBytes[0]);
    dwWordCount = pucSidBytes[1];
    
    //The byte length should be 8 + wordlength*words
    if(dwSidBytesLength != 
        8 + (sizeof(DWORD) * dwWordCount))
    {
        dwError = LWPS_ERROR_INVALID_SID;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    

    if (pucSidBytes[7] != 0 || pucSidBytes[6] != 0)
    {
        DWORD dwFirstIndex = 0;
        CHAR pszAuthTemp[16];
        
        for (i = 0; i < 6; i++)
        {
            sprintf((char*)pszAuthTemp+(2*i), "%.2X", pucSidBytes[2+i]);
        }
        
        for (i = 0; i < 12; i++) 
        {
            if ((dwFirstIndex == 0))
            {
               if ((pszAuthTemp[i] != '0'))
               {
                  dwFirstIndex = i;
               }
            }
        }
        
        sprintf((char*)pszAuth, "%s", pszAuthTemp+dwFirstIndex);
    }
    else
    {
        DWORD* pdwAuthTempWrongEndian = (DWORD*)(pucSidBytes+2);
        sprintf((char*)pszAuth, "%u", *pdwAuthTempWrongEndian);
    }
    
    
    dwError = LwpsBuildSIDString(
                    pszRevision,
                    pszAuth,
                    pucSidBytes,
                    dwWordCount,
                    &pszSidString);
    BAIL_ON_LWPS_ERROR(dwError);
    
    
    *ppszSidString = pszSidString;
    
cleanup:
    
    return dwError;

error:
    
    LWPS_SAFE_FREE_STRING(pszSidString);
    *ppszSidString = NULL;
    
    goto cleanup;
}

DWORD
LwpsBuildSIDString(
    PCSTR pszRevision,
    PCSTR pszAuth,
    PBYTE pucSidBytes,
    DWORD dwWordCount,
    PSTR* ppszSidString
    )
{
    DWORD dwError = 0;
    PSTR  pszSidString = NULL;
    DWORD dwCurLen = 0;
    DWORD dwCurOffset = 0;
    PSTR  pszSidPart = NULL;
    DWORD dwSidStringMemory = 64;  //initial memory amount
    DWORD dwMemoryUsed = 0;
    DWORD i = 0;
    
    //allow this many characters for each DWORD
    DWORD dwDecimalCharsForDWORD = 10;
    
    //add the component strings, dashes, wordcound * max wordsize, plus padding.
    dwSidStringMemory = dwSidStringMemory +
                        strlen(pszRevision) + 
                        strlen(pszAuth) + 
                        dwWordCount + 2 + //for the dashes
                        (dwWordCount * dwDecimalCharsForDWORD);
    
    dwError = LwpsAllocateMemory(
                dwSidStringMemory, 
                (PVOID*)&pszSidString);
    BAIL_ON_LWPS_ERROR(dwError); 
    
    
    dwError = LwpsAllocateStringPrintf(
                   &pszSidPart,
                   "S-%s-%s",
                   pszRevision,
                   pszAuth);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwCurLen = strlen(pszSidPart);
    dwCurOffset = 0;
    
    memcpy(pszSidString+dwCurOffset, pszSidPart, dwCurLen);
    dwCurOffset += dwCurLen;
    dwMemoryUsed += dwCurLen;
    
    LWPS_SAFE_FREE_STRING(pszSidPart);

    for (i = 0; i < dwWordCount; i++)
    {
        DWORD* pdwTempWrongEndian = (DWORD*)(pucSidBytes+8+(i*4));
        

        #if defined(WORDS_BIGENDIAN)
            *pdwTempWrongEndian = LW_ENDIAN_SWAP32(*pdwTempWrongEndian);     
        #endif
                
        dwError = LwpsAllocateStringPrintf(
                         &pszSidPart, "-%u",
                         *pdwTempWrongEndian);
        BAIL_ON_LWPS_ERROR(dwError);
        
        dwCurLen = strlen(pszSidPart);
        
        if(dwMemoryUsed + dwCurLen > dwSidStringMemory)
        {
            dwSidStringMemory = 2*(dwMemoryUsed + dwCurLen);
            
            dwError = LwpsReallocMemory(
                (PVOID)pszSidString,
                (PVOID*)&pszSidString,
                dwSidStringMemory);
            BAIL_ON_LWPS_ERROR(dwError); 
        }
        
        memcpy(pszSidString+dwCurOffset, pszSidPart, dwCurLen);
        dwCurOffset += dwCurLen;
        dwMemoryUsed += dwCurLen;
        
        LWPS_SAFE_FREE_STRING(pszSidPart);

    }
    
    *ppszSidString = pszSidString;
    
cleanup:

    return dwError;
    
error:

    LWPS_SAFE_FREE_STRING(pszSidString);
    *ppszSidString = NULL;
    
    goto cleanup;
}


