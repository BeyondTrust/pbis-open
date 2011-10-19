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
 *        lsasecurityidentifier.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaAllocSecurityIdentifierFromBinary(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    )
{
    DWORD dwError = 0;

    PLSA_SECURITY_IDENTIFIER pSID = NULL;

    dwError = LwAllocateMemory(
                   sizeof(LSA_SECURITY_IDENTIFIER),
                  (PVOID*)&pSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  dwSidBytesLength * sizeof(UCHAR),
                  (PVOID*)&(pSID->pucSidBytes));
    BAIL_ON_LSA_ERROR(dwError);

    pSID->dwByteLength = dwSidBytesLength;

    memcpy(pSID->pucSidBytes, pucSidBytes, dwSidBytesLength);

    *ppSecurityIdentifier = pSID;

cleanup:

    return dwError;

error:

    if (pSID)
    {
       LsaFreeSecurityIdentifier(pSID);
    }
    *ppSecurityIdentifier = NULL;

    goto cleanup;
}

DWORD
LsaAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_IDENTIFIER pSID = NULL;

    dwError = LwAllocateMemory(
                   sizeof(LSA_SECURITY_IDENTIFIER),
                   (PVOID*)&pSID);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LsaSidStringToBytes(
                    pszSidString,
                    &(pSID->pucSidBytes),
                    &(pSID->dwByteLength));
    BAIL_ON_LSA_ERROR(dwError);

    *ppSecurityIdentifier = pSID;

cleanup:

    return dwError;

error:

    if (pSID) {
       LsaFreeSecurityIdentifier(pSID);
    }
    *ppSecurityIdentifier = NULL;

    goto cleanup;
}

VOID
LsaFreeSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier
    )
{

    LW_SAFE_FREE_MEMORY(pSecurityIdentifier->pucSidBytes);

    LwFreeMemory(pSecurityIdentifier);
}

DWORD
LsaGetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
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
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;

    //verify the SID is version 1.
    if(pucSidBytes[0] != 1)
    {
        dwError = LW_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //verify the number of bytes is plausible
    if((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&dwRid, pucSidBytes+dwByteLength-sizeof(DWORD), sizeof(dwRid));

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
LsaSetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
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
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;

    //verify the SID is version 1.
    if(pucSidBytes[0] != 1)
    {
        dwError = LW_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //verify the number of bytes is plausible
    if((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

#if defined(WORDS_BIGENDIAN)
    dwRidLocal = LW_ENDIAN_SWAP32(dwRidLocal);
#endif

    memcpy(pucSidBytes+dwByteLength-sizeof(DWORD), &dwRidLocal, sizeof(DWORD));


error:

    return dwError;
}

DWORD
LsaReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_IDENTIFIER pSid = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pSid),
                    (PVOID*)&pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSidStringToBytes(
                    pszSid,
                    &pSid->pucSidBytes,
                    &pSid->dwByteLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSetSecurityIdentifierRid(pSid, dwNewRid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierString(pSid, ppszNewSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pSid)
    {
        LsaFreeSecurityIdentifier(pSid);
    }

    return dwError;

error:
    *ppszNewSid = NULL;

    goto cleanup;
}

DWORD
LsaGetSecurityIdentifierHashedRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    )
{
    return LsaHashSecurityIdentifierToId(pSecurityIdentifier, dwHashedRid);
}

// The UID is a DWORD constructued using a non-cryptographic hash
// of the User's domain SID and user RID.
DWORD
LsaHashSecurityIdentifierToId(
    IN PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    )
{
    DWORD dwError = 0;
    // dwAuthorityCount includes the final RID.
    DWORD dwAuthorityCount = 0;
    PDWORD pdwAuthorities = NULL;
    DWORD dwHash = 0;
    UCHAR* pucSidBytes = NULL;
    DWORD dwByteLength = 0;
#if defined(WORDS_BIGENDIAN)
    DWORD i = 0;
#endif

    if (!pSecurityIdentifier ||
        !pSecurityIdentifier->pucSidBytes ||
        pSecurityIdentifier->dwByteLength < SECURITY_IDENTIFIER_MINIMUM_SIZE)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pucSidBytes = pSecurityIdentifier->pucSidBytes;
    dwByteLength = pSecurityIdentifier->dwByteLength;

    // Verify that the SID is version 1.
    if (pucSidBytes[0] != 1)
    {
        dwError = LW_ERROR_INVALID_SID_REVISION;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Verify that the number of bytes is plausible.
    if ((dwByteLength - SECURITY_IDENTIFIER_MINIMUM_SIZE) % sizeof(DWORD) != 0)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwAuthorityCount =
        ((dwByteLength - SECURITY_IDENTIFIER_HEADER_SIZE) /
        sizeof(DWORD));

    dwError = LwAllocateMemory(
              dwAuthorityCount * sizeof(DWORD),
              (PVOID*)&pdwAuthorities);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy((PVOID)pdwAuthorities,
           (PVOID)(pucSidBytes + SECURITY_IDENTIFIER_HEADER_SIZE),
           dwByteLength - SECURITY_IDENTIFIER_HEADER_SIZE);

#if defined(WORDS_BIGENDIAN)
    for (i = 0; i < dwAuthorityCount; i++)
    {
        pdwAuthorities[i] = LW_ENDIAN_SWAP32(pdwAuthorities[i]);
    }
#endif

    LsaUidHashCalc(pdwAuthorities, dwAuthorityCount, &dwHash);

    *pdwId = dwHash;

cleanup:
    LW_SAFE_FREE_MEMORY(pdwAuthorities);

    return dwError;

error:
    *pdwId = 0;

    goto cleanup;
}

DWORD
LsaHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;
    LSA_SECURITY_IDENTIFIER sid = { 0 };

    dwError = LsaSidStringToBytes(
                    pszSidString,
                    &sid.pucSidBytes,
                    &sid.dwByteLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSecurityIdentifierToId(&sid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwId = dwId;

cleanup:
    LW_SAFE_FREE_MEMORY(sid.pucSidBytes);
    return dwError;

error:
    *pdwId = 0;
    goto cleanup;
}

DWORD
LsaGetSecurityIdentifierBinary(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    )
{
    DWORD dwError = 0;
    UCHAR* pucSidBytes = NULL;

    if (pSecurityIdentifier->dwByteLength <= 0 ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    pSecurityIdentifier->dwByteLength * sizeof(UCHAR),
                    (PVOID*)&pucSidBytes);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pucSidBytes,
           pSecurityIdentifier->pucSidBytes,
           pSecurityIdentifier->dwByteLength);

    *ppucSidBytes = pucSidBytes;
    *pdwSidBytesLength = pSecurityIdentifier->dwByteLength;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pucSidBytes);

    *ppucSidBytes = NULL;
    *pdwSidBytesLength = 0;

    goto cleanup;
}

DWORD
LsaGetSecurityIdentifierString(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    )
{
    DWORD dwError = 0;
    PSTR pszSidStr = NULL;

    if (pSecurityIdentifier->dwByteLength < 8 ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
       dwError = LW_ERROR_INVALID_SID;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSidBytesToString(pSecurityIdentifier->pucSidBytes,
            pSecurityIdentifier->dwByteLength, &(pszSidStr));
    BAIL_ON_LSA_ERROR(dwError);


    *ppszSidStr = pszSidStr;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSidStr);

    ppszSidStr = NULL;

    goto cleanup;
}

DWORD
LsaGetDomainSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLSA_SECURITY_IDENTIFIER* ppDomainSID
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_IDENTIFIER pDomainSID = NULL;
    UCHAR* pucDomainSID = NULL;
    DWORD dwDomainSIDByteLength = 0;

    if ((pSecurityIdentifier->dwByteLength <=
         SECURITY_IDENTIFIER_MINIMUM_SIZE + sizeof(DWORD)) ||
        pSecurityIdentifier->pucSidBytes == NULL)
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwDomainSIDByteLength = pSecurityIdentifier->dwByteLength - sizeof(DWORD);
    dwError = LwAllocateMemory(
                dwDomainSIDByteLength,
                (PVOID*)&pucDomainSID);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pucDomainSID,
           pSecurityIdentifier->pucSidBytes,
           dwDomainSIDByteLength);

    pucDomainSID[1]--; //decrement word count

    dwError = LsaAllocSecurityIdentifierFromBinary(
                pucDomainSID,
                dwDomainSIDByteLength,
                &pDomainSID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDomainSID = pDomainSID;

cleanup:

    LW_SAFE_FREE_MEMORY(pucDomainSID);

    return dwError;

error:

    if (pDomainSID)
    {
        LsaFreeSecurityIdentifier(pDomainSID);
    }

    *ppDomainSID = NULL;

    goto cleanup;
}

DWORD
LsaHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = 0;
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);

    if (*pdwHexStringLength)
    {
        dwHexChars = *pdwHexStringLength;
    }
    else
    {
        dwHexChars = strlen(pszHexString);
    }
    dwByteArrayLength = dwHexChars / 2;

    if ((dwHexChars & 0x00000001) != 0)
    {
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                  sizeof(UCHAR)*(dwByteArrayLength),
                  (PVOID*)&pucByteArray
                  );
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];

        UCHAR ucHi = 0;
        UCHAR ucLow = 0;

        dwError = LsaHexCharToByte(hexHi, &ucHi);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHexCharToByte(hexLow, &ucLow);
        BAIL_ON_LSA_ERROR(dwError);

        pucByteArray[i] = (ucHi * 16) + ucLow;
    }

    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;

    goto cleanup;
}

DWORD
LsaByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;

    dwError = LwAllocateMemory(
                (dwByteArrayLength*2 + 1) * sizeof(CHAR),
                (PVOID*)&pszHexString);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf(pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }

    *ppszHexString = pszHexString;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHexString);

    *ppszHexString = NULL;
    goto cleanup;
}

DWORD
LsaByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;

    dwError = LwAllocateMemory(
                (dwByteArrayLength*3 + 1) * sizeof(CHAR),
                (PVOID*)&pszHexString);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf(pszHexString+(3*i), "\\%.2X", pucByteArray[i]);
    }

    *ppszHexString = pszHexString;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHexString);

    *ppszHexString = NULL;
    goto cleanup;
}

DWORD
LsaSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    )
{
    DWORD dwError = 0;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PSTR pszHexSid = NULL;

    dwError = LsaSidStringToBytes(
                  pszSid,
                  &pucSIDBytes,
                  &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaByteArrayToLdapFormatHexStr(
                pucSIDBytes,
                dwSIDByteLength,
                &pszHexSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHexSid = pszHexSid;

cleanup:
    LW_SAFE_FREE_MEMORY(pucSIDBytes);

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHexSid);
    *ppszHexSid = NULL;

    goto cleanup;
}


DWORD
LsaHexCharToByte(
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
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    *pucByte = ucByte;

cleanup:

    return dwError;

error:

    *pucByte = 0;

    goto cleanup;
}

void
LsaUidHashCalc(
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
LsaSidStringToBytes(
    IN PCSTR pszSidString,
    OUT UCHAR** ppucSidBytes,
    OUT DWORD* pdwSidBytesLength
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

    if (LW_IS_NULL_OR_EMPTY_STR(pszSidString))
    {
       dwError = LW_ERROR_INVALID_SID;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(
                 pszSidString,
                 &pszSidCopy);
    BAIL_ON_LSA_ERROR(dwError);

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
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                   iTailCount * sizeof(DWORD),
                   (PVOID*)&pdwTail);
    BAIL_ON_LSA_ERROR(dwError);

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
                dwError = LW_ERROR_INVALID_SID;
            }
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case PARSE_MODE_REVISION:
            dwRevision = (DWORD) strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LW_ERROR_DATA_ERROR;
               BAIL_ON_LSA_ERROR(dwError);
            }

            if (dwRevision > 0)
            {
                dwParseMode = PARSE_MODE_AUTHORITY;
            }
            else
            {
                dwError = LW_ERROR_INVALID_SID;
            }
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case PARSE_MODE_AUTHORITY:
            uiAuth = (DWORD) strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LW_ERROR_DATA_ERROR;
               BAIL_ON_LSA_ERROR(dwError);
            }

            dwParseMode = PARSE_MODE_TAIL;
            break;
        case PARSE_MODE_TAIL:
            pdwTail[dwTokenCount - 3] =
                   (DWORD) strtoll(pszToken, &pszEndPtr, 10);

            if (!pszEndPtr || (pszEndPtr == pszToken) || *pszEndPtr) {
               dwError = LW_ERROR_DATA_ERROR;
               BAIL_ON_LSA_ERROR(dwError);
            }

            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INVALID_SID;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    //see comments in lsasecurityidentifier_p.h
    dwSidBytesLength = 1 + 1 + 6 + 4*(iTailCount);

    dwError = LwAllocateMemory(
                    dwSidBytesLength * sizeof(UCHAR),
                    (PVOID*)&pucSidBytes);
    BAIL_ON_LSA_ERROR(dwError);

    pucSidBytes[0] = (UCHAR)dwRevision;
    pucSidBytes[1] = (UCHAR)iTailCount;

#if !defined(WORDS_BIGENDIAN)
    uiAuth = LW_ENDIAN_SWAP64(uiAuth);
#endif

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

    LW_SAFE_FREE_MEMORY(pszSidCopy);
    LW_SAFE_FREE_MEMORY(pdwTail);

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pucSidBytes);

    *ppucSidBytes = NULL;
    *pdwSidBytesLength = 0;

    goto cleanup;
}

DWORD
LsaSidBytesToString(
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
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    sprintf(pszRevision, "%u", pucSidBytes[0]);
    dwWordCount = pucSidBytes[1];

    //The byte length should be 8 + wordlength*words
    if(dwSidBytesLength !=
        8 + (sizeof(DWORD) * dwWordCount))
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }


    if (pucSidBytes[2] != 0 || pucSidBytes[3] != 0)
    {
        CHAR pszAuthTemp[16];

        for (i = 0; i < 6; i++)
        {
            sprintf(pszAuthTemp+(2*i), "%.2X", (unsigned int) pucSidBytes[2+i]);
        }

        sprintf(pszAuth, "0x%s", pszAuthTemp);
    }
    else
    {
        DWORD dwAuth =
            (pucSidBytes[4] << 24) |
            (pucSidBytes[5] << 16) |
            (pucSidBytes[6] << 8)  |
            (pucSidBytes[7] << 0);

        sprintf(pszAuth, "%u", dwAuth);
    }


    dwError = LsaBuildSIDString(
                    pszRevision,
                    pszAuth,
                    pucSidBytes,
                    dwWordCount,
                    &pszSidString);
    BAIL_ON_LSA_ERROR(dwError);


    *ppszSidString = pszSidString;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSidString);
    *ppszSidString = NULL;

    goto cleanup;
}

DWORD
LsaBuildSIDString(
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

    dwError = LwAllocateMemory(
                dwSidStringMemory,
                (PVOID*)&pszSidString);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LwAllocateStringPrintf(
                   &pszSidPart,
                   "S-%s-%s",
                   pszRevision,
                   pszAuth);
    BAIL_ON_LSA_ERROR(dwError);

    dwCurLen = strlen(pszSidPart);
    dwCurOffset = 0;

    memcpy(pszSidString+dwCurOffset, pszSidPart, dwCurLen);
    dwCurOffset += dwCurLen;
    dwMemoryUsed += dwCurLen;

    LW_SAFE_FREE_STRING(pszSidPart);

    for (i = 0; i < dwWordCount; i++)
    {
        DWORD dwTempWrongEndian = 0;
        memcpy(&dwTempWrongEndian, pucSidBytes+8+(i*4), sizeof(dwTempWrongEndian));

#if defined(WORDS_BIGENDIAN)
        dwTempWrongEndian = LW_ENDIAN_SWAP32(dwTempWrongEndian);
#endif

        dwError = LwAllocateStringPrintf(
                         &pszSidPart, "-%u",
                         dwTempWrongEndian);
        BAIL_ON_LSA_ERROR(dwError);

        dwCurLen = strlen(pszSidPart);

        if(dwMemoryUsed + dwCurLen > dwSidStringMemory)
        {
            dwSidStringMemory = 2*(dwMemoryUsed + dwCurLen);

            dwError = LwReallocMemory(
                (PVOID)pszSidString,
                (PVOID*)&pszSidString,
                dwSidStringMemory);
            BAIL_ON_LSA_ERROR(dwError);
        }

        memcpy(pszSidString+dwCurOffset, pszSidPart, dwCurLen);
        dwCurOffset += dwCurLen;
        dwMemoryUsed += dwCurLen;

        LW_SAFE_FREE_STRING(pszSidPart);

    }

    *ppszSidString = pszSidString;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszSidString);
    *ppszSidString = NULL;

    goto cleanup;
}


