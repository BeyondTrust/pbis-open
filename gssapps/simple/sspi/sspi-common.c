/*
 * Copyright 1994 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "sspi-common.h"

typedef VOID (*PFN_DUMP)(PVOID, DWORD);

static
VOID
DumpNtlmFlags(
    DWORD dwFlags
    );

static
VOID
DumpNegegotiateMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

static
VOID
DumpChallengeMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

static
VOID
DumpResponseMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

static
DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesWritten
    );

static
DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesRead
    );

PCSTR
GetBasename(
    IN PCSTR pszPath
    )
{
    PCSTR pszLastSeparator = pszPath;
    PCSTR pszCurrent = NULL;

    for (pszCurrent = pszPath; pszCurrent[0]; pszCurrent++)
    {
        if (('/' == pszCurrent[0]) || ('\\' == pszCurrent[0]))
        {
            pszLastSeparator = pszCurrent;
        }
    }

    return (pszLastSeparator[0] && pszLastSeparator[1]) ? pszLastSeparator + 1 : pszPath;
}

VOID
DumpIscReqFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    DUMP_FLAG(dwRemainder, ISC_REQ_DELEGATE);
    DUMP_FLAG(dwRemainder, ISC_REQ_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ISC_REQ_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ISC_REQ_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ISC_REQ_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ISC_REQ_PROMPT_FOR_CREDS);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_SUPPLIED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_REQ_ALLOCATE_MEMORY);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ISC_REQ_DATAGRAM);
    DUMP_FLAG(dwRemainder, ISC_REQ_CONNECTION);
    DUMP_FLAG(dwRemainder, ISC_REQ_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ISC_REQ_FRAGMENT_SUPPLIED);
    DUMP_FLAG(dwRemainder, ISC_REQ_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ISC_REQ_STREAM);
    DUMP_FLAG(dwRemainder, ISC_REQ_INTEGRITY);
    DUMP_FLAG(dwRemainder, ISC_REQ_IDENTIFY);
    DUMP_FLAG(dwRemainder, ISC_REQ_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ISC_REQ_MANUAL_CRED_VALIDATION);
    DUMP_FLAG(dwRemainder, ISC_REQ_RESERVED1);
    DUMP_FLAG(dwRemainder, ISC_REQ_FRAGMENT_TO_FIT);
#ifdef ISC_REQ_FORWARD_CREDENTIALS
    DUMP_FLAG(dwRemainder, ISC_REQ_FORWARD_CREDENTIALS);
#endif
#ifdef ISC_REQ_NO_INTEGRITY
    DUMP_FLAG(dwRemainder, ISC_REQ_NO_INTEGRITY);
#endif
#ifdef ISC_REQ_USE_HTTP_STYLE
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_HTTP_STYLE);
#endif

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

VOID
DumpIscRetFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    DUMP_FLAG(dwRemainder, ISC_RET_DELEGATE);
    DUMP_FLAG(dwRemainder, ISC_RET_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ISC_RET_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ISC_RET_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ISC_RET_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ISC_RET_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_COLLECTED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_SUPPLIED_CREDS);
    DUMP_FLAG(dwRemainder, ISC_RET_ALLOCATED_MEMORY);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ISC_RET_DATAGRAM);
    DUMP_FLAG(dwRemainder, ISC_RET_CONNECTION);
    DUMP_FLAG(dwRemainder, ISC_RET_INTERMEDIATE_RETURN);
    DUMP_FLAG(dwRemainder, ISC_RET_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ISC_RET_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ISC_RET_STREAM);
    DUMP_FLAG(dwRemainder, ISC_RET_INTEGRITY);
    DUMP_FLAG(dwRemainder, ISC_RET_IDENTIFY);
    DUMP_FLAG(dwRemainder, ISC_RET_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ISC_RET_MANUAL_CRED_VALIDATION);
    DUMP_FLAG(dwRemainder, ISC_RET_RESERVED1);
    DUMP_FLAG(dwRemainder, ISC_RET_FRAGMENT_ONLY);
#ifdef ISC_RET_FORWARD_CREDENTIALS
    DUMP_FLAG(dwRemainder, ISC_RET_FORWARD_CREDENTIALS);
#endif
#ifdef  ISC_RET_USED_HTTP_STYLE
    DUMP_FLAG(dwRemainder, ISC_RET_USED_HTTP_STYLE);
#endif
#ifdef ISC_RET_NO_ADDITIONAL_TOKEN
    DUMP_FLAG(dwRemainder, ISC_RET_NO_ADDITIONAL_TOKEN);
#endif
#ifdef ISC_RET_REAUTHENTICATION
    DUMP_FLAG(dwRemainder, ISC_RET_REAUTHENTICATION);
#endif

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

VOID
DumpAscReqFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    DUMP_FLAG(dwRemainder, ASC_REQ_DELEGATE);
    DUMP_FLAG(dwRemainder, ASC_REQ_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ASC_REQ_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ASC_REQ_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ASC_REQ_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ASC_REQ_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOCATE_MEMORY);
    DUMP_FLAG(dwRemainder, ASC_REQ_USE_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ASC_REQ_DATAGRAM);
    DUMP_FLAG(dwRemainder, ASC_REQ_CONNECTION);
    DUMP_FLAG(dwRemainder, ASC_REQ_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ASC_REQ_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ASC_REQ_STREAM);
    DUMP_FLAG(dwRemainder, ASC_REQ_INTEGRITY);
    DUMP_FLAG(dwRemainder, ASC_REQ_LICENSING);
    DUMP_FLAG(dwRemainder, ASC_REQ_IDENTIFY);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_NON_USER_LOGONS);
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_CONTEXT_REPLAY);
    DUMP_FLAG(dwRemainder, ASC_REQ_FRAGMENT_TO_FIT);
    DUMP_FLAG(dwRemainder, ASC_REQ_FRAGMENT_SUPPLIED);
    DUMP_FLAG(dwRemainder, ASC_REQ_NO_TOKEN);
#ifdef ASC_REQ_PROXY_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_REQ_PROXY_BINDINGS);
#endif
#ifdef ASC_REQ_ALLOW_MISSING_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_REQ_ALLOW_MISSING_BINDINGS);
#endif

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

VOID
DumpAscRetFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    DUMP_FLAG(dwRemainder, ASC_RET_DELEGATE);
    DUMP_FLAG(dwRemainder, ASC_RET_MUTUAL_AUTH);
    DUMP_FLAG(dwRemainder, ASC_RET_REPLAY_DETECT);
    DUMP_FLAG(dwRemainder, ASC_RET_SEQUENCE_DETECT);
    DUMP_FLAG(dwRemainder, ASC_RET_CONFIDENTIALITY);
    DUMP_FLAG(dwRemainder, ASC_RET_USE_SESSION_KEY);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOCATED_MEMORY);
    DUMP_FLAG(dwRemainder, ASC_RET_USED_DCE_STYLE);
    DUMP_FLAG(dwRemainder, ASC_RET_DATAGRAM);
    DUMP_FLAG(dwRemainder, ASC_RET_CONNECTION);
    DUMP_FLAG(dwRemainder, ASC_RET_CALL_LEVEL);
    DUMP_FLAG(dwRemainder, ASC_RET_THIRD_LEG_FAILED);
    DUMP_FLAG(dwRemainder, ASC_RET_EXTENDED_ERROR);
    DUMP_FLAG(dwRemainder, ASC_RET_STREAM);
    DUMP_FLAG(dwRemainder, ASC_RET_INTEGRITY);
    DUMP_FLAG(dwRemainder, ASC_RET_LICENSING);
    DUMP_FLAG(dwRemainder, ASC_RET_IDENTIFY);
    DUMP_FLAG(dwRemainder, ASC_RET_NULL_SESSION);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOW_NON_USER_LOGONS);
    DUMP_FLAG(dwRemainder, ASC_RET_ALLOW_CONTEXT_REPLAY);
    DUMP_FLAG(dwRemainder, ASC_RET_FRAGMENT_ONLY);
    DUMP_FLAG(dwRemainder, ASC_RET_NO_TOKEN);
#ifdef ASC_RET_NO_ADDITIONAL_TOKEN
    DUMP_FLAG(dwRemainder, ASC_RET_NO_ADDITIONAL_TOKEN);
#endif
#ifdef ASC_RET_NO_PROXY_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_RET_NO_PROXY_BINDINGS);
#endif
#ifdef ASC_RET_MISSING_BINDINGS
    DUMP_FLAG(dwRemainder, ASC_RET_MISSING_BINDINGS);
#endif

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

static
VOID
DumpNtlmFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

    printf("NTLM flag information (0x%08X):\n", dwFlags);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNICODE);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_OEM);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_REQUEST_TARGET);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_00000008);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_SIGN);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_SEAL);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_DATAGRAM);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_LM_KEY);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_NETWARE);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_NTLM);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_00000400);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_ANONYMOUS);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_DOMAIN);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_WORKSTATION);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_LOCAL_CALL);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_ALWAYS_SIGN);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_TYPE_DOMAIN);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_TYPE_SERVER);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_TYPE_SHARE);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_NTLM2);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_INIT_RESPONSE);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_ACCEPT_RESPONSE);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_NON_NT_SESSION_KEY);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_TARGET_INFO);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_10000000);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNKNOWN_02000000);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_04000000);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_08000000);

    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_10000000);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_128);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_KEY_EXCH);
    DUMP_FLAG(dwRemainder, NTLM_FLAG_56);

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

static
VOID
DumpNegotiateMessage(
    PVOID pBuffer,
    DWORD dwSize
    )
{
    PNTLM_NEGOTIATE_MESSAGE pMsg = (PNTLM_NEGOTIATE_MESSAGE) pBuffer;
    if (dwSize < RTL_SIZEOF_THROUGH_FIELD(NTLM_NEGOTIATE_MESSAGE, NtlmFlags))
    {
        return;
    }
    DumpNtlmFlags(pMsg->NtlmFlags);
}

static
VOID
DumpChallengeMessage(
    PVOID pBuffer,
    DWORD dwSize
    )
{
    PNTLM_CHALLENGE_MESSAGE pMsg = (PNTLM_CHALLENGE_MESSAGE) pBuffer;
    if (dwSize < RTL_SIZEOF_THROUGH_FIELD(NTLM_CHALLENGE_MESSAGE, NtlmFlags))
    {
        return;
    }
    DumpNtlmFlags(pMsg->NtlmFlags);
}

static
VOID
DumpResponseMessage(
    PVOID pBuffer,
    DWORD dwSize
    )
{
    PNTLM_RESPONSE_MESSAGE pMsg = (PNTLM_RESPONSE_MESSAGE) pBuffer;
    DWORD firstDataOffset = (DWORD)-1;

    if (dwSize < RTL_SIZEOF_THROUGH_FIELD(NTLM_RESPONSE_MESSAGE, Workstation))
    {
        return;
    }

#define MIN(x,y) ((x) < (y) ? (x) : (y))

    firstDataOffset = MIN(firstDataOffset, pMsg->LmResponse.dwOffset);
    firstDataOffset = MIN(firstDataOffset, pMsg->NtResponse.dwOffset);
    firstDataOffset = MIN(firstDataOffset, pMsg->AuthTargetName.dwOffset);
    firstDataOffset = MIN(firstDataOffset, pMsg->UserName.dwOffset);
    firstDataOffset = MIN(firstDataOffset, pMsg->Workstation.dwOffset);

    if (firstDataOffset >= RTL_SIZEOF_THROUGH_FIELD(NTLM_RESPONSE_MESSAGE, SessionKey) && firstDataOffset <= dwSize)
    {
	firstDataOffset = MIN(firstDataOffset, pMsg->SessionKey.dwOffset);
    }

    if (firstDataOffset >= RTL_SIZEOF_THROUGH_FIELD(NTLM_RESPONSE_MESSAGE, NtlmFlags) && firstDataOffset <= dwSize)
    {
	DumpNtlmFlags(pMsg->NtlmFlags);
    }
}

VOID
DumpNtlmMessage(
    PVOID pBuffer,
    DWORD dwSize
    )
{

    PNTLM_MESSAGE pMsg = (PNTLM_MESSAGE)pBuffer;
    PCSTR pszType = NULL;
    PFN_DUMP pfnDump = NULL;

    if (dwSize < RTL_SIZEOF_THROUGH_FIELD(NTLM_MESSAGE, MessageType))
    {
        return;
    }

    printf("NTLM message information:\n");

    switch (pMsg->MessageType)
    {
    case 1:
        pszType = "Negotiate";
        pfnDump = DumpNegotiateMessage;
        break;
    case 2:
        pszType = "Challenge";
        pfnDump = DumpChallengeMessage;
        break;
    case 3:
        pszType = "Response";
        pfnDump = DumpResponseMessage;
        break;
    default:
        pszType = "UNKNOWN";
        break;
    }

    printf("    Message type: %s (0x%08X)\n", pszType, pMsg->MessageType);

    if (pfnDump)
    {
        pfnDump(pBuffer, dwSize);
    }
}

VOID
DumpBuffer(
    PVOID pBuffer,
    DWORD dwLength
    )
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    INT cbLine;
    PBYTE pCurrent = (PBYTE) pBuffer;
    DWORD dwCurrentLength = dwLength;

    for (index = 0;
         dwCurrentLength;
         dwCurrentLength -= count, pCurrent += count, index += count)
    {
        count = (dwCurrentLength > 16) ? 16 : dwCurrentLength;

        sprintf_s(rgbLine, 100, "%4.4x  ",index);
        cbLine = 6;

        for (i=0;i<count;i++)
        {
            rgbLine[cbLine++] = rgbDigits[pCurrent[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[pCurrent[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[cbLine++] = ':';
            }
            else
            {
                rgbLine[cbLine++] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
        }

        rgbLine[cbLine++] = ' ';

        for (i = 0; i < count; i++)
        {
            if (pCurrent[i] < 32 || pCurrent[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = pCurrent[i];
            }
        }

        rgbLine[cbLine++] = 0;
        printf("%s\n", rgbLine);
    }
}

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    )
{
    DWORD dwError = ERROR_SUCCESS;
    ULONG ulLen = 0;
    DWORD dwBytesWritten = 0;

    ulLen = htonl(pToken->cbBuffer);

    dwError = WriteAll(
        nSocket,
        (PCHAR)&ulLen,
        4,
        &dwBytesWritten
        );

    BAIL_ON_ERROR(dwError);

    if (4 != dwBytesWritten)
    {
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = WriteAll(
        nSocket,
        pToken->pvBuffer,
        pToken->cbBuffer,
        &dwBytesWritten
        );

    BAIL_ON_ERROR(dwError);

    if (dwBytesWritten != pToken->cbBuffer)
    {
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwBytesRead = 0;

    memset(pToken, 0, sizeof(SecBuffer));

    pToken->BufferType = SECBUFFER_TOKEN;

    dwError = ReadAll(
        nSocket,
        (PCHAR)&pToken->cbBuffer,
        4,
        &dwBytesRead
        );
    BAIL_ON_ERROR(dwError);

    if (4 != dwBytesRead)
    {
        fprintf(stderr, "Incorrect read size of %u bytes (expected %u bytes)\n", dwBytesRead, 4);
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

    pToken->cbBuffer = ntohl(pToken->cbBuffer);
    pToken->pvBuffer = (PCHAR) malloc(pToken->cbBuffer);

    if (pToken->pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
    }

    dwError = ReadAll(
        nSocket,
        (PCHAR)pToken->pvBuffer,
        pToken->cbBuffer,
        &dwBytesRead
        );

    BAIL_ON_ERROR(dwError);

    if (dwBytesRead != pToken->cbBuffer)
    {
        fprintf(stderr, "Incorrect read size of %u bytes (expected %u bytes)\n", dwBytesRead, pToken->cbBuffer);
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (pToken->pvBuffer)
    {
        free(pToken->pvBuffer);
        memset(pToken, 0, sizeof(SecBuffer));
    }

    goto cleanup;
}

static
DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesWritten
    )
{
    DWORD dwError = ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *pdwBytesWritten = 0;

    for (pTrav = pBuffer; dwBytes; pTrav += nReturn, dwBytes -= nReturn)
    {
        nReturn = send(nSocket, pTrav, dwBytes, 0);

        if (nReturn < 0)
        {
            dwError = GetLastError();
            BAIL_ON_ERROR(dwError);
        }

        if (nReturn == 0)
        {
            break;
        }
    }

    *pdwBytesWritten = pTrav - pBuffer;

error:
    return dwError;
}

static
DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesRead
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int nReturn = 0;
    char *pTrav = NULL;

    memset(pBuffer, 0, dwBytes);
    *pdwBytesRead = 0;

    for (pTrav = pBuffer; dwBytes; pTrav += nReturn, dwBytes -= nReturn)
    {
        nReturn = recv(nSocket, pTrav, dwBytes, 0);

        if (nReturn < 0)
        {
            dwError = GetLastError();
            BAIL_ON_ERROR(dwError);
        }

        if (nReturn == 0)
        {
            // TODO- Perhaps return error here
            printf("Connection closed\n");
            break;
        }
    }

    *pdwBytesRead = pTrav - pBuffer;

error:
    return dwError;
}
