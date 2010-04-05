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


#if defined(_WIN32)

#ifdef UNICODE
#undef UNICODE
#endif

#include <windows.h>
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

#define DUPE( x ) { "-" #x, ISC_REQ_ ## x, "ISC_REQ_"#x }

#define LW_ERROR_UNKNOWN -1

#define BAIL_ON_ERROR(dwError) \
    if (dwError)               \
    {                          \
        printf("%s()@%s:%d - error = %u (0x%08x)\n", __FUNCTION__, __FILE__, __LINE__, dwError, dwError); \
        goto error;            \
    }

typedef ULONG OM_uint32;

typedef struct _FLAGMAPPING
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
CallServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    IN PCHAR pSPN,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 DelegFlag,
    IN PCHAR pMsg,
    IN PCHAR pSecPkgName,
    IN INT nSignOnly
    );

DWORD
ConnectToServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    );

DWORD
ClientEstablishContext(
    IN PCHAR pSPN,
    IN INT nSocket,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 DelegFlag,
    OUT CtxtHandle *pSspiContext,
    IN PCHAR pSecPkgName,
    OUT OM_uint32 *pRetFlags
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
    PBYTE pBuffer
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

VOID
PrintHexDump(
    DWORD dwLength,
    PBYTE pBuffer
    );

int _cdecl
main(
    IN INT argc,
    IN PCHAR *argv
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    PCHAR pServerHost = "localhost";
    PCHAR pSPN = NULL;
    PCHAR pMsg = "Hello";
    PCHAR pSecPkgName = NTLMSP_NAME_A;
    USHORT usPort = 4444;
    INT nSignOnly = 0;
    INT  nIndex = 0;

    OM_uint32 DelegFlag =
        ISC_REQ_STREAM |
        //ISC_REQ_MUTUAL_AUTH |
        //ISC_REQ_REPLAY_DETECT |
        ISC_REQ_INTEGRITY |
        //ISC_REQ_CONFIDENTIALITY |
        ISC_REQ_ALLOCATE_MEMORY;

    FLAGMAPPING FlagMappings[] = {
        DUPE( CONFIDENTIALITY ),
        DUPE( DELEGATE ),
        DUPE( INTEGRITY ),
        DUPE( USE_SESSION_KEY ),
        DUPE( REPLAY_DETECT ),
        DUPE( SEQUENCE_DETECT ),
        DUPE( MUTUAL_AUTH )
    };

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
                BAIL_ON_ERROR(dwError);
            }

            usPort = (u_short)atoi(*argv);
        }
        else if (strcmp(*argv, "-sign") == 0)
        {
            nSignOnly = TRUE;
        }
        else if (strcmp(*argv, "-k") == 0)
        {
            pSecPkgName = MICROSOFT_KERBEROS_NAME_A;
        }
        else if (strcmp(*argv, "-spnego") == 0)
        {
            pSecPkgName = NEGOSSP_NAME;
        }
        else if (_strcmpi(*argv, "-d") == 0)
        {
            DelegFlag |= ISC_REQ_DELEGATE;
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
        else if (strcmp(*argv, "-h") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_ERROR(dwError);
            }

            pServerHost = *argv;
        }
        else if (strcmp(*argv, "-spn") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_ERROR(dwError);
            }

            pSPN = *argv;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                dwError = Usage();
                BAIL_ON_ERROR(dwError);
            }

            pMsg = *argv;
        }
        else
        {
            /*
            * Search for flags to use along the command line.
            * We do this so that we can determine what, if any,
            * flags are busted.
            */

            BOOLEAN bFound = FALSE;

            for (nIndex = 0; nIndex < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); nIndex++)
            {
                if (_strcmpi( *argv, FlagMappings[nIndex].name ) == 0)
                {
                    bFound = TRUE;
                    DelegFlag |= FlagMappings[nIndex].value ;
                    break;
                }
            }

            if (!bFound)
            {
                fprintf(
                    stderr,
                    "Unknown option %s\n",
                    *argv
                    );
                break;
            }
        }
        argc--; argv++;
    }

    for (nIndex = 0; nIndex < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); nIndex++)
    {
        if (DelegFlag & FlagMappings[nIndex].value)
        {
            printf("Context flag: %s\n", FlagMappings[nIndex].realname);
        }
    }

    if (nSignOnly)
    {
        printf("No encryption - sign only\n");
    }

    dwError = CallServer(
        pServerHost,
        usPort,
        pSPN,
        pServiceName,
        pServicePassword,
        pServiceRealm,
        DelegFlag,
        pMsg,
        pSecPkgName,
        nSignOnly
        );

    BAIL_ON_ERROR(dwError);

error:
    return dwError;
}

DWORD
Usage(VOID)
{
    fprintf(
        stderr,
        "Usage: sspi-client [-port port] [-sign] [-d] [-k] [-spnego] [-h host] [-m msg]\n"
        );
    fprintf(
        stderr,
        "       [-spn spn] [-c username password domain]\n"
        );

    return ERROR_INVALID_PARAMETER;
}

DWORD
CallServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    IN PCHAR pSPN,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 DelegFlag,
    IN PCHAR pMsg,
    IN PCHAR pSecPkgName,
    IN INT nSignOnly
    )
{
    DWORD dwError = ERROR_SUCCESS;
    SecBuffer WrapBuffers[3] = {0};
    INT nSocket = INVALID_SOCKET;
    ULONG nIndex = 0;
    OM_uint32 RetFlags = 0;
    OM_uint32 QopState = 0;
    INT nContextAcquired = 0;

    CtxtHandle Context;
    SecBuffer InBuffer;
    SecBuffer OutBuffer;
    SecBufferDesc InBufferDesc;
    SecPkgContext_Sizes Sizes;
    SecPkgContext_Names Names;
    SecPkgContext_AccessToken Token;
    SecPkgContext_NativeNames NativeNames;
    HANDLE TokenHandle;
    SecPkgContext_Authority Authority;
    TOKEN_USER User;
    DWORD NeedLength = 0;
    SecPkgContext_TargetInformation Target;
    SecPkgContext_SessionKey SessionKey;

    memset(&Context, 0, sizeof(CtxtHandle));
    memset(&InBuffer, 0, sizeof(SecBuffer));
    memset(&OutBuffer, 0, sizeof(SecBuffer));
    memset(&InBufferDesc, 0, sizeof(SecBufferDesc));
    memset(&Sizes, 0, sizeof(SecPkgContext_Sizes));

    /* Open connection */

    dwError = ConnectToServer(pHost, usPort, &nSocket);
    BAIL_ON_ERROR(dwError);

    /* Establish context */
    dwError = ClientEstablishContext(
            pSPN,
            nSocket,
            pServiceName,
            pServicePassword,
            pServiceRealm,
            DelegFlag,
            &Context,
            pSecPkgName,
            &RetFlags
            );

    BAIL_ON_ERROR(dwError);

    nContextAcquired = 1;

    dwError = QueryContextAttributes(
        &Context,
        SECPKG_ATTR_NAMES,
        &Names);
    BAIL_ON_ERROR(dwError);

    printf("Context is for user: %s\n", Names.sUserName);

    /*dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_ACCESS_TOKEN,
                    &Token);
    BAIL_ON_ERROR(dwError);

    dwError = GetTokenInformation(
                    &Context,
                    TokenUser,
                    &User,
                    sizeof(User),
                    &NeedLength);
    BAIL_ON_ERROR(dwError);*/

    /*dwError = QuerySecurityContextToken(
            &Context,
            &TokenHandle);
    BAIL_ON_ERROR(dwError);*/

    /*dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_AUTHORITY,
                    &Authority);
    BAIL_ON_ERROR(dwError);

    printf("Authority is %s\n", Authority.sAuthorityName);*/

    /*dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_NATIVE_NAMES,
                    &NativeNames);
    BAIL_ON_ERROR(dwError);*/

    dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_TARGET_INFORMATION,
                    &Target);
    if (dwError == SEC_E_UNSUPPORTED_FUNCTION)
    {
        printf("Querying server is unsupported\n");
    }
    else
    {
        BAIL_ON_ERROR(dwError);

        printf("Context is for server %s\n", Target.MarshalledTargetInfo);
    }

    dwError = QueryContextAttributes(
        &Context,
        SECPKG_ATTR_SESSION_KEY,
        &SessionKey);

    if(!dwError)
    {
        printf("Session Key: ");
        for(nIndex = 0; nIndex < SessionKey.SessionKeyLength; nIndex++)
        {
            printf("%02X ", SessionKey.SessionKey[nIndex]);
        }
        printf("\n");
    }

    dwError = QueryContextAttributes(
        &Context,
        SECPKG_ATTR_SIZES,
        &Sizes
        );

    BAIL_ON_ERROR(dwError);

    /* Seal the message */
    InBuffer.pvBuffer = pMsg;
    InBuffer.cbBuffer = (ULONG)strlen(pMsg) + 1;

    //
    // Prepare to encrypt the message
    //

    InBufferDesc.cBuffers = 3;
    InBufferDesc.pBuffers = WrapBuffers;
    InBufferDesc.ulVersion = SECBUFFER_VERSION;

    WrapBuffers[0].cbBuffer = Sizes.cbSecurityTrailer;
    WrapBuffers[0].BufferType = SECBUFFER_TOKEN;
    WrapBuffers[0].pvBuffer = malloc(Sizes.cbSecurityTrailer);

    if (WrapBuffers[0].pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
    }

    WrapBuffers[1].BufferType = SECBUFFER_DATA;
    WrapBuffers[1].cbBuffer = InBuffer.cbBuffer;
    WrapBuffers[1].pvBuffer = malloc(WrapBuffers[1].cbBuffer);

    if (WrapBuffers[1].pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
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
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
    }

    if (nSignOnly)
    {
        printf("Signing only (not encrypting)\n");
    }

    dwError = EncryptMessage(
        &Context,
        nSignOnly ? SECQOP_WRAP_NO_ENCRYPT : 0,
        &InBufferDesc,
        0);

    BAIL_ON_ERROR(dwError);

    //
    // Create the mesage to send to server
    //

    OutBuffer.cbBuffer = WrapBuffers[0].cbBuffer + WrapBuffers[1].cbBuffer + WrapBuffers[2].cbBuffer;
    OutBuffer.pvBuffer = malloc(OutBuffer.cbBuffer);

    if (OutBuffer.pvBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        BAIL_ON_ERROR(dwError);
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
    BAIL_ON_ERROR(dwError);
    printf("Encrypted and sent '%s' to server\n", pMsg);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;
    OutBuffer.cbBuffer = 0;
    free(WrapBuffers[0].pvBuffer);
    WrapBuffers[0].pvBuffer = NULL;
    free(WrapBuffers[1].pvBuffer);
    WrapBuffers[1].pvBuffer = NULL;

    /* Read signature block into OutBuffer */
    dwError = RecvToken(nSocket, &OutBuffer);
    BAIL_ON_ERROR(dwError);

    /* Verify signature block */

    InBufferDesc.cBuffers = 2;
    WrapBuffers[0] = InBuffer;
    WrapBuffers[0].BufferType = SECBUFFER_DATA;
    WrapBuffers[1] = OutBuffer;
    WrapBuffers[1].BufferType = SECBUFFER_TOKEN;

    dwError = VerifySignature(&Context, &InBufferDesc, 0, &QopState);
    BAIL_ON_ERROR(dwError);

    free(OutBuffer.pvBuffer);
    OutBuffer.pvBuffer = NULL;

    /* Delete context */
    dwError = DeleteSecurityContext(&Context);
    BAIL_ON_ERROR(dwError);

    closesocket(nSocket);

finish:
    return dwError;
error:
    if (nContextAcquired)
    {
        DeleteSecurityContext(&Context);
    }
    if (INVALID_SOCKET != nSocket)
    {
        closesocket(nSocket);
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

    printf("Failed with error %d\n", dwError);
    goto finish;
}

DWORD
ConnectToServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    )
{
    DWORD dwError = ERROR_SUCCESS;
    USHORT usVersionRequired = 0x0101;
    struct sockaddr_in sAddr;
    struct hostent *pHostEnt;
    WSADATA SocketData;

    memset(&sAddr, 0, sizeof(struct sockaddr_in));
    memset(&SocketData, 0, sizeof(WSADATA));

    *pSocket = INVALID_SOCKET;

    dwError = WSAStartup(usVersionRequired, &SocketData);

    if (0 != dwError)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    pHostEnt = gethostbyname(pHost);

    if (pHostEnt == NULL)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    sAddr.sin_family = pHostEnt->h_addrtype;
    memcpy((PCHAR)&sAddr.sin_addr, pHostEnt->h_addr, sizeof(sAddr.sin_addr));
    sAddr.sin_port = htons(usPort);

    *pSocket = (INT)socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == *pSocket)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

    dwError = connect(*pSocket, (struct sockaddr *)&sAddr, sizeof(sAddr));
    if (dwError == SOCKET_ERROR)
    {
        dwError = WSAGetLastError();
        BAIL_ON_ERROR(dwError);
    }

finish:
    return dwError;
error:
    if (INVALID_SOCKET != *pSocket)
    {
        closesocket(*pSocket);
        *pSocket = INVALID_SOCKET;
    }
    goto finish;
}

DWORD
ClientEstablishContext(
    IN PCHAR pSPN,
    IN INT nSocket,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 DelegFlag,
    OUT CtxtHandle *pSspiContext,
    IN PCHAR pSecPkgName,
    OUT OM_uint32 *pRetFlags
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwLoopError = ERROR_SUCCESS;
    PCtxtHandle pContextHandle = NULL;
    INT nCredentialsAcquired = 0;
    INT nContextAcquired = 0;

    SecBuffer SendTokenBuffer;
    SecBuffer RecvTokenBuffer;
    SecBufferDesc InputDesc;
    SecBufferDesc OutputDesc;
    CredHandle CredHandle;
    TimeStamp Expiry;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;
    PSEC_WINNT_AUTH_IDENTITY pAuthId = NULL;

    memset(&SendTokenBuffer, 0, sizeof(SecBuffer));
    memset(&RecvTokenBuffer, 0, sizeof(SecBuffer));
    memset(&InputDesc, 0, sizeof(SecBufferDesc));
    memset(&OutputDesc, 0, sizeof(SecBufferDesc));
    memset(&CredHandle, 0, sizeof(CredHandle));
    memset(&Expiry, 0, sizeof(TimeStamp));
    memset(&AuthIdentity, 0, sizeof(AuthIdentity));

    memset(pSspiContext, 0, sizeof(CtxtHandle));
    *pRetFlags = 0;

    InputDesc.cBuffers = 1;
    InputDesc.pBuffers = &RecvTokenBuffer;
    InputDesc.ulVersion = SECBUFFER_VERSION;

    RecvTokenBuffer.BufferType = SECBUFFER_TOKEN;
    RecvTokenBuffer.cbBuffer = 0;
    RecvTokenBuffer.pvBuffer = NULL;

    OutputDesc.cBuffers = 1;
    OutputDesc.pBuffers = &SendTokenBuffer;
    OutputDesc.ulVersion = SECBUFFER_VERSION;

    SendTokenBuffer.BufferType = SECBUFFER_TOKEN;
    SendTokenBuffer.cbBuffer = 0;
    SendTokenBuffer.pvBuffer = NULL;

    CredHandle.dwLower = 0;
    CredHandle.dwUpper = 0;

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
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    dwError = AcquireCredentialsHandle(
        "join@CORPQA.CENTERIS.COM",                       // no principal name
        pSecPkgName,                // package name
        SECPKG_CRED_OUTBOUND,
        NULL,                       // no logon id
        pAuthId,
        NULL,                       // no get key fn
        NULL,                       // noget key arg
        &CredHandle,
        &Expiry
        );

    BAIL_ON_ERROR(dwError);

    nCredentialsAcquired = 1;

   /*
    * Perform the context-establishement loop.
    */

    pSspiContext->dwLower = 0;
    pSspiContext->dwUpper = 0;

    do
    {
        // we need to use dwLoopErr in this case because we may get
        // back a "continue" command.  In those cases, we still
        // need dwError to be used and set seperatly based on other
        // calls.
        dwLoopError =
            InitializeSecurityContext(
                &CredHandle,
                pContextHandle,
                pSPN,
                DelegFlag,
                0,          // reserved
                SECURITY_NATIVE_DREP,
                &InputDesc,
                0,          // reserved
                pSspiContext,
                &OutputDesc,
                pRetFlags,
                &Expiry
                );

        if (SEC_E_OK != dwLoopError && SEC_I_CONTINUE_NEEDED != dwLoopError)
        {
            dwError = dwLoopError;
            BAIL_ON_ERROR(dwError);
        }

        nContextAcquired = 1;

        if (SEC_I_CONTINUE_NEEDED == dwLoopError)
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
            printf("Flags used:\n");
            DumpNtlmFlags(*pRetFlags);
            printf("\n");
        }

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
            BAIL_ON_ERROR(dwError);
        }

        FreeContextBuffer(SendTokenBuffer.pvBuffer);
        SendTokenBuffer.pvBuffer = NULL;
        SendTokenBuffer.cbBuffer = 0;

        if (SEC_I_CONTINUE_NEEDED == dwLoopError)
        {
            dwError = RecvToken(nSocket, &RecvTokenBuffer);
            BAIL_ON_ERROR(dwError);
            printf("RECEIVED:\n");
            PrintHexDump(RecvTokenBuffer.cbBuffer, RecvTokenBuffer.pvBuffer);
            printf("\n");
        }

    } while (dwLoopError == SEC_I_CONTINUE_NEEDED);

    FreeCredentialsHandle(&CredHandle);

finish:
    return dwError;
error:
    if (nCredentialsAcquired)
    {
        FreeCredentialsHandle(&CredHandle);
    }
    if (nContextAcquired)
    {
        DeleteSecurityContext(pSspiContext);
        memset(pSspiContext, 0, sizeof(CtxtHandle));
    }
    if (RecvTokenBuffer.pvBuffer)
    {
        free(RecvTokenBuffer.pvBuffer);
    }
    if (SendTokenBuffer.cbBuffer)
    {
        FreeContextBuffer(SendTokenBuffer.pvBuffer);
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
    int nReturn = 0;
    char *pTrav = NULL;

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

    if (dwSize == 0)
    {
	    return;
    }

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
    INT cbLine;
    PBYTE pToken = pBuffer;

    for (index = 0; dwLength;
        dwLength -= count, pBuffer += count, index += count)
    {
        count = (dwLength > 16) ? 16:dwLength;

        sprintf_s(rgbLine, 100, "%4.4x  ",index);
        cbLine = 6;

        for (i=0;i<count;i++)
        {
            rgbLine[cbLine++] = rgbDigits[pBuffer[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[pBuffer[i] & 0x0f];
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
            if (pBuffer[i] < 32 || pBuffer[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = pBuffer[i];
            }
        }

        rgbLine[cbLine++] = 0;
        printf("%s\n", rgbLine);
    }

    DumpNtlmMessage(pToken, dwLength);
}
