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

#include "ntlm-server.h"

INT
main(
    IN INT argc,
    IN PCHAR* argv
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nListenSocket = INVALID_SOCKET;
    INT nAcceptSocket = INVALID_SOCKET;
    USHORT usPort = 4444;
    INT nOnce = 0;
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    NTLM_CRED_HANDLE ServerCreds = NULL;
    INT nServerCredsAcquired = 0;
    DWORD AscFlags = 0; //ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_MUTUAL_AUTH;
    PCHAR pSecPkgName = "NTLM";
    BOOLEAN bFound = FALSE;
    CHAR Error[256] = {0};

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
            usPort = (u_short)atoi(*argv);
        }
        else if (strcmp(*argv, "-once") == 0)
        {
            nOnce = 1;
        }
        else if (strcmp(*argv, "-w") == 0)
        {
            gbFlipEndian = TRUE;
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
        else
        {
            /*
            for (i = 0; i < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); i++)
            {
                if (_strcmpi( *argv, FlagMappings[ i ].name ) == 0)
                {
                    bFound = TRUE;
                    AscFlags |= FlagMappings[ i ].value ;
                    break;
                }
            }
            */

            if (!bFound)
            {
                break;
            }
        }

        argc--; argv++;
    }

    dwError = ServerAcquireCreds(
        pServiceName,
        pServicePassword,
        pServiceRealm,
        pSecPkgName,
        &ServerCreds
        );

    BAIL_ON_NTLM_ERROR(dwError);

    nServerCredsAcquired = 1;

    dwError = CreateSocket(usPort, &nListenSocket);

    BAIL_ON_NTLM_ERROR(dwError);

    do
    {
        /* Accept a TCP connection */
        printf("Listening\n");
        nAcceptSocket = accept(nListenSocket, NULL, 0);

        if (INVALID_SOCKET == nAcceptSocket)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_NTLM_ERROR(dwError);
        }

        dwError = SignServer(
            nAcceptSocket,
            &ServerCreds,
            AscFlags
            );

        if (dwError)
        {
            printf("Finished with error code: %u\n", dwError);
            LwGetErrorString(dwError, Error, 256);
            printf("%s\n", Error);
        }

        //BAIL_ON_NTLM_ERROR(dwError);

        close(nAcceptSocket);

    } while (!nOnce);

    close(nListenSocket);

    NtlmClientFreeCredentialsHandle(&ServerCreds);

finish:
    if (dwError)
    {
        printf("Finished with error code: %u\n", dwError);
        LwGetErrorString(dwError, Error, 256);
        printf("%s\n", Error);
    }

    return dwError;
error:
    if (nServerCredsAcquired)
    {
        NtlmClientFreeCredentialsHandle(&ServerCreds);
    }
    if (INVALID_SOCKET != nListenSocket)
    {
        close(nListenSocket);
    }
    if (INVALID_SOCKET != nAcceptSocket)
    {
        close(nAcceptSocket);
    }
    goto finish;
}

DWORD
Usage(
    VOID
    )
{
    fprintf(stderr, "Usage: sspi-server [-port port] [-k]\n");
    fprintf(stderr, "       [-c service_name service_password service_realm]\n");

    return LW_ERROR_INVALID_PARAMETER;
}

DWORD
ServerAcquireCreds(
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN PCHAR pSecPkgName,
    OUT PNTLM_CRED_HANDLE pServerCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    TimeStamp Expiry;

    SEC_WINNT_AUTH_IDENTITY AuthIdentity;
    PSEC_WINNT_AUTH_IDENTITY pAuthId = NULL;

    memset(&Expiry, 0, sizeof(TimeStamp));
    memset(&AuthIdentity, 0, sizeof(SEC_WINNT_AUTH_IDENTITY));

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
        NULL,
        pSecPkgName,
        NTLM_CRED_INBOUND,
        NULL,                       // no logon id
        pAuthId,                    // auth data
        pServerCreds,
        &Expiry
        );

    BAIL_ON_NTLM_ERROR(dwError);

error:
    return dwError;
}

DWORD
CreateSocket(
    IN USHORT uPort,
    OUT PINT pSocket
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct sockaddr_in sAddr;
    INT nOn = 1;

    *pSocket = INVALID_SOCKET;
    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(uPort);
    sAddr.sin_addr.s_addr = INADDR_ANY;

    *pSocket = (int)socket(PF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == *pSocket)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* Let the socket be reused right away */
    dwError = setsockopt
        (
        *pSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char *)&nOn,
        sizeof(nOn)
        );

    if (dwError != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = bind(
        *pSocket,
        (struct sockaddr *) &sAddr,
        sizeof(sAddr)
        );

    if (SOCKET_ERROR == dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = listen(*pSocket, 5);

    if (SOCKET_ERROR == dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (*pSocket != INVALID_SOCKET)
    {
        close(*pSocket);
        *pSocket = INVALID_SOCKET;
    }

    goto finish;
}

DWORD
SignServer(
    IN INT nSocket,
    IN PNTLM_CRED_HANDLE pServerCreds,
    IN DWORD AscFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    //BOOLEAN bEncrypted = 0;
    INT nContextAcquired = 0;
    INT nIndex = 0;
    SecBuffer TransmitBuffer;
    SecBuffer MsgBuffer;
    SecBuffer WrapBuffers[2];
    SecBufferDesc WrapBufferDesc;
    NTLM_CONTEXT_HANDLE Context = NULL;
    SecPkgContext_Sizes Sizes;
    SecPkgContext_Names Names;
    SecPkgContext_SessionKey SessionKey;

    memset(&TransmitBuffer, 0, sizeof(SecBuffer));
    memset(&MsgBuffer, 0, sizeof(SecBuffer));
    memset(WrapBuffers, 0, sizeof(SecBuffer) * 2);
    memset(&WrapBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));
    memset(&Names, 0, sizeof(SecPkgContext_Names));

    /* Establish a context with the client */
    dwError = ServerEstablishContext(
        nSocket,
        pServerCreds,
        &Context,
        AscFlags
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_NAMES,
        &Names
        );
    BAIL_ON_NTLM_ERROR(dwError);

    printf("Context is for user: %s\n", Names.pUserName);

    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SESSION_KEY,
        &SessionKey
        );
    BAIL_ON_NTLM_ERROR(dwError);

    printf("Session Key: ");
    for(nIndex = 0; nIndex < SessionKey.SessionKeyLength; nIndex++)
    {
        printf("%02X ", SessionKey.pSessionKey[nIndex]);
    }
    printf("\n\n");

    printf("Server accepted context successfully!\n");

    // for clean up... once we've established a context, we must clean it up on
    // future failures.
    nContextAcquired = 1;

#if 0
    dwError = NtlmClientQueryContextAttributes(
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_NTLM_ERROR(dwError);

    /* Receive the sealed message token */
    dwError = RecvToken(nSocket, &TransmitBuffer);
    BAIL_ON_NTLM_ERROR(dwError);

    WrapBufferDesc.cBuffers = 2;
    WrapBufferDesc.pBuffers = WrapBuffers;
    //WrapBufferDesc.ulVersion = SECBUFFER_VERSION;
    WrapBuffers[0].BufferType = SECBUFFER_STREAM;
    WrapBuffers[0].pvBuffer = TransmitBuffer.pvBuffer;
    WrapBuffers[0].cbBuffer = TransmitBuffer.cbBuffer;
    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = 0;
    WrapBuffers[1].pvBuffer = NULL;

    dwError = NtlmClientDecryptMessage(
        &Context,
        &WrapBufferDesc,
        0,                  // no sequence number
        &bEncrypted
        );

    BAIL_ON_NTLM_ERROR(dwError);

    MsgBuffer = WrapBuffers[1];

    /* Produce a signature block for the message */

    WrapBuffers[0] = MsgBuffer;

    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[1].cbBuffer = Sizes.cbMaxSignature;
    WrapBuffers[1].pvBuffer = malloc(Sizes.cbMaxSignature);

    if (WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NtlmClientMakeSignature(
        &Context,
        FALSE,
        &WrapBufferDesc,
        0
        );

    BAIL_ON_NTLM_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);

    TransmitBuffer = WrapBuffers[1];
    WrapBuffers[1].pvBuffer = NULL;
    WrapBuffers[1].cbBuffer = 0;

    /* Send the signature block to the client */

    dwError = SendToken(nSocket, &TransmitBuffer);
    BAIL_ON_NTLM_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);
    TransmitBuffer.pvBuffer = NULL;
    TransmitBuffer.cbBuffer = 0;
#endif

    /* Delete context */

    dwError = NtlmClientDeleteSecurityContext(&Context);
    BAIL_ON_NTLM_ERROR(dwError);

finish:
    return dwError;
error:
    if (TransmitBuffer.pvBuffer)
    {
        free(TransmitBuffer.pvBuffer);
        TransmitBuffer.pvBuffer = NULL;
        TransmitBuffer.cbBuffer = 0;
    }
    if (WrapBuffers[1].pvBuffer)
    {
        free(WrapBuffers[1].pvBuffer);
        WrapBuffers[1].pvBuffer = NULL;
        WrapBuffers[1].cbBuffer = 0;
    }
    if (nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(&Context);
    }
    goto finish;
}

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN PNTLM_CRED_HANDLE pServerCreds,
    OUT PNTLM_CONTEXT_HANDLE pContext,
    IN DWORD AscFlags
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwLoopError = LW_ERROR_SUCCESS;
    DWORD nRetFlags = 0;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    TimeStamp Expiry;
    PNTLM_CONTEXT_HANDLE pContextHandle = NULL;
    INT nContextAcquired = 0;

    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&Expiry, 0, sizeof(TimeStamp));

    InputDesc.cBuffers = 1;
    //InputDesc.ulVersion = SECBUFFER_VERSION;
    InputDesc.pBuffers = &RecvTokenBuffer;

    OutputDesc.cBuffers = 1;
    //OutputDesc.ulVersion = SECBUFFER_VERSION;
    OutputDesc.pBuffers = &SendTokenBuffer;

    do
    {
        dwError = RecvToken(nSocket, &RecvTokenBuffer);
        BAIL_ON_NTLM_ERROR(dwError);

        printf("RECEIVED:\n");
        PrintHexDump(RecvTokenBuffer.cbBuffer, RecvTokenBuffer.pvBuffer);
        printf("\n");

        RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
        SendTokenBuffer.cbBuffer = 0;
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.BufferType = SECBUFFER_TOKEN;

        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError = NtlmClientAcceptSecurityContext(
            pServerCreds,
            pContextHandle,
            &InputDesc,
            AscFlags,
            gbFlipEndian ? NTLM_OTHER_DATA_REP : NTLM_NATIVE_DATA_REP,
            pContext,
            &OutputDesc,
            &nRetFlags,
            &Expiry
            );

        if (LW_ERROR_SUCCESS != dwLoopError && LW_WARNING_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_NTLM_ERROR(dwError);
        }

        if (LW_WARNING_CONTINUE_NEEDED == dwLoopError)
        {
            printf("Context partially accepted...\n");
            PrintHexDump(SendTokenBuffer.cbBuffer, SendTokenBuffer.pvBuffer);
            printf("\n");
        }
        else
        {
            printf("Context FULLY accepted!\n");
        }

        nContextAcquired = 1;

        pContextHandle = pContext;
        free(RecvTokenBuffer.pvBuffer);
        RecvTokenBuffer.pvBuffer = NULL;

        if (SendTokenBuffer.cbBuffer != 0)
        {
            dwError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_NTLM_ERROR(dwError);

            NtlmFreeContextBuffer(SendTokenBuffer.pvBuffer);
            SendTokenBuffer.pvBuffer = NULL;
            SendTokenBuffer.cbBuffer = 0;
        }

    } while (dwLoopError == LW_WARNING_CONTINUE_NEEDED);

finish:
    return dwError;
error:
    if (RecvTokenBuffer.pvBuffer)
    {
        free(RecvTokenBuffer.pvBuffer);
        RecvTokenBuffer.pvBuffer = NULL;
    }
    if (SendTokenBuffer.cbBuffer)
    {
        NtlmFreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;
    }
    if (nContextAcquired)
    {
        NtlmClientDeleteSecurityContext(pContext);
    }
    goto finish;
}

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
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
    pToken->pvBuffer = (char *) malloc(pToken->cbBuffer);

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
    INT nReturn = 0;
    PCHAR pTrav = NULL;

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
