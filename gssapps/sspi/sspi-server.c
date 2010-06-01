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

#ifdef UNICODE
#undef UNICODE
#endif

#if defined(_WIN32)

#include <windows.h>
#include <winerror.h>
#include <rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#define SECURITY_WIN32
#include <security.h>
#include <ntsecapi.h>
#include <stddef.h>
#include <sys/types.h>

#else

#include "config.h"
#include <lw/base.h>
#include <ntlm/sspintlm.h>
#include <wc16str.h>
#include <lwdef.h>
#include <lwerror.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>

#endif

#define DUPE( x ) { "-" #x, ASC_REQ_ ## x }

#define BAIL_ON_ERROR(dwError) \
    if (dwError)               \
    {                          \
        printf("EE = %d, error = %u (0x%08x)\n", __LINE__, dwError, dwError); \
        goto error;            \
    }

typedef ULONG OM_uint32;

typedef struct
{
   PCHAR     name;
   OM_uint32 value;
   PCHAR     realname;
} FLAGMAPPING, *PFLAGMAPPING;

typedef struct _NTLM_SEC_BUFFER
{
    USHORT usLength;    // number of bytes used
    USHORT usMaxLength; // true size of buffer in bytes
    DWORD  dwOffset;
} NTLM_SEC_BUFFER, *PNTLM_SEC_BUFFER;

typedef struct _NTLM_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
} NTLM_MESSAGE, *PNTLM_MESSAGE;

typedef struct _NTLM_NEGOTIATE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    DWORD NtlmFlags;
    // Optional Supplied Domain NTLM_SEC_BUFFER
    // Optional Supplied Workstation NTLM_SEC_BUFFER
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_NEGOTIATE_MESSAGE, *PNTLM_NEGOTIATE_MESSAGE;

typedef struct _NTLM_CHALLENGE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    NTLM_SEC_BUFFER Target;
    DWORD NtlmFlags;
    UCHAR Challenge[8];
    // Optional Context 8 bytes
    // Optional Target Information NTLM_SEC_BUFFER
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_CHALLENGE_MESSAGE, *PNTLM_CHALLENGE_MESSAGE;

typedef struct _NTLM_RESPONSE_MESSAGE
{
    UCHAR NtlmSignature[8];
    DWORD MessageType;
    NTLM_SEC_BUFFER LmResponse;
    NTLM_SEC_BUFFER NtResponse;
    NTLM_SEC_BUFFER AuthTargetName;
    NTLM_SEC_BUFFER UserName;
    NTLM_SEC_BUFFER Workstation;
    // Optional Session Key NTLM_SEC_BUFFER
    // Optional Flags 4 bytes
    // Optional OS Version 8 bytes
    // Optional Data
} NTLM_RESPONSE_MESSAGE, *PNTLM_RESPONSE_MESSAGE;

DWORD
Usage(VOID);

DWORD
ServerAcquireCreds(
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN PCHAR pSecPkgName,
    OUT CredHandle *pServerCreds
    );

DWORD
CreateSocket(
    IN USHORT uPort,
    OUT PINT pSocket
    );

DWORD
SignServer(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    IN OM_uint32 AscFlags
    );

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    OUT CtxtHandle *pContext,
    IN OM_uint32 AscFlags
    );

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    );

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesWritten
    );

DWORD
RecvToken(
    IN INT nSocket,
    OUT PSecBuffer pToken
    );

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nBytes,
    OUT PINT nBytesRead
    );

VOID
PrintHexDump(
    DWORD dwLength,
    PBYTE pBuffer,
    INT bIsNtlmMessage
    );

VOID
DumpNtlmFlags(
    DWORD dwFlags
    );

VOID
DumpNegMessage(
    PNTLM_NEGOTIATE_MESSAGE pMsg,
    DWORD dwSize
    );

VOID
DumpChlngMessage(
    PNTLM_CHALLENGE_MESSAGE pMsg,
    DWORD dwSize
    );

VOID
DumpRespMessage(
    PNTLM_RESPONSE_MESSAGE pMsg,
    DWORD dwSize
    );

VOID
DumpNtlmMessage(
    PBYTE pBuffer,
    DWORD dwSize
    );

INT _cdecl
main(
    IN INT argc,
    IN PCHAR* argv
    )
{
    DWORD dwError = ERROR_SUCCESS;
    INT nListenSocket = INVALID_SOCKET;
    INT nAcceptSocket = INVALID_SOCKET;
    USHORT usVersionRequired = 0x0101;
    USHORT usPort = 4444;
    INT nOnce = 0;
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    WSADATA SocketData;
    CredHandle ServerCreds;
    INT nServerCredsAcquired = 0;
    INT nSocketsStarted = 0;
    OM_uint32 AscFlags =
        //ASC_REQ_MUTUAL_AUTH |
        //ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_INTEGRITY |
        ASC_REQ_ALLOCATE_MEMORY;
    PCHAR pSecPkgName = NTLMSP_NAME_A;

    FLAGMAPPING FlagMappings[] = {
        DUPE( CONFIDENTIALITY ),
        DUPE( DELEGATE ),
        DUPE( INTEGRITY ),
        DUPE( USE_SESSION_KEY ),
        DUPE( REPLAY_DETECT ),
        DUPE( SEQUENCE_DETECT )
    };

    memset(&SocketData, 0, sizeof(WSADATA));
    memset(&ServerCreds, 0, sizeof(CredHandle));

    argc--; argv++;

    while (argc)
    {
        if (strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;
            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_ERROR(dwError);
            }
            usPort = (u_short)atoi(*argv);
        }
        else if (strcmp(*argv, "-once") == 0)
        {
            nOnce = 1;
        }
        else if (strcmp(*argv, "-k") == 0)
        {
            pSecPkgName = MICROSOFT_KERBEROS_NAME_A;
        }
        else if (strcmp(*argv, "-spnego") == 0)
        {
            pSecPkgName = NEGOSSP_NAME;
        }
        else if (strcmp(*argv, "-c") == 0)
        {
            argv++;
            argc--;

            if (argc < 3)
            {
                dwError = Usage();
                BAIL_ON_ERROR(dwError);
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
            int i;
            BOOLEAN bFound = FALSE;

            for (i = 0; i < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); i++)
            {
                if (_strcmpi( *argv, FlagMappings[ i ].name ) == 0)
                {
                    bFound = TRUE;
                    AscFlags |= FlagMappings[ i ].value ;
                    break;
                }
            }

            if (!bFound)
            {
                break;
            }
        }

        argc--; argv++;
    }



    dwError = WSAStartup(usVersionRequired, &SocketData);
    if (0 != dwError)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    nSocketsStarted = 1;

    dwError = CreateSocket(usPort, &nListenSocket);

    BAIL_ON_ERROR(dwError);

    do
    {
        dwError = ServerAcquireCreds(
            pServiceName,
            pServicePassword,
            pServiceRealm,
            pSecPkgName,
            &ServerCreds
            );
        BAIL_ON_ERROR(dwError);

        nServerCredsAcquired = 1;

        /* Accept a TCP connection */
        printf("Listening...\n");
        nAcceptSocket = (int)accept(nListenSocket, NULL, 0);

        if (INVALID_SOCKET == nAcceptSocket)
        {
            dwError = WSAGetLastError();
            BAIL_ON_ERROR(dwError);
        }

        dwError = SignServer(
            nAcceptSocket,
            &ServerCreds,
            AscFlags
            );

        if (dwError)
        {
            printf("Finished with error code: %d\n", dwError);
        }

        if (nServerCredsAcquired)
        {
            FreeCredentialsHandle(&ServerCreds);
            nServerCredsAcquired = 0;
        }

        closesocket(nAcceptSocket);

    } while (!nOnce);

    closesocket(nListenSocket);

    FreeCredentialsHandle(&ServerCreds);

    WSACleanup();

finish:
    return dwError;
error:
    if (nServerCredsAcquired)
    {
        FreeCredentialsHandle(&ServerCreds);
    }
    if (INVALID_SOCKET != nListenSocket)
    {
        closesocket(nListenSocket);
    }
    if (INVALID_SOCKET != nAcceptSocket)
    {
        closesocket(nAcceptSocket);
    }
    if (nSocketsStarted)
    {
        WSACleanup();
    }
    goto finish;
}

DWORD
Usage(VOID)
{
    fprintf(stderr, "Usage: sspi-server [-port port] [-k] [-spnego]\n");
    fprintf(stderr, "       [-c service_name service_password service_realm]\n");

    return ERROR_INVALID_PARAMETER;
}

DWORD
ServerAcquireCreds(
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN PCHAR pSecPkgName,
    OUT CredHandle *pServerCreds
    )
{
    DWORD dwError = SEC_E_OK;
    TimeStamp Expiry;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;
    PSEC_WINNT_AUTH_IDENTITY pAuthId = NULL;

    memset(&Expiry, 0, sizeof(TimeStamp));
    memset(&AuthIdentity, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_W));

    memset(pServerCreds, 0, sizeof(CredHandle));

    if (pServiceName)
    {
        AuthIdentity.User = pServiceName;
        AuthIdentity.UserLength = (ULONG)strlen(pServiceName);
        pAuthId = &AuthIdentity;
    }

    if (pServicePassword)
    {
        AuthIdentity.Password = pServicePassword;
        AuthIdentity.PasswordLength = (ULONG)strlen(pServicePassword);
        pAuthId = &AuthIdentity;
    }

    if (pServiceRealm)
    {
        AuthIdentity.Domain = pServiceRealm;
        AuthIdentity.DomainLength = (ULONG)strlen(pServiceRealm);
        pAuthId = &AuthIdentity;
    }

    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    dwError = AcquireCredentialsHandle(
        NULL,
        pSecPkgName,
        SECPKG_CRED_INBOUND,
        NULL,                       // no logon id
        pAuthId,
        NULL,                       // no get key fn
        NULL,                       // no get key arg
        pServerCreds,
        &Expiry
        );

    BAIL_ON_ERROR(dwError);

error:
    return dwError;
}

DWORD
CreateSocket(
    IN USHORT uPort,
    OUT PINT pSocket
    )
{
    DWORD dwError = ERROR_SUCCESS;
    struct sockaddr_in sAddr;
    INT nOn = 1;

    *pSocket = INVALID_SOCKET;
    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(uPort);
    sAddr.sin_addr.s_addr = INADDR_ANY;

    *pSocket = (int)socket(AF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == *pSocket)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
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
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    dwError = bind
        (
        *pSocket,
        (struct sockaddr *) &sAddr,
        sizeof(sAddr)
        );

    if (SOCKET_ERROR == dwError)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    dwError = listen(*pSocket, 5);

    if (SOCKET_ERROR == dwError)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (*pSocket != INVALID_SOCKET)
    {
        closesocket(*pSocket);
        *pSocket = INVALID_SOCKET;
    }

    goto finish;
}

DWORD
SignServer(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    IN OM_uint32 AscFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    ULONG ulQop = 0;
    INT nContextAcquired = 0;
    ULONG nIndex = 0;
    SecBuffer TransmitBuffer;
    SecBuffer MsgBuffer;
    SecBuffer WrapBuffers[2] = {0};
    SecBufferDesc WrapBufferDesc;
    CtxtHandle Context;
    SecPkgContext_Names Names;
    SecPkgContext_Sizes Sizes;
    SecPkgContext_SessionKey SessionKey;

    memset(&TransmitBuffer, 0, sizeof(SecBuffer));
    memset(&MsgBuffer, 0, sizeof(SecBuffer));
    memset(&WrapBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Context, 0, sizeof(CtxtHandle));
    memset(&Names, 0, sizeof(SecPkgContext_Names));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));
    memset(&SessionKey, 0, sizeof(SecPkgContext_SessionKey));

    /* Establish a context with the client */
    dwError = ServerEstablishContext(
        nSocket,
        pServerCreds,
        &Context,
        AscFlags
        );

    BAIL_ON_ERROR(dwError);

    dwError = QueryContextAttributes(
        &Context,
        SECPKG_ATTR_NAMES,
        &Names
        );

    if (dwError)
    {
        printf("Unable to query context: %d\n", dwError);
    }
    else
    {
        printf("Context is for user: %s\n", Names.sUserName);
    }

    dwError = QueryContextAttributes(
        &Context,
        SECPKG_ATTR_SESSION_KEY,
        &SessionKey
        );

    if (dwError)
    {
        printf("Unable to query context: %X\n", dwError);
    }
    else
    {
        printf("Session Key: ");
        for(nIndex = 0; nIndex < SessionKey.SessionKeyLength; nIndex++)
        {
            printf("%02X ", SessionKey.SessionKey[nIndex]);
        }
        printf("\n\n");
    }

    printf("Server accepted context successfully!\n");

    // for clean up... once we've established a context, we must clean it up on
    // future failures.
    nContextAcquired = 1;

    dwError = QueryContextAttributes
        (
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_ERROR(dwError);

    /* Receive the sealed message token */
    dwError = RecvToken(nSocket, &TransmitBuffer);
    BAIL_ON_ERROR(dwError);

    printf("RECEIVED:\n");
    PrintHexDump(TransmitBuffer.cbBuffer, TransmitBuffer.pvBuffer, FALSE);
    printf("\n");

    WrapBufferDesc.cBuffers = 2;
    WrapBufferDesc.pBuffers = WrapBuffers;
    WrapBufferDesc.ulVersion = SECBUFFER_VERSION;

    WrapBuffers[0].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[0].pvBuffer = TransmitBuffer.pvBuffer;
    WrapBuffers[0].cbBuffer = Sizes.cbMaxSignature;

    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = TransmitBuffer.cbBuffer - Sizes.cbMaxSignature;
    WrapBuffers[1].pvBuffer = (PBYTE)TransmitBuffer.pvBuffer + Sizes.cbMaxSignature;

    dwError = DecryptMessage(
        &Context,
        &WrapBufferDesc,
        0,                  // no sequence number
        &ulQop
        );

    if (dwError)
    {
        // When we bail, this var will try to be freed which is a bad thing...
        // the memory will be freed when TransmitBuffer is freed, so it's ok
        // to set this buffer to NULL here.
        WrapBuffers[1].pvBuffer = NULL;
        printf("Unable to decrypt message\n");
    }

    BAIL_ON_ERROR(dwError);

    MsgBuffer = WrapBuffers[1];

    printf("Received message '%.*s' from client\n", MsgBuffer.cbBuffer, MsgBuffer.pvBuffer);

    /* Produce a signature block for the message */

    WrapBuffers[0] = MsgBuffer;

    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[1].cbBuffer = Sizes.cbMaxSignature;
    WrapBuffers[1].pvBuffer = malloc(Sizes.cbMaxSignature);

    if (WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
    }

    dwError = MakeSignature(
        &Context,
        0,
        &WrapBufferDesc,
        0
        );

    if (dwError)
    {
        printf("Unable to MakeSignature");
    }

    BAIL_ON_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);

    TransmitBuffer = WrapBuffers[1];
    WrapBuffers[1].pvBuffer = NULL;
    WrapBuffers[1].cbBuffer = 0;

    /* Send the signature block to the client */

    dwError = SendToken(nSocket, &TransmitBuffer);
    BAIL_ON_ERROR(dwError);

    free(TransmitBuffer.pvBuffer);
    TransmitBuffer.pvBuffer = NULL;
    TransmitBuffer.cbBuffer = 0;

    /* Delete context */

    dwError = DeleteSecurityContext( &Context );
    BAIL_ON_ERROR(dwError);

finish:
    return dwError;
error:
    if (Names.sUserName)
    {
        FreeContextBuffer(Names.sUserName);
    }
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
        DeleteSecurityContext(&Context);
    }
    goto finish;
}

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    OUT CtxtHandle *pContext,
    IN OM_uint32 AscFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLoopError = ERROR_SUCCESS;
    ULONG nRetFlags = 0;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    TimeStamp Expiry;
    PCtxtHandle pContextHandle = NULL;
    INT nContextAcquired = 0;

    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&Expiry, 0, sizeof(TimeStamp));

    memset(pContext, 0, sizeof(CtxtHandle));

    InputDesc.cBuffers = 1;
    InputDesc.ulVersion = SECBUFFER_VERSION;
    InputDesc.pBuffers = &RecvTokenBuffer;

    OutputDesc.cBuffers = 1;
    OutputDesc.ulVersion = SECBUFFER_VERSION;
    OutputDesc.pBuffers = &SendTokenBuffer;

    do
    {
        dwError = RecvToken(nSocket, &RecvTokenBuffer);
        BAIL_ON_ERROR(dwError);

        printf("RECEIVED:\n");
        PrintHexDump(RecvTokenBuffer.cbBuffer, RecvTokenBuffer.pvBuffer, TRUE);
        printf("\n");

        RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
        SendTokenBuffer.cbBuffer = 0;
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.BufferType = SECBUFFER_TOKEN;

        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError = AcceptSecurityContext(
            pServerCreds,
            pContextHandle,
            &InputDesc,
            AscFlags,
            SECURITY_NATIVE_DREP,
            pContext,
            &OutputDesc,
            &nRetFlags,
            &Expiry
            );

        if (SEC_E_OK != dwLoopError && SEC_I_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_ERROR(dwError);
        }

        if (SEC_I_CONTINUE_NEEDED == dwLoopError)
        {
            printf("Context partially accepted...\n");
            PrintHexDump(SendTokenBuffer.cbBuffer, SendTokenBuffer.pvBuffer, TRUE);
            printf("\n");
        }
        else
        {
            printf("Context FULLY accepted!\n");
            printf("Flags used:\n");
            DumpNtlmFlags(nRetFlags);
            printf("\n");
        }

        nContextAcquired = 1;

        pContextHandle = pContext;
        free(RecvTokenBuffer.pvBuffer);
        RecvTokenBuffer.pvBuffer = NULL;

        if (SendTokenBuffer.cbBuffer != 0)
        {
            dwError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_ERROR(dwError);

            FreeContextBuffer(SendTokenBuffer.pvBuffer);
            SendTokenBuffer.pvBuffer = NULL;
        }

    } while (dwLoopError == SEC_I_CONTINUE_NEEDED);

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
        FreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;
    }
    if (nContextAcquired)
    {
        DeleteSecurityContext(pContext);
    }
    goto finish;
}

DWORD
SendToken(
    IN INT nSocket,
    IN PSecBuffer pToken
    )
{
    DWORD dwError = ERROR_SUCCESS;
    ULONG ulLen = 0;
    INT nBytesWritten = 0;

    ulLen = htonl(pToken->cbBuffer);

    dwError = WriteAll(
        nSocket,
        (PCHAR)&ulLen,
        4,
        &nBytesWritten
        );

    BAIL_ON_ERROR(dwError);

    if (4 != nBytesWritten)
    {
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = WriteAll(
        nSocket,
        pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesWritten
        );

    BAIL_ON_ERROR(dwError);

    if (nBytesWritten != pToken->cbBuffer)
    {
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
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
    DWORD dwError = ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *nBytesWritten = 0;

    for (pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = send(nSocket, pTrav, nBytes, 0);

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
    DWORD dwError = ERROR_SUCCESS;
    INT nBytesRead = 0;

    memset(pToken, 0, sizeof(SecBuffer));

    pToken->BufferType = SECBUFFER_TOKEN;

    dwError = ReadAll(
        nSocket,
        (PCHAR)&pToken->cbBuffer,
        4,
        &nBytesRead
        );

    BAIL_ON_ERROR(dwError);

    if (4 != nBytesRead)
    {
        printf("%u\n", nBytesRead);
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

    pToken->cbBuffer = ntohl(pToken->cbBuffer);
    pToken->pvBuffer = (char *) malloc(pToken->cbBuffer);

    if (pToken->pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
    }

    dwError = ReadAll(
        nSocket,
        (PCHAR)pToken->pvBuffer,
        pToken->cbBuffer,
        &nBytesRead
        );

    BAIL_ON_ERROR(dwError);

    if (nBytesRead != pToken->cbBuffer)
    {
        dwError = ERROR_INCORRECT_SIZE;
        BAIL_ON_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (pToken->pvBuffer)
    {
        free(pToken->pvBuffer);
        memset(pToken, 0, sizeof(SecBuffer));
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
    DWORD dwError = ERROR_SUCCESS;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    memset(pBuffer, 0, nBytes);
    *nBytesRead = 0;

    for (pTrav = pBuffer; nBytes; pTrav += nReturn, nBytes -= nReturn)
    {
        nReturn = recv(nSocket, pTrav, nBytes, 0);

        if (nReturn < 0)
        {
            dwError = GetLastError();
            BAIL_ON_ERROR(dwError);
        }

        if (nReturn == 0)
        {
            printf("Connection closed\n");
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
    PBYTE pBuffer,
    INT bIsNtlmMessage
    )
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    int cbLine;
    PBYTE pToken = pBuffer;

    for (index = 0; dwLength;
        dwLength -= count, pBuffer += count, index += count)
    {
        count = (dwLength > 16) ? 16:dwLength;

        sprintf(rgbLine, "%4.4x  ",index);
        cbLine = 6;

        for (i=0;i<count;i++)
        {
            rgbLine[(cbLine)++] = rgbDigits[pBuffer[i] >> 4];
            rgbLine[(cbLine)++] = rgbDigits[pBuffer[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[(cbLine)++] = ':';
            }
            else
            {
                rgbLine[(cbLine)++] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[(cbLine)++] = ' ';
            rgbLine[(cbLine)++] = ' ';
            rgbLine[(cbLine)++] = ' ';
        }

        rgbLine[(cbLine)++] = ' ';

        for (i = 0; i < count; i++)
        {
            if (pBuffer[i] < 32 || pBuffer[i] > 126)
            {
                rgbLine[(cbLine)++] = '.';
            }
            else
            {
                rgbLine[(cbLine)++] = pBuffer[i];
            }
        }

        rgbLine[(cbLine)++] = 0;
        printf("%s\n", rgbLine);
    }

    if (bIsNtlmMessage)
    {
        DumpNtlmMessage(pToken, dwLength);
    }
}
