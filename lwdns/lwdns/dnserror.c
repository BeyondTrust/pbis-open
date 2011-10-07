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

#include "includes.h"

static
size_t
DNSMapDNSErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

static
size_t
DNSGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

static
size_t
DNSGetUnmappedErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

static
const char* gDNSErrorMessages[] =
{
     // LWDNS_ERROR_INIT_FAILED         : 57344
     "Failed to initialize DNS library",
     // LWDNS_ERROR_RECORD_NOT_FOUND    : 57345
     "Failed to find the DNS record",
     // LWDNS_ERROR_BAD_RESPONSE        : 57346
     "Received bad response from DNS server",
     // LWDNS_ERROR_PASSWORD_EXPIRED    : 57347
     "Failed authentication because password expired",
     // LWDNS_ERROR_PASSWORD_MISMATCH   : 57348
     "Failed authentication due to bad password",
     // LWDNS_ERROR_CLOCK_SKEW          : 57349
     "Failed authentication due to clock skew",
     // LWDNS_ERROR_KRB5_NO_KEYS_FOUND  : 57350
     "Failed authentication because no kerberos keys are available",
     // LWDNS_ERROR_KRB5_CALL_FAILED    : 57351
     "Kerberos call failed",
     // LWDNS_ERROR_RCODE_UNKNOWN       : 57352
     "Found an unmapped DNS RCode",
     // LWDNS_ERROR_RCODE_FORMERR       : 57353
     "The name server was unable to interpret the request due to a format error [DNS RCODE:FORMERR:1]",
     // LWDNS_ERROR_RCODE_SERVFAIL      : 57354
     "The name server encountered an internal failure [DNS RCODE:SERVFAIL:2]",
     // LWDNS_ERROR_RCODE_NXDOMAIN      : 57355
     "A name in the DNS request is invalid or does not exist [DNS RCODE:NXDOMAIN:3]",
     // LWDNS_ERROR_RCODE_NOTIMP        : 57356
     "The name server does not support the specified opcode [DNS RCODE:NOTIMP:4]",
     // LWDNS_ERROR_RCODE_REFUSED       : 57357
     "The name server refused the request due to policy or security reasons [DNS RCODE:REFUSED:5]",
     // LWDNS_ERROR_RCODE_YXDOMAIN      : 57358
     "A domain name in the DNS request is invalid or does not exist [DNS RCODE:YXDOMAIN:6]",
     // LWDNS_ERROR_RCODE_YXRRSET       : 57359
     "An RRSet in the DNS request is invalid or does not exist [DNS RCODE:YXRRSET:7]",
     // LWDNS_ERROR_RCODE_NXRRSET       : 57360
     "An RRSet in the DNS request is invalid or does not exist [DNS RCODE:NXRRSET:8]",
     // LWDNS_ERROR_RCODE_NOTAUTH       : 57361
     "The name server is not authoritative for the named zone [DNS RCODE:NOTAUTH:9]",
     // LWDNS_ERROR_RCODE_NOTZONE       : 57362
     "The name used in the pre-requisite section of the DNS request is not within the zone in the zone section [DNS RCODE:NOTZONE:10]",
     // LWDNS_ERROR_NO_NAMESERVER       : 57363
     "No name servers could be found for this domain",
     // LWDNS_ERROR_NO_SUCH_ZONE        : 57364
     "The zone for the specified domain could not be located",
     // LWDNS_ERROR_NO_RESPONSE         : 57365
     "No response was obtained for the DNS query",
     // LWDNS_ERROR_UNEXPECTED          : 57366
     "An unexpected error was encountered when performing DNS query",
     // LWDNS_ERROR_NO_SUCH_ADDRESS     : 57367
     "Failed to find IP address for host name",
     // LWDNS_ERROR_UPDATE_FAILED       : 57368
     "Failed to update DNS",
     // LWDNS_ERROR_NO_INTERFACES       : 57369
     "No suitable network interfaces could be found, whose address can be configured in DNS",
     // LWDNS_ERROR_INVALID_IP_ADDRESS  : 57370
     "An invalid IP Address was specified",
     // LWDNS_ERROR_STRING_CONV_FAILED  : 57371
     "Failed to convert string format (wide/ansi)"
};


DWORD
DNSMapRCode(
    DWORD dwRCode
    )
{
    DWORD dwError = 0;

    switch (dwRCode)
    {
        case 0:

            dwError = LWDNS_ERROR_SUCCESS;
            break;

        case 1:

            dwError = LWDNS_ERROR_RCODE_FORMERR;
            break;

        case 2:

            dwError = LWDNS_ERROR_RCODE_SERVFAIL;
            break;

        case 3:

            dwError = LWDNS_ERROR_RCODE_NXDOMAIN;
            break;

        case 4:

            dwError = LWDNS_ERROR_RCODE_NOTIMP;
            break;

        case 5:

            dwError = LWDNS_ERROR_RCODE_REFUSED;
            break;

        case 6:

            dwError = LWDNS_ERROR_RCODE_YXDOMAIN;
            break;

        case 7:

            dwError = LWDNS_ERROR_RCODE_YXRRSET;
            break;

        case 8:

            dwError = LWDNS_ERROR_RCODE_NXRRSET;
            break;

        case 9:

            dwError = LWDNS_ERROR_RCODE_NOTAUTH;
            break;

        case 10:

            dwError = LWDNS_ERROR_RCODE_NOTZONE;
            break;

        default:

            dwError = LWDNS_ERROR_RCODE_UNKNOWN;
            break;

    }

    return dwError;
}

size_t
DNSGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    if (!dwError)
    {
        // No error string for success
        goto cleanup;
    }

    if (LWDNS_ERROR_MASK(dwError) != 0)
    {
        stResult = DNSMapDNSErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = DNSGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }

cleanup:

    return stResult;
}

static
size_t
DNSMapDNSErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gDNSErrorMessages)/sizeof(PCSTR);

    if ((dwError >= LWDNS_ERROR_INIT_FAILED) &&
        (dwError < LWDNS_ERROR_SENTINEL))
    {
        DWORD dwErrorOffset = dwError - 0xE000;

        if (dwErrorOffset < dwNMessages)
        {
            PCSTR pszMessage = gDNSErrorMessages[dwErrorOffset];
            DWORD dwRequiredLen = strlen(pszMessage) + 1;

            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }

            stResult = dwRequiredLen;

            goto cleanup;
        }
    }

    stResult = DNSGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

static
size_t
DNSGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = LWDNS_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = DNSStrError(dwConvertError, pszBuffer, stBufSize);
    if (result == EINVAL)
    {
        stResult = DNSGetUnmappedErrorString(
                        dwConvertError,
                        pszBuffer,
                        stBufSize);
        goto cleanup;
    }

    while (result != 0)
    {
        if (result == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = DNSGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        LWDNS_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = DNSAllocateMemory(
                        stBufSize,
                        (PVOID*)&pszTempBuffer);
        BAIL_ON_LWDNS_ERROR(dwError);

        result = DNSStrError(dwConvertError, pszTempBuffer, stBufSize);
    }

    if (pszTempBuffer != NULL)
    {
        stResult = strlen(pszTempBuffer) + 1;
    }
    else
    {
        stResult = strlen(pszBuffer) + 1;
    }

cleanup:

    LWDNS_SAFE_FREE_MEMORY(pszTempBuffer);

    return stResult;

error:

    stResult = 0;

    goto cleanup;
}

static
size_t
DNSGetUnmappedErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    CHAR  szBuf[128] = "";
    DWORD dwRequiredLen = 0;

    dwRequiredLen = sprintf(szBuf, "Error [code=%d] occurred.", dwError) + 1;

    if (stBufSize >= dwRequiredLen) {
        memcpy(pszBuffer, szBuf, dwRequiredLen);
    }

    stResult = dwRequiredLen;

    return stResult;
}

DWORD
DNSMapHerrno(
    DWORD dwHerrno
    )
{
    switch (dwHerrno)
    {
        case HOST_NOT_FOUND:
            return LWDNS_ERROR_RECORD_NOT_FOUND;
        case NO_ADDRESS:
#if NO_ADDRESS != NO_DATA
        case NO_DATA:
#endif
            return LWDNS_ERROR_NO_SUCH_ADDRESS;
        case NO_RECOVERY:
            return LWDNS_ERROR_RCODE_SERVFAIL;
        case TRY_AGAIN:
            return EAGAIN;
    }
    return dwHerrno;
}
