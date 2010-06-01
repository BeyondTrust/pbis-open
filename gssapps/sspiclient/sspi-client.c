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

#define DEFAULT_MESSAGE "Hello"
#define DEFAULT_SERVER_NAME "localhost"
#define DEFAULT_SERVER_PORT 4444
#define DEFAULT_SECURITY_PACKAGE NTLMSP_NAME_A
#define DEFAULT_ISC_FLAGS ( \
    ISC_REQ_STREAM | \
    /* ISC_REQ_MUTUAL_AUTH | */ \
    /* ISC_REQ_REPLAY_DETECT | */ \
    ISC_REQ_INTEGRITY | \
    /* ISC_REQ_CONFIDENTIALITY | */ \
    ISC_REQ_ALLOCATE_MEMORY | \
    0 )

#define BAIL_ON_ERROR(dwError) \
    if (dwError)               \
    {                          \
        printf("%s()@%s:%d - error = %u (0x%08x)\n", __FUNCTION__, __FILE__, __LINE__, dwError, dwError); \
        goto error;            \
    }

#define DUMP_FLAG(Flags, Flag) \
    do { \
        if ((Flags) & (Flag)) \
        { \
            printf("    %-35s (0x%08X)\n", # Flag, Flag); \
            (Flags) &= ~(Flag); \
        } \
    } while (0)

typedef struct _SSPI_CLIENT_PARAMS {
    PCSTR pszServerName;
    USHORT usServerPort;
    PCSTR pszServerPrincipalName;
    PCSTR pszUserName;
    PCSTR pszUserDomain;
    PCSTR pszUserPassword;
    PCSTR pszMessage;
    PCSTR pszSecurityPackage;
    ULONG ulIscFlags;
    BOOL bSignOnly;
} SSPI_CLIENT_PARAMS, *PSSPI_CLIENT_PARAMS;

typedef struct _FLAGMAPPING
{
    PCSTR name;
    ULONG value;
    PCSTR realname;
} FLAGMAPPING, *PFLAGMAPPING;

#define INIT_FLAGMAPPING( x ) { "-" #x, ISC_REQ_ ## x, "ISC_REQ_"#x }

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

typedef VOID (*PFN_DUMP)(PVOID, DWORD);

VOID
Usage(
    IN PCSTR pszProgramName,
    IN FLAGMAPPING FlagMappings[],
    IN DWORD dwFlagMappingsLength
    );

VOID
ParseArgs(
    IN INT argc,
    IN PCSTR argv[],
    OUT PSSPI_CLIENT_PARAMS pParams
    );

DWORD
CallServer(
    IN PCSTR pHost,
    IN USHORT usPort,
    IN PCSTR pSPN,
    IN PCSTR pServiceName,
    IN PCSTR pServicePassword,
    IN PCSTR pServiceRealm,
    IN ULONG DelegFlag,
    IN PCSTR pMsg,
    IN PCSTR pSecPkgName,
    IN INT nSignOnly
    );

DWORD
ConnectToServer(
    IN PCSTR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    );

DWORD
ClientEstablishContext(
    IN PCSTR pSPN,
    IN INT nSocket,
    IN PCSTR pServiceName,
    IN PCSTR pServicePassword,
    IN PCSTR pServiceRealm,
    IN ULONG DelegFlag,
    OUT CtxtHandle *pSspiContext,
    IN PCSTR pSecPkgName,
    OUT ULONG *pRetFlags
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
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesWritten
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
    IN DWORD dwBytes,
    OUT PDWORD pdwBytesRead
    );

VOID
DumpIscReqFlags(
    DWORD dwFlags
    );

VOID
DumpIscRetFlags(
    DWORD dwFlags
    );

VOID
DumpNtlmFlags(
    DWORD dwFlags
    );

VOID
DumpNegegotiateMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

VOID
DumpChallengeMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

VOID
DumpResponseMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

VOID
DumpNtlmMessage(
    PVOID pBuffer,
    DWORD dwSize
    );

VOID
PrintHexDump(
    DWORD dwLength,
    PVOID pBuffer
    );

int
_cdecl
main(
    IN INT argc,
    IN PCSTR argv[]
    )
{
    DWORD dwError = ERROR_SUCCESS;
    SSPI_CLIENT_PARAMS params = { 0 };

    params.pszMessage = DEFAULT_MESSAGE;
    params.pszServerName = DEFAULT_SERVER_NAME;
    params.usServerPort = DEFAULT_SERVER_PORT;
    params.pszSecurityPackage = DEFAULT_SECURITY_PACKAGE;
    params.ulIscFlags = DEFAULT_ISC_FLAGS;

    ParseArgs(argc, argv, &params);

    if (params.bSignOnly)
    {
        printf("No encryption - sign only\n");
    }
    printf("Context req flags:\n");
    DumpIscReqFlags(params.ulIscFlags);

    printf("\n");

    dwError = CallServer(
        params.pszServerName,
        params.usServerPort,
        params.pszServerPrincipalName,
        params.pszUserName,
        params.pszUserPassword,
        params.pszUserDomain,
        params.ulIscFlags,
        params.pszMessage,
        params.pszSecurityPackage,
        params.bSignOnly
        );

    BAIL_ON_ERROR(dwError);

error:
    return dwError;
}

VOID
Usage(
    IN PCSTR pszProgramName,
    IN FLAGMAPPING FlagMappings[],
    IN DWORD dwFlagMappingsLength
    )
{
    DWORD dwIndex = 0;
    fprintf(stderr, "usage: %s [OPTION]\n"
            "\n"
            "  where options are:\n"
            "\n"
            "    -h HOSTNAME   -- Connect to specified host (default is %s)\n"
            "    -port NUMBER  -- Connect to specified port number (default is %u)\n"
            "    -spn SPN      -- Use specified SPN\n"
            "    -sign         -- Sign only (i.e., do not encrypt)\n"
            "    -d            -- Use delegation\n"
            "    -k            -- Use Kerberos (default is %s)\n"
            "    -spnego       -- Use SPNEGO (default is %s)\n"
            "    -m MESSAGE    -- Send specified message (default is %s)\n"
            "    -c USER PASSWORD DOMAIN -- Use specifed credentials\n",
            pszProgramName,
            DEFAULT_SERVER_NAME,
            DEFAULT_SERVER_PORT,
            DEFAULT_SECURITY_PACKAGE,
            DEFAULT_SECURITY_PACKAGE,
            DEFAULT_MESSAGE);
    for (dwIndex = 0; dwIndex < dwFlagMappingsLength; dwIndex++)
    {
        printf("    %-23s -- Set %s flag\n",
               FlagMappings[dwIndex].name,
               FlagMappings[dwIndex].realname);
    }
    printf("\n");
    exit(1);
}

VOID
ParseArgs(
    IN INT argc,
    IN PCSTR argv[],
    OUT PSSPI_CLIENT_PARAMS pParams
    )
{
    PCSTR pszProgramName = argv[0];
    BOOL bShowUsage = FALSE;
    FLAGMAPPING FlagMappings[] = {
        INIT_FLAGMAPPING( CONFIDENTIALITY ),
        INIT_FLAGMAPPING( DELEGATE ),
        INIT_FLAGMAPPING( INTEGRITY ),
        INIT_FLAGMAPPING( USE_SESSION_KEY ),
        INIT_FLAGMAPPING( REPLAY_DETECT ),
        INIT_FLAGMAPPING( SEQUENCE_DETECT ),
        INIT_FLAGMAPPING( MUTUAL_AUTH )
    };

    /* Parse arguments. */
    argc--; argv++;
    while (argc)
    {
        if (!strcmp(*argv, "/?") ||
            !_strcmpi(*argv, "--help"))
        {
            bShowUsage = TRUE;
            goto cleanup;
        }
        else if (strcmp(*argv, "-h") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                bShowUsage = TRUE;
                goto cleanup;
            }

            pParams->pszServerName = *argv;
        }
        else if (strcmp(*argv, "-port") == 0)
        {
            argc--; argv++;

            if (!argc)
            {
                bShowUsage = TRUE;
                goto cleanup;
            }

            pParams->usServerPort = (USHORT) atoi(*argv);
        }
        else if (strcmp(*argv, "-spn") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                bShowUsage = TRUE;
                goto cleanup;
            }

            pParams->pszServerPrincipalName = *argv;
        }
        else if (strcmp(*argv, "-sign") == 0)
        {
            pParams->bSignOnly = TRUE;
        }
        else if (strcmp(*argv, "-k") == 0)
        {
            pParams->pszSecurityPackage = MICROSOFT_KERBEROS_NAME_A;
        }
        else if (strcmp(*argv, "-spnego") == 0)
        {
            pParams->pszSecurityPackage = NEGOSSP_NAME;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            pParams->ulIscFlags |= ISC_REQ_DELEGATE;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                bShowUsage = TRUE;
                goto cleanup;
            }

            pParams->pszMessage = *argv;
        }
        else if (strcmp(*argv, "-c") == 0)
        {
            argv++;
            argc--;

            if (argc < 3)
            {
                bShowUsage = TRUE;
                goto cleanup;
            }

            pParams->pszUserName = *argv;
            argv++;
            argc--;
            pParams->pszUserPassword = *argv;
            argv++;
            argc--;
            pParams->pszUserDomain = *argv;
        }
        else
        {
            /*
            * Search for flags to use along the command line.
            * We do this so that we can determine what, if any,
            * flags are busted.
            */

            BOOLEAN bFound = FALSE;
            DWORD dwIndex = 0;

            for (dwIndex = 0; dwIndex < (sizeof(FlagMappings)/sizeof(FLAGMAPPING)); dwIndex++)
            {
                if (_strcmpi( *argv, FlagMappings[dwIndex].name ) == 0)
                {
                    bFound = TRUE;
                    pParams->ulIscFlags |= FlagMappings[dwIndex].value;
                    break;
                }
            }

            if (!bFound)
            {
                fprintf(stderr, "Unknown option %s\n", *argv);
                bShowUsage = TRUE;
                goto cleanup;
            }
        }
        argc--; argv++;
    }

cleanup:
    if (bShowUsage)
    {
        Usage(pszProgramName, FlagMappings, (sizeof(FlagMappings)/sizeof(FLAGMAPPING)));
    }
}

DWORD
CallServer(
    IN PCSTR pHost,
    IN USHORT usPort,
    IN PCSTR pSPN,
    IN PCSTR pServiceName,
    IN PCSTR pServicePassword,
    IN PCSTR pServiceRealm,
    IN ULONG DelegFlag,
    IN PCSTR pMsg,
    IN PCSTR pSecPkgName,
    IN INT nSignOnly
    )
{
    DWORD dwError = ERROR_SUCCESS;
    SecBuffer WrapBuffers[3] = {0};
    INT nSocket = INVALID_SOCKET;
    ULONG nIndex = 0;
    ULONG RetFlags = 0;
    ULONG QopState = 0;
    INT nContextAcquired = 0;

    CtxtHandle Context;
    SecBuffer InBuffer;
    SecBuffer OutBuffer;
    SecBufferDesc InBufferDesc;
    SecPkgContext_Sizes Sizes;
    SecPkgContext_Names Names;
#if 0
    SecPkgContext_AccessToken Token;
    SecPkgContext_NativeNames NativeNames;
    HANDLE TokenHandle;
    SecPkgContext_Authority Authority;
    TOKEN_USER User;
    DWORD NeedLength = 0;
#endif
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

#if 0
    dwError = QueryContextAttributes(
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
    BAIL_ON_ERROR(dwError);

    dwError = QuerySecurityContextToken(
            &Context,
            &TokenHandle);
    BAIL_ON_ERROR(dwError);

    dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_AUTHORITY,
                    &Authority);
    BAIL_ON_ERROR(dwError);

    printf("Authority is %s\n", Authority.sAuthorityName);

    dwError = QueryContextAttributes(
                    &Context,
                    SECPKG_ATTR_NATIVE_NAMES,
                    &NativeNames);
    BAIL_ON_ERROR(dwError);
#endif

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
    InBuffer.pvBuffer = (PVOID) pMsg;
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
    IN PCSTR pHost,
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
    IN PCSTR pSPN,
    IN INT nSocket,
    IN PCSTR pServiceName,
    IN PCSTR pServicePassword,
    IN PCSTR pServiceRealm,
    IN ULONG DelegFlag,
    OUT CtxtHandle *pSspiContext,
    IN PCSTR pSecPkgName,
    OUT ULONG *pRetFlags
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
        AuthIdentity.User = (PBYTE) pServiceName;
        AuthIdentity.UserLength = (DWORD)strlen(pServiceName);
        pAuthId = &AuthIdentity;
    }

    if (pServicePassword)
    {
        AuthIdentity.Password = (PBYTE) pServicePassword;
        AuthIdentity.PasswordLength = (DWORD)strlen(pServicePassword);
        pAuthId = &AuthIdentity;
    }

    if (pServiceRealm)
    {
        AuthIdentity.Domain = (PBYTE) pServiceRealm;
        AuthIdentity.DomainLength = (DWORD)strlen(pServiceRealm);
        pAuthId = &AuthIdentity;
    }
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    dwError = AcquireCredentialsHandle(
        "join@CORPQA.CENTERIS.COM",                       // no principal name
        (PSTR) pSecPkgName,                // package name
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
                (PSTR) pSPN,
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
            printf("Flags returned:\n");
            DumpIscRetFlags(*pRetFlags);
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
            break;
        }
    }

    *pdwBytesRead = pTrav - pBuffer;

error:
    return dwError;
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
    DUMP_FLAG(dwRemainder, ISC_REQ_FORWARD_CREDENTIALS);
    DUMP_FLAG(dwRemainder, ISC_REQ_NO_INTEGRITY);
    DUMP_FLAG(dwRemainder, ISC_REQ_USE_HTTP_STYLE);

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
    DUMP_FLAG(dwRemainder, ISC_RET_FORWARD_CREDENTIALS);
    DUMP_FLAG(dwRemainder, ISC_RET_USED_HTTP_STYLE);
    DUMP_FLAG(dwRemainder, ISC_RET_NO_ADDITIONAL_TOKEN);
    DUMP_FLAG(dwRemainder, ISC_RET_REAUTHENTICATION);

    if (dwRemainder)
    {
        printf("    Unknown flags: 0x%08X\n", dwRemainder);
    }
}

VOID
DumpNtlmFlags(
    DWORD dwFlags
    )
{
    DWORD dwRemainder = dwFlags;

#define NTLM_FLAG_UNICODE               0x00000001  /* unicode charset */
#define NTLM_FLAG_OEM                   0x00000002  /* oem charset */
#define NTLM_FLAG_REQUEST_TARGET        0x00000004  /* ret trgt in challenge */
#define NTLM_FLAG_UNDEFINED_00000008    0x00000008

#define NTLM_FLAG_SIGN                  0x00000010  /* sign requested */
#define NTLM_FLAG_SEAL                  0x00000020  /* encryption requested */
#define NTLM_FLAG_DATAGRAM              0x00000040  /* udp message */
#define NTLM_FLAG_LM_KEY                0x00000080  /* use LM key for crypto */

#define NTLM_FLAG_NETWARE               0x00000100  /* netware - unsupported */
#define NTLM_FLAG_NTLM                  0x00000200  /* use NTLM auth */
#define NTLM_FLAG_UNDEFINED_00000400    0x00000400
#define NTLM_FLAG_UNDEFINED_00000800    0x00000800

#define NTLM_FLAG_DOMAIN                0x00001000  /* domain supplied */
#define NTLM_FLAG_WORKSTATION           0x00002000  /* wks supplied */
#define NTLM_FLAG_LOCAL_CALL            0x00004000  /* loopback auth */
#define NTLM_FLAG_ALWAYS_SIGN           0x00008000  /* use dummy sig */

#define NTLM_FLAG_TYPE_DOMAIN           0x00010000  /* domain authenticator */
#define NTLM_FLAG_TYPE_SERVER           0x00020000  /* server authenticator */
#define NTLM_FLAG_TYPE_SHARE            0x00040000  /* share authenticator */
#define NTLM_FLAG_NTLM2                 0x00080000  /* use NTLMv2 key */

#define NTLM_FLAG_INIT_RESPONSE         0x00100000  /* unknown */
#define NTLM_FLAG_ACCEPT_RESPONSE       0x00200000  /* unknown */
#define NTLM_FLAG_NON_NT_SESSION_KEY    0x00400000  /* unknown */
#define NTLM_FLAG_TARGET_INFO           0x00800000  /* target info used */

#define NTLM_FLAG_UNDEFINED_01000000    0x01000000
#define NTLM_FLAG_UNKNOWN_02000000      0x02000000  /* needed, for what? */
#define NTLM_FLAG_UNDEFINED_04000000    0x04000000
#define NTLM_FLAG_UNDEFINED_08000000    0x08000000

#define NTLM_FLAG_UNDEFINED_10000000    0x10000000
#define NTLM_FLAG_128                   0x20000000  /* 128-bit encryption */
#define NTLM_FLAG_KEY_EXCH              0x40000000  /* perform key exchange */
#define NTLM_FLAG_56                    0x80000000  /* 56-bit encryption */

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
    DUMP_FLAG(dwRemainder, NTLM_FLAG_UNDEFINED_00000800);

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

VOID
DumpResponseMessage(
    PVOID pBuffer,
    DWORD dwSize
    )
{
    PNTLM_RESPONSE_MESSAGE pMsg = (PNTLM_RESPONSE_MESSAGE) pBuffer;
    UNREFERENCED_PARAMETER(dwSize);
    UNREFERENCED_PARAMETER(pMsg);
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
PrintHexDump(
    DWORD dwLength,
    PVOID pBuffer
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

    DumpNtlmMessage(pBuffer, dwLength);
}
