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


#include "ntlmclient.h"

INT
main(
    IN INT argc,
    IN PCHAR* argv
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    PCHAR pServerHost = "localhost";
    PCHAR pMsg = "Hello";
    PCHAR pSecPkgName = "NTLM";
    USHORT usPort = 4444;
    INT nSignOnly = 0;
    CHAR Error[256] = {0};

    // These are the negotiation flags... we'll update these later
    //
    DWORD DelegFlag =
        NTLM_FLAG_NEGOTIATE_DEFAULT |
        NTLM_FLAG_SIGN              |
        NTLM_FLAG_SEAL;

    /* Parse arguments. */
    argc--; argv++;
    while (argc)
    {

        if (strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_NTLM_ERROR(dwError);
            }

            usPort = (USHORT)atoi(*argv);
        }
        else if (strcmp(*argv, "-sign") == 0)
        {
            nSignOnly = TRUE;
        }
        else if (strcmp(*argv, "-c") == 0)
        {
            argv++;
            argc--;

            if (argc < 3)
            {
                dwError = Usage();
                BAIL_ON_NTLM_ERROR(dwError);
            }

            pServiceName = *argv;
            argv++;
            argc--;
            pServicePassword = *argv;
            argv++;
            argc--;
            pServiceRealm = *argv;
        }
        else if (strcmp(*argv, "-h") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_NTLM_ERROR(dwError);
            }

            pServerHost = *argv;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_NTLM_ERROR(dwError);
            }

            pMsg = *argv;
        }
        else
        {
            fprintf(
                stderr,
                "Invalid parameter: %s\n",
                *argv
                );

            dwError = Usage();
            BAIL_ON_NTLM_ERROR(dwError);
        }
        argc--; argv++;
    }

    if (nSignOnly)
    {
        DelegFlag &= ~NTLM_FLAG_SEAL;
        printf("No encryption - sign only\n");
    }

    dwError = CallServer(
        pServerHost,
        usPort,
        pServiceName,
        pServicePassword,
        pServiceRealm,
        DelegFlag,
        pMsg,
        pSecPkgName,
        nSignOnly
        );

    BAIL_ON_NTLM_ERROR(dwError);

error:
    if (dwError)
    {
        printf("Finished with error code: %u\n", dwError);
        LwGetErrorString(dwError, Error, 256);
        printf("%s\n", Error);
    }

    return dwError;
}

DWORD
Usage(VOID)
{
    fprintf(
        stderr,
        "Usage: ntlm-client [-port port] [-sign] [-h host] [-m msg]\n"
        );
    fprintf(
        stderr,
        "       [-c username password domain]\n"
        );

    return LW_ERROR_INVALID_PARAMETER;
}

DWORD
CallServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN DWORD DelegFlag,
    IN PCHAR pMsg,
    IN PCHAR pSecPkgName,
    IN INT nSignOnly
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    SecBuffer WrapBuffers[3];
    INT nSocket = INVALID_SOCKET;
    INT nIndex = 0;
    DWORD RetFlags = 0;
    DWORD dwQop = 0;
    INT nContextAcquired = 0;

    NTLM_CONTEXT_HANDLE Context = NULL;
    SecBuffer InBuffer;
    SecBuffer OutBuffer;
    SecBufferDesc InBufferDesc;
    SecPkgContext_Sizes Sizes;
    SecPkgContext_Names Names;
    SecPkgContext_SessionKey SessionKey;

    memset(&InBuffer, 0, sizeof(SecBuffer));
    memset(&OutBuffer, 0, sizeof(SecBuffer));
    memset(&InBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));
    memset(&Names, 0, sizeof(SecPkgContext_Names));
    memset(&SessionKey, 0, sizeof(SecPkgContext_SessionKey));
    memset(WrapBuffers, 0, sizeof(SecBuffer)*3);

    /* Open connection */

    dwError = ConnectToServer(pHost, usPort, &nSocket);
    BAIL_ON_NTLM_ERROR(dwError);

    /* Establish context */
    dwError = ClientEstablishContext(
            nSocket,
            pServiceName,
            pServicePassword,
            pServiceRealm,
            DelegFlag,
            &Context,
            pSecPkgName,
            &RetFlags
            );

    BAIL_ON_NTLM_ERROR(dwError);

    nContextAcquired = 1;

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_NAMES,
        &Names);
    BAIL_ON_NTLM_ERROR(dwError);

    printf("Context is for user: %s\n", Names.pUserName);

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SESSION_KEY,
        &SessionKey);
    BAIL_ON_NTLM_ERROR(dwError);

    printf("Session Key: ");
    for(nIndex = 0; nIndex < SessionKey.SessionKeyLength; nIndex++)
    {
        printf("%02X ", SessionKey.pSessionKey[nIndex]);
    }
    printf("\n\n");

    // Cut the test short for now... until sign and seal are done
    printf("Client initialized context successfully!\n");
    exit(1);

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_NTLM_ERROR(dwError);

    /* Seal the message */
    InBuffer.pvBuffer = pMsg;
    InBuffer.cbBuffer = (DWORD)strlen(pMsg) + 1;

    //
    // Prepare to encrypt the message
    //

    InBufferDesc.cBuffers = 3;
    InBufferDesc.pBuffers = WrapBuffers;
    //InBufferDesc.ulVersion = SECBUFFER_VERSION;

    WrapBuffers[0].cbBuffer = Sizes.cbSecurityTrailer;
    WrapBuffers[0].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[0].pvBuffer = malloc(Sizes.cbSecurityTrailer);

    if (WrapBuffers[0].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = InBuffer.cbBuffer;
    WrapBuffers[1].pvBuffer = malloc(WrapBuffers[1].cbBuffer);

    if (WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    memcpy(
        WrapBuffers[1].pvBuffer,
        InBuffer.pvBuffer,
        InBuffer.cbBuffer
        );

    WrapBuffers[2].BufferType = SECBUFFER_PADDING;
    WrapBuffers[2].cbBuffer = Sizes.cbBlockSize;
    WrapBuffers[2].pvBuffer = malloc(WrapBuffers[2].cbBuffer);

    if (WrapBuffers[2].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NtlmClientEncryptMessage(
        &Context,
        !nSignOnly,
        &InBufferDesc,
        0
        );

    BAIL_ON_NTLM_ERROR(dwError);

    //
    // Create the mesage to send to server
    //

    OutBuffer.cbBuffer = WrapBuffers[0].cbBuffer + WrapBuffers[1].cbBuffer + WrapBuffers[2].cbBuffer;
    OutBuffer.pvBuffer = malloc(OutBuffer.cbBuffer);

    if (OutBuffer.pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    memcpy(
        OutBuffer.pvBuffer,
        WrapBuffers[0].pvBuffer,
        WrapBuffers[0].cbBuffer
        );
    memcpy(
        (PUCHAR) OutBuffer.pvBuffer + (int) WrapBuffers[0].cbBuffer,
        WrapBuffers[1].pvBuffer,
        WrapBuffers[1].cbBuffer
        );
    memcpy(
        (PUCHAR) OutBuffer.pvBuffer + WrapBuffers[0].cbBuffer + WrapBuffers[1].cbBuffer,
        WrapBuffers[2].pvBuffer,
        WrapBuffers[2].cbBuffer
        );

    /* Send to server */
    dwError = SendToken(nSocket, &OutBuffer);
    BAIL_ON_NTLM_ERROR(dwError);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;
    OutBuffer.cbBuffer = 0;
    free(WrapBuffers[0].pvBuffer);
    WrapBuffers[0].pvBuffer = NULL;
    free(WrapBuffers[1].pvBuffer);
    WrapBuffers[1].pvBuffer = NULL;

    /* Read signature block into OutBuffer */
    dwError = RecvToken(nSocket, &OutBuffer);
    BAIL_ON_NTLM_ERROR(dwError);

    /* Verify signature block */

    InBufferDesc.cBuffers = 2;
    WrapBuffers[0] = InBuffer;
    WrapBuffers[0].BufferType = SECBUFFER_DATA;
    WrapBuffers[1] = OutBuffer;
    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;

    dwError = NtlmClientVerifySignature(
        &Context,
        &InBufferDesc,
        0,
        &dwQop
        );
    BAIL_ON_NTLM_ERROR(dwError);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;

    /* Delete context */
    dwError = NtlmClientDeleteSecurityContext(
        &Context);
    BAIL_ON_NTLM_ERROR(dwError);

    close(nSocket);

finish:
    return dwError;
error:
    if (nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(
            &Context);
    }
    if (INVALID_SOCKET != nSocket)
    {
        close(nSocket);
    }
    if (WrapBuffers[0].pvBuffer)
    {
        free(WrapBuffers[0].pvBuffer);
    }
    if (WrapBuffers[1].pvBuffer)
    {
        free(WrapBuffers[1].pvBuffer);
    }
    if (WrapBuffers[2].pvBuffer)
    {
        free(WrapBuffers[2].pvBuffer);
    }
    if (OutBuffer.pvBuffer)
    {
        free(OutBuffer.pvBuffer);
    }

    goto finish;
}

DWORD
ConnectToServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct sockaddr_in sAddr;
    struct hostent *pHostEnt;

    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    *pSocket = INVALID_SOCKET;

    pHostEnt = gethostbyname(pHost);

    if (pHostEnt == NULL)
    {
        dwError = LwMapErrnoToLwError(h_errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    sAddr.sin_family = pHostEnt->h_addrtype;
    memcpy((PCHAR)&sAddr.sin_addr, pHostEnt->h_addr, sizeof(sAddr.sin_addr));
    sAddr.sin_port = htons(usPort);

    *pSocket = (INT)socket(PF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == *pSocket)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = connect(*pSocket, (struct sockaddr *)&sAddr, sizeof(sAddr));
    if (dwError == SOCKET_ERROR)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (INVALID_SOCKET != *pSocket)
    {
        close(*pSocket);
        *pSocket = INVALID_SOCKET;
    }
    goto finish;
}

DWORD
ClientEstablishContext(
    IN INT nSocket,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN DWORD DelegFlag,
    OUT PNTLM_CONTEXT_HANDLE pSspiContext,
    IN PCHAR pSecPkgName,
    OUT PDWORD pRetFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLoopError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT_HANDLE pContextHandle = NULL;
    INT nCredentialsAcquired = 0;
    INT nContextAcquired = 0;

    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    NTLM_CRED_HANDLE CredHandle = NULL;
    TimeStamp Expiry = 0;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;
    PSEC_WINNT_AUTH_IDENTITY pAuthId = NULL;

    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&AuthIdentity, 0, sizeof(AuthIdentity));

    *pSspiContext = NULL;
    *pRetFlags = 0;

    InputDesc.cBuffers = 1;
    InputDesc.pBuffers = &RecvTokenBuffer;
    //InputDesc.ulVersion = SECBUFFER_VERSION;

    RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
    RecvTokenBuffer.cbBuffer = 0;
    RecvTokenBuffer.pvBuffer = NULL;

    OutputDesc.cBuffers = 1;
    OutputDesc.pBuffers = &SendTokenBuffer;
    //OutputDesc.ulVersion = SECBUFFER_VERSION;

    SendTokenBuffer.BufferType = SECBUFFER_TOKEN;
    SendTokenBuffer.cbBuffer = 0;
    SendTokenBuffer.pvBuffer = NULL;

    if (pServiceName)
    {
        AuthIdentity.User = pServiceName;
        AuthIdentity.UserLength = (DWORD)strlen(pServiceName);
        pAuthId = &AuthIdentity;
    }

    if (pServicePassword)
    {
        AuthIdentity.Password = pServicePassword;
        AuthIdentity.PasswordLength = (DWORD)strlen(pServicePassword);
        pAuthId = &AuthIdentity;
    }

    if (pServiceRealm)
    {
        AuthIdentity.Domain = pServiceRealm;
        AuthIdentity.DomainLength = (DWORD)strlen(pServiceRealm);
        pAuthId = &AuthIdentity;
    }
    //AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    dwError = NtlmClientAcquireCredentialsHandle(
        NULL,                       // no principal name
        pSecPkgName,                // package name
        NTLM_CRED_OUTBOUND,
        NULL,                       // no logon id
        pAuthId,              // no auth data
        &CredHandle,
        &Expiry
        );

    BAIL_ON_NTLM_ERROR(dwError);

    nCredentialsAcquired = 1;

    // Perform the context-establishement loop.
    do
    {
        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError =
            NtlmClientInitializeSecurityContext(
                &CredHandle,
                pContextHandle,
                pServiceName,
                DelegFlag,
                0,          // reserved
                0, //SECURITY_NATIVE_DREP,
                &InputDesc,
                0,          // reserved
                pSspiContext,  // <-- this is the handle to the data returned in OutputDesc... it's used to make look ups faster.
                &OutputDesc,   // <-- this is the data the above handle represents
                pRetFlags,
                &Expiry
                );

        if (LW_ERROR_SUCCESS != dwLoopError && LW_WARNING_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_NTLM_ERROR(dwError);
        }

        if (LW_WARNING_CONTINUE_NEEDED == dwLoopError)
        {
            printf("Context partially initialized...\n");
            PrintHexDump(SendTokenBuffer.cbBuffer, SendTokenBuffer.pvBuffer);
            printf("\n");
        }
        else
        {
            printf("Context FULLY initialized!\n");
            PrintHexDump(SendTokenBuffer.cbBuffer, SendTokenBuffer.pvBuffer);
            printf("\n");
        }

        nContextAcquired = 1;

        pContextHandle = pSspiContext;

        if (RecvTokenBuffer.pvBuffer)
        {
            free(RecvTokenBuffer.pvBuffer);
            RecvTokenBuffer.pvBuffer = NULL;
            RecvTokenBuffer.cbBuffer = 0;
        }

        if (SendTokenBuffer.cbBuffer != 0)
        {
            dwError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_NTLM_ERROR(dwError);
        }

        NtlmFreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;

        if (LW_WARNING_CONTINUE_NEEDED == dwLoopError)
        {
            dwError = RecvToken(nSocket, &RecvTokenBuffer);
            BAIL_ON_NTLM_ERROR(dwError);
            printf("RECEIVED:\n");
            PrintHexDump(RecvTokenBuffer.cbBuffer, RecvTokenBuffer.pvBuffer);
            printf("\n");
        }

    } while (dwLoopError == LW_WARNING_CONTINUE_NEEDED);

    NtlmClientFreeCredentialsHandle(&CredHandle);

finish:
    return dwError;
error:
    if (nCredentialsAcquired)
    {
        NtlmClientFreeCredentialsHandle(&CredHandle);
    }
    if (nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(pSspiContext);
        *pSspiContext = 0;
    }
    if (RecvTokenBuffer.pvBuffer)
    {
        free(RecvTokenBuffer.pvBuffer);
    }
    if (SendTokenBuffer.cbBuffer)
    {
        NtlmFreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;

    }

    goto finish;
}

DWORD
SendToken(
    IN INT          nSocket,
    IN PSecBuffer   pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLen = 0;
    INT nBytesWritten = 0;

    dwLen = htonl(pToken->cbBuffer);

    dwError = WriteAll(
        nSocket,
        (PCHAR)&dwLen,
        4,
        &nBytesWritten
        );

    BAIL_ON_NTLM_ERROR(dwError);

    if (4 != nBytesWritten)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = WriteAll(
        nSocket,
        pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesWritten
        );

    BAIL_ON_NTLM_ERROR(dwError);

    if (nBytesWritten != pToken->cbBuffer)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesWritten
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *nBytesWritten = 0;

    for (pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = send(nSocket, pTrav, nBytes, 0);

        if (nReturn < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_NTLM_ERROR(dwError);
        }

        if (nReturn == 0)
        {
            break;
        }
    }

    *nBytesWritten = pTrav - pBuffer;

error:
    return dwError;
}

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nBytesRead = 0;

    memset(pToken, 0, sizeof(SecBuffer));

    pToken->BufferType = SECBUFFER_TOKEN;

    dwError = ReadAll(
        nSocket,
        (PCHAR)&pToken->cbBuffer,
        4,
        &nBytesRead
        );

    BAIL_ON_NTLM_ERROR(dwError);

    if (4 != nBytesRead)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pToken->cbBuffer = ntohl(pToken->cbBuffer);
    pToken->pvBuffer = (PCHAR) malloc(pToken->cbBuffer);

    if (pToken->pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = ReadAll(
        nSocket,
        (PCHAR)pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesRead
        );

    BAIL_ON_NTLM_ERROR(dwError);

    if (nBytesRead != pToken->cbBuffer)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (pToken->pvBuffer)
    {
        free(pToken->pvBuffer);
        pToken->pvBuffer = NULL;
    }
    goto finish;
}

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesRead
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    int nReturn = 0;
    char *pTrav = NULL;

    memset(pBuffer, 0, nBytes);
    *nBytesRead = 0;

    for (pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = recv(nSocket, pTrav, nBytes, 0);

        if (nReturn < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_NTLM_ERROR(dwError);
        }

        if (nReturn == 0)
        {
            break;
        }
    }

    *nBytesRead = pTrav - pBuffer;

error:
    return dwError;
}

VOID
DumpNtlmFlags(
    DWORD dwFlags
    )
{
    printf("NTLM flag information (0x%X):\n", dwFlags);
    if (dwFlags & 0x00000001)
    {
        printf("NTLM_FLAG_UNICODE\n");
        dwFlags &= ~0x00000001;
    }
    if (dwFlags & 0x00000002)
    {
        printf("NTLM_FLAG_OEM\n");
        dwFlags &= ~0x00000002;
    }
    if (dwFlags & 0x00000004)
    {
        printf("NTLM_FLAG_REQUEST_TARGET\n");
        dwFlags &= ~0x00000004;
    }
    if (dwFlags & 0x00000010)
    {
        printf("NTLM_FLAG_SIGN\n");
        dwFlags &= ~0x00000010;
    }
    if (dwFlags & 0x00000020)
    {
        printf("NTLM_FLAG_SEAL\n");
        dwFlags &= ~0x00000020;
    }
    if (dwFlags & 0x00000040)
    {
        printf("NTLM_FLAG_DATAGRAM\n");
        dwFlags &= ~0x00000040;
    }
    if (dwFlags & 0x00000080)
    {
        printf("NTLM_FLAG_LM_KEY\n");
        dwFlags &= ~0x00000080;
    }
    if (dwFlags & 0x00000100)
    {
        printf("NTLM_FLAG_NETWARE\n");
        dwFlags &= ~0x00000100;
    }
    if (dwFlags & 0x00000200)
    {
        printf("NTLM_FLAG_NTLM\n");
        dwFlags &= ~0x00000200;
    }
    if (dwFlags & 0x00001000)
    {
        printf("NTLM_FLAG_DOMAIN\n");
        dwFlags &= ~0x00001000;
    }
    if (dwFlags & 0x00002000)
    {
        printf("NTLM_FLAG_WORKSTATION\n");
        dwFlags &= ~0x00002000;
    }
    if (dwFlags & 0x00004000)
    {
        printf("NTLM_FLAG_LOCAL_CALL\n");
        dwFlags &= ~0x00004000;
    }
    if (dwFlags & 0x00008000)
    {
        printf("NTLM_FLAG_ALWAYS_SIGN\n");
        dwFlags &= ~0x00008000;
    }
    if (dwFlags & 0x00010000)
    {
        printf("NTLM_FLAG_TYPE_DOMAIN\n");
        dwFlags &= ~0x00010000;
    }
    if (dwFlags & 0x00020000)
    {
        printf("NTLM_FLAG_TYPE_SERVER\n");
        dwFlags &= ~0x00020000;
    }
    if (dwFlags & 0x00040000)
    {
        printf("NTLM_FLAG_TYPE_SHARE\n");
        dwFlags &= ~0x00040000;
    }
    if (dwFlags & 0x00080000)
    {
        printf("NTLM_FLAG_NTLM2\n");
        dwFlags &= ~0x00080000;
    }
    if (dwFlags & 0x00100000)
    {
        printf("NTLM_FLAG_INIT_RESPONSE\n");
        dwFlags &= ~0x00100000;
    }
    if (dwFlags & 0x00200000)
    {
        printf("NTLM_FLAG_ACCEPT_RESPONSE\n");
        dwFlags &= ~0x00200000;
    }
    if (dwFlags & 0x00400000)
    {
        printf("NTLM_FLAG_NON_NT_SESSION_KEY\n");
        dwFlags &= ~0x00400000;
    }
    if (dwFlags & 0x00800000)
    {
        printf("NTLM_FLAG_TARGET_INFO\n");
        dwFlags &= ~0x00800000;
    }
    if (dwFlags & 0x02000000)
    {
        printf("NTLM_FLAG_UNKNOWN_02000000\n");
        dwFlags &= ~0x02000000;
    }
    if (dwFlags & 0x20000000)
    {
        printf("NTLM_FLAG_128\n");
        dwFlags &= ~0x20000000;
    }
    if (dwFlags & 0x40000000)
    {
        printf("NTLM_FLAG_KEY_EXCH\n");
        dwFlags &= ~0x40000000;
    }
    if (dwFlags & 0x80000000)
    {
        printf("NTLM_FLAG_56\n");
        dwFlags &= ~0x80000000;
    }
    if (dwFlags)
    {
        printf("Unknown flags: 0x%X\n", dwFlags);
    }
}

VOID
DumpNegMessage(
    PNTLM_NEGOTIATE_MESSAGE pMsg,
    DWORD dwSize
    )
{
    printf("Message type: Negotiate\n");
    DumpNtlmFlags(pMsg->NtlmFlags);
}

VOID
DumpChlngMessage(
    PNTLM_CHALLENGE_MESSAGE pMsg,
    DWORD dwSize
    )
{
    printf("Message type: Challenge\n");
    DumpNtlmFlags(pMsg->NtlmFlags);
}

VOID
DumpRespMessage(
    PNTLM_RESPONSE_MESSAGE pMsg,
    DWORD dwSize
    )
{
    printf("Message type: Response\n");
}

VOID
DumpNtlmMessage(
    PBYTE pBuffer,
    DWORD dwSize
    )
{

    PNTLM_MESSAGE pMsg = (PNTLM_MESSAGE)pBuffer;

    printf("NTLM message information:\n");

    switch (pMsg->MessageType)
    {
    case 1:
        DumpNegMessage((PNTLM_NEGOTIATE_MESSAGE)pMsg, dwSize);
        break;
    case 2:
        DumpChlngMessage((PNTLM_CHALLENGE_MESSAGE)pMsg, dwSize);
        break;
    case 3:
        DumpRespMessage((PNTLM_RESPONSE_MESSAGE)pMsg, dwSize);
        break;
    }
}

VOID
PrintHexDump(
    DWORD dwLength,
    PBYTE pBuffer
    )
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    CHAR cbLine;
    PBYTE pToken = pBuffer;

    for (index = 0; dwLength;
        dwLength -= count, pBuffer += count, index += count)
    {
        count = (dwLength > 16) ? 16:dwLength;

        sprintf(rgbLine, "%4.4x  ",index);
        cbLine = 6;

        for (i=0;i<count;i++)
        {
            rgbLine[(int)(cbLine++)] = rgbDigits[pBuffer[i] >> 4];
            rgbLine[(int)(cbLine++)] = rgbDigits[pBuffer[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[(int)(cbLine++)] = ':';
            }
            else
            {
                rgbLine[(int)(cbLine++)] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[(int)(cbLine++)] = ' ';
            rgbLine[(int)(cbLine++)] = ' ';
            rgbLine[(int)(cbLine++)] = ' ';
        }

        rgbLine[(int)(cbLine++)] = ' ';

        for (i = 0; i < count; i++)
        {
            if (pBuffer[i] < 32 || pBuffer[i] > 126)
            {
                rgbLine[(int)(cbLine++)] = '.';
            }
            else
            {
                rgbLine[(int)(cbLine++)] = pBuffer[i];
            }
        }

        rgbLine[(int)(cbLine++)] = 0;
        printf("%s\n", rgbLine);
    }

    DumpNtlmMessage(pToken, dwLength);
}
