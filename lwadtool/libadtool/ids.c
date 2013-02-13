/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        ids.c
 *
 * Abstract:
 *        Methods fo ID manipulations.
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 7, 2010
 *
 */

#include "includes.h"
#include "config.h"

/**
 * Covert object GUID to hex string.
 *
 * @param s Bytes to convert.
 * @param out Converted string.
 * @return 0 on success; error code on failure.
 */
DWORD Guid2Str(IN PVOID s, OUT PSTR *out)
{
    DWORD dwError = 0;
    INT i = 0, j;
    PUCHAR b = (PUCHAR) s;

    dwError = LwAllocateMemory(37 * sizeof(char), OUT_PPVOID(out));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    for(j = 3; j >= 0; --j, i += 2) {
        Char2Hex(b[j], *out + i);
    }

    *(*out + i) = '-'; ++i;

    for(j = 5; j >= 4; --j, i += 2) {
        Char2Hex(b[j], *out + i);
    }

    *(*out + i) = '-'; ++i;

    for(j = 7; j >= 6; --j, i += 2) {
        Char2Hex(b[j], *out + i);
    }

    *(*out + i) = '-'; ++i;

    for(j = 8; j <= 9; ++j, i += 2) {
        Char2Hex(b[j], *out + i);
    }

    *(*out + i) = '-'; ++i;

    for(j = 10; j <= 15; ++j, i += 2) {
        Char2Hex(b[j], *out + i);
    }

    cleanup:
       return dwError;

    error:
       LW_SAFE_FREE_MEMORY(*out);
       goto cleanup;
}

/**
 * Convert char to hex.
 *
 * @param in Char to convert.
 * @param s Preallocated 2 chars array.
 */
VOID Char2Hex(IN UCHAR in, OUT PSTR s)
{
    UCHAR pseudo[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    UCHAR ch;

    ch = in >> 4;
    s[0] = pseudo[(int) ch];

    ch = in & 0x0F;
    s[1] = pseudo[(int) ch];
}

/**
 * Covert AD object's SID to an ASCII string.
 *
 * @param s Bytes to convert.
 * @param out Converted string.
 * @return 0 on success; error code on failure.
 */
DWORD Sid2Str(IN PVOID s, OUT PSTR *out)
{
    DWORD dwError = 0;
    size_t size = 0;
    INT count = 0;
    ULONG i = 0;
    SidTP sid = (SidTP) s;

    *out = NULL;

    size = 2 + 3 + 1 + 14 + (1 + 10) * sid->SubAuthorityCount + 1;

    dwError = LwAllocateMemory(size * sizeof(char), OUT_PPVOID(out));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    if (sid->IdentifierAuthority[0] || sid->IdentifierAuthority[1]) {
        count += snprintf(*out + count, size - count,
                          "S-%u-0x%.2X%.2X%.2X%.2X%.2X%.2X", sid->Revision,
                          sid->IdentifierAuthority[0],
                          sid->IdentifierAuthority[1],
                          sid->IdentifierAuthority[2],
                          sid->IdentifierAuthority[3],
                          sid->IdentifierAuthority[4],
                          sid->IdentifierAuthority[5]);
    }
    else {
        ULONG value = 0;

        value |= (ULONG) sid->IdentifierAuthority[5];
        value |= (ULONG) sid->IdentifierAuthority[4] << 8;
        value |= (ULONG) sid->IdentifierAuthority[3] << 16;
        value |= (ULONG) sid->IdentifierAuthority[2] << 24;

        count += snprintf(*out  + count, size - count, "S-%u-%u",
                          sid->Revision, value);
    }

    for (i = 0; i < sid->SubAuthorityCount; i++) {
#if defined(WORDS_BIGENDIAN)
        sid->SubAuthority[i] = LW_ENDIAN_SWAP32(sid->SubAuthority[i]);
#endif
        count += snprintf(*out  + count, size - count, "-%u", sid->SubAuthority[i]);
    }

    cleanup:
       return dwError;

    error:
       LW_SAFE_FREE_MEMORY(*out);
       goto cleanup;
}

/**
 * Generate UID from SID.
 *
 * @param s SID bytes.
 * @param out UID (dynamically allocated).
 * @return 0 on success; error code on failure.
 */
DWORD Sid2Id(IN PVOID s, OUT PDWORD out)
{
    DWORD dwError = 0;
    size_t size = 0;
    SidTP sid = (SidTP) s;
    PDWORD subs = NULL;

    *out = 0;

    if (sid->Revision != 1)
    {
        dwError = ADT_ERR_INVALID_SID;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    size = sid->SubAuthorityCount * sizeof(DWORD);

    dwError = LwAllocateMemory(size, OUT_PPVOID(&subs));
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    memcpy((PVOID)subs, (PVOID) sid->SubAuthority, size);

#if defined(WORDS_BIGENDIAN)
    INT i;

    for (i = 0; i < sid->SubAuthorityCount; i++)
    {
        subs[i] = LW_ENDIAN_SWAP32(subs[i]);
    }
#endif

    LwUidHashCalc(subs, sid->SubAuthorityCount, out);

    cleanup:
        LW_SAFE_FREE_MEMORY(subs);
       return dwError;

    error:
       goto cleanup;
}
