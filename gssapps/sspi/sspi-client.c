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

#define INIT_FLAGMAPPING(Flag) _INIT_FLAGMAPPING(ISC_REQ_, Flag)

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

static
VOID
Usage(
    IN PCSTR pszProgramName,
    IN FLAGMAPPING FlagMappings[],
    IN DWORD dwFlagMappingsLength
    );

static
VOID
ParseArgs(
    IN INT argc,
    IN PCSTR argv[],
    OUT PSSPI_CLIENT_PARAMS pParams
    );

static
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

static
DWORD
ConnectToServer(
    IN PCSTR pHost,
    IN USHORT usPort,
    OUT PINT pSocket
    );

static
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

static
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

static
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
        INIT_FLAGMAPPING( MUTUAL_AUTH ),
        INIT_FLAGMAPPING( NULL_SESSION )
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

static
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

    goto finish;
}

static
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
    memcpy(&sAddr.sin_addr, pHostEnt->h_addr, sizeof(sAddr.sin_addr));
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

static
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
        NULL,                       // no principal name
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
	    PNTLM_NEGOTIATE_MESSAGE pMsg = (PNTLM_NEGOTIATE_MESSAGE) SendTokenBuffer.pvBuffer;
	    // Adjust any flags for debugging
	    // pMsg->NtlmFlags |= NTLM_FLAG_SEAL;
            printf("Context partially initialized...\n");

            DumpBuffer(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
            DumpNtlmMessage(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
            printf("\n");
            printf("Flags returned:\n");
            DumpIscRetFlags(*pRetFlags);
            printf("\n");
        }
        else
        {
	    PNTLM_RESPONSE_MESSAGE pMsg = (PNTLM_RESPONSE_MESSAGE) SendTokenBuffer.pvBuffer;
	    // Adjust any flags for debugging
	    //pMsg->NtlmFlags |= NTLM_FLAG_SEAL;

            printf("Context FULLY initialized!\n");
            DumpBuffer(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
            DumpNtlmMessage(SendTokenBuffer.pvBuffer, SendTokenBuffer.cbBuffer);
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
            DumpBuffer(RecvTokenBuffer.pvBuffer, RecvTokenBuffer.cbBuffer);
            DumpNtlmMessage(RecvTokenBuffer.pvBuffer, RecvTokenBuffer.cbBuffer);
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
