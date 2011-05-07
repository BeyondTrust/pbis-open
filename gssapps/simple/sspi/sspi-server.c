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

#define INIT_FLAGMAPPING(Flag) _INIT_FLAGMAPPING(ASC_REQ_, Flag)

static
DWORD
Usage(
    VOID
    );

static
DWORD
ServerAcquireCreds(
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN PCHAR pSecPkgName,
    OUT CredHandle *pServerCreds
    );

static
DWORD
CreateSocket(
    IN USHORT uPort,
    OUT PINT pSocket
    );

static
DWORD
SignServer(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    IN ULONG AscFlags
    );

static
DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    OUT CtxtHandle *pContext,
    IN ULONG AscFlags
    );

INT
_cdecl
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
    ULONG AscFlags =
        //ASC_REQ_MUTUAL_AUTH |
        //ASC_REQ_CONFIDENTIALITY |
        ASC_REQ_INTEGRITY |
        ASC_REQ_ALLOCATE_MEMORY;
    PCHAR pSecPkgName = NTLMSP_NAME_A;

    FLAGMAPPING FlagMappings[] = {
        INIT_FLAGMAPPING( CONFIDENTIALITY ),
        INIT_FLAGMAPPING( DELEGATE ),
        INIT_FLAGMAPPING( INTEGRITY ),
        INIT_FLAGMAPPING( USE_SESSION_KEY ),
        INIT_FLAGMAPPING( REPLAY_DETECT ),
        INIT_FLAGMAPPING( SEQUENCE_DETECT )
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

static
DWORD
Usage(VOID)
{
    fprintf(stderr, "Usage: sspi-server [-port port] [-k] [-spnego]\n");
    fprintf(stderr, "       [-c service_name service_password service_realm]\n");

    return ERROR_INVALID_PARAMETER;
}

static
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
    SEC_WINNT_AUTH_IDENTITY_A AuthIdentity;
    PSEC_WINNT_AUTH_IDENTITY_A pAuthId = NULL;

    memset(&Expiry, 0, sizeof(TimeStamp));
    memset(&AuthIdentity, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_A));

    memset(pServerCreds, 0, sizeof(CredHandle));

    if (pServiceName)
    {
        AuthIdentity.User = (PBYTE) pServiceName;
        AuthIdentity.UserLength = (ULONG)strlen(pServiceName);
        pAuthId = &AuthIdentity;
    }

    if (pServicePassword)
    {
        AuthIdentity.Password = (PBYTE) pServicePassword;
        AuthIdentity.PasswordLength = (ULONG)strlen(pServicePassword);
        pAuthId = &AuthIdentity;
    }

    if (pServiceRealm)
    {
        AuthIdentity.Domain = (PBYTE) pServiceRealm;
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

static
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

static
DWORD
SignServer(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    IN ULONG AscFlags
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
    DumpBuffer(TransmitBuffer.pvBuffer, TransmitBuffer.cbBuffer);
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

static
DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN CredHandle *pServerCreds,
    OUT CtxtHandle *pContext,
    IN ULONG AscFlags
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

    printf("ASC flags requested (0x%08x):\n", AscFlags);
    DumpAscReqFlags(AscFlags);

    do
    {
        dwError = RecvToken(nSocket, &RecvTokenBuffer);
        BAIL_ON_ERROR(dwError);

        printf("RECEIVED:\n");
        DumpBuffer(RecvTokenBuffer.pvBuffer, RecvTokenBuffer.cbBuffer);
        DumpNtlmMessage(RecvTokenBuffer.pvBuffer, RecvTokenBuffer.cbBuffer);
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
            DumpBuffer(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
            DumpNtlmMessage(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
            if (nRetFlags)
            {
                printf("ASC flags returned (0x%08x):\n", nRetFlags);
                DumpAscRetFlags(nRetFlags);
            }
            printf("\n");
        }
        else
        {
            printf("Context FULLY accepted!\n");
            printf("ASC flags returned (0x%08x):\n", nRetFlags);
            DumpAscRetFlags(nRetFlags);
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
