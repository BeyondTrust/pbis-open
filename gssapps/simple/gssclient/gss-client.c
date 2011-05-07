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
/*
 * Copyright (C) 2003, 2004, 2005 by the Massachusetts Institute of Technology.
 * All rights reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#include "gss-client.h"

void display_status_1(char *m, OM_uint32 code, int type);

void display_status(char *msg, OM_uint32 maj_stat, OM_uint32 min_stat)
{
    if (maj_stat != GSS_S_CONTINUE_NEEDED)
    {
            display_status_1(msg, maj_stat, GSS_C_GSS_CODE);
            display_status_1(msg, min_stat, GSS_C_MECH_CODE);
    }
}

void display_status_1(char *m, OM_uint32 code, int type)
{
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1) {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        switch(code)
        {
#ifdef WIN32
        case SEC_E_OK:
        case SEC_I_CONTINUE_NEEDED:
#else
        case GSS_S_COMPLETE:
        case GSS_S_CONTINUE_NEEDED:
#endif
            break;
        default:
            fprintf(stderr, "GSS-API error calling %s: %d (%s)\n", m, code, (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

INT
main(
    IN INT argc,
    IN PCHAR* argv
    )
{
    PCHAR pServiceName = NULL;
    PCHAR pServicePassword = NULL;
    PCHAR pServiceRealm = NULL;
    PCHAR pServerHost = "localhost";
    PCHAR pSPN = "unset";
    PCHAR pMsg = "Hello";
    DWORD nError = 0;
    USHORT usPort = 4444;
    OM_uint32 nGssFlags = 
            GSS_C_CONF_FLAG |
            GSS_C_INTEG_FLAG |
            GSS_C_REPLAY_FLAG |
            GSS_C_MUTUAL_FLAG;
    INT nFormat = 0;
    INT nUseSpnego = 0;
    gss_OID GssOid = NULL;
    gss_buffer_desc OidString;
    OM_uint32 MinorStat = GSS_S_COMPLETE;

    /* Parse arguments. */
    argc--;
    argv++;

    while (argc)
    {
        if (strcmp(*argv, "-port") == 0)
        {
            argc--;
            argv++;
            if (!argc)
            {
                nError = Usage();
                BAIL_ON_ERROR(nError);
            }
            usPort = (USHORT)atoi(*argv);
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            nGssFlags |= GSS_C_DELEG_FLAG;
        }
        else if (strcmp(*argv, "-seq") == 0)
        {
            nGssFlags |= GSS_C_SEQUENCE_FLAG;
        }
        else if (strcmp(*argv, "-noreplay") == 0)
        {
            nGssFlags &= ~GSS_C_REPLAY_FLAG;
        }
        else if (strcmp(*argv, "-nomutual") == 0)
        {
            nGssFlags &= ~GSS_C_MUTUAL_FLAG;
        }
        else if (strcmp(*argv, "-anon") == 0)
        {
            nGssFlags |= GSS_C_ANON_FLAG;
        }
        else if (strcmp(*argv, "-spnego") == 0)
        {
            nUseSpnego = 1;
        }
        else if (strcmp(*argv, "-c") == 0)
        {
            argc--;
            argv++;

            if (argc < 3)
            {
                nError = Usage();
                BAIL_ON_ERROR(nError);
            } 
            pServiceName = *argv;
            argv++;
            argc--;
            pServicePassword = *argv;
            argv++;
            argc--;
            pServiceRealm = *argv;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            argc--;
            argv++;

            if (argc < 1)
            {
                nError = Usage();
                BAIL_ON_ERROR(nError);
            }

            pMsg = *argv;
        }
        else if (strcmp(*argv, "-h") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                nError = Usage();
                BAIL_ON_ERROR(nError);
            }

            pServerHost = *argv;
        }
        else if (strcmp(*argv, "-spn") == 0)
        {
            argv++;
            argc--;

            if (!argc)
            {
                nError = Usage();
                BAIL_ON_ERROR(nError);
            }

            pSPN = *argv;
        }

        else
        {
            nError = Usage();
            BAIL_ON_ERROR(nError);
        }

        argc--;
        argv++;
    }

    if (nUseSpnego)
    {
        OidString.value = "1.3.6.1.5.5.2";
    }
    else
    {
        OidString.value = "1.3.6.1.4.1.311.2.2.10";
    }
    OidString.length = strlen(OidString.value);

    gss_str_to_oid(&MinorStat, &OidString, &GssOid);

    nError =
        CallServer(
            pServerHost,
            usPort,
            pSPN,
            GssOid,
            pServiceName,
            pServicePassword,
            pServiceRealm,
            nGssFlags,
            nFormat,
            pMsg
            );

    BAIL_ON_ERROR(nError);

finish:
    gss_release_oid(&MinorStat, &GssOid);
    return nError;
error:
    goto finish;
}

DWORD
Usage(VOID)
{
    fprintf(stderr, "Usage: gsssimple-client [-port port] [-d] [-seq] [-noreplay]\n");
    fprintf(stderr, "  [-nomutual] [-spnego] [-anon] [-h host] \n");
    fprintf(stderr, "  [-c user password domain] [-m message]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -d         delegate credentials\n");
    fprintf(stderr, "  -seq       add sequence number\n");
    fprintf(stderr, "  -noreplay  disable replay checking\n");
    fprintf(stderr, "  -nomutual  disable mutual auth\n");
    fprintf(stderr, "  -anon      perform anonymous authentication\n");
    fprintf(stderr, "  -spnego    use spnego instead of ntlm directly\n");
    fprintf(stderr, "  -c         use supplemental credentials\n");

    return EINVAL;
}

DWORD
ShowContextDetails(
    gss_ctx_id_t pContext
    )
{
    DWORD dwError = 0;
    gss_name_t srcName = NULL;
    gss_name_t targetName = NULL;
    OM_uint32 lifetime = 0;
    gss_OID mech = 0;
    //OM_uint32 flags = 0;
    //int locallyCreated = 0;
    //int fullyOpen = 0;
    OM_uint32 minorStatus = 0;

    gss_buffer_desc buffer = {0};
    gss_OID nameOID;

    dwError = gss_inquire_context(
                &minorStatus,
                pContext,
                &srcName,
                NULL, //&targetName,
                &lifetime,
                &mech,
                NULL, //&flags,
                NULL, //&locallyCreated,
                NULL); //&fullyOpen);
    display_status("gss_inquire_context", dwError, minorStatus);
    BAIL_ON_ERROR(dwError);

    printf("Context created from ");

    dwError = gss_display_name(
                &minorStatus,
                srcName,
                &buffer,
                &nameOID);
    BAIL_ON_ERROR(dwError);

    printf("%.*s to ", (int)buffer.length, (char *)buffer.value);

    /*dwError = gss_display_name(
                &minorStatus,
                targetName,
                buffer,
                &nameOID);
    BAIL_ON_ERROR(dwError);*/

    printf("%s\n", "unknown"); //(char *)buffer->value);
    printf("lifetime = %d\n", lifetime);

    gss_release_buffer(&minorStatus, &buffer);
    dwError = gss_oid_to_str(
                &minorStatus,
                mech,
                &buffer);
    printf("mech = %.*s\n", (int)buffer.length, (char *)buffer.value);

    //printf("flags = 0x%X\n", flags);
    //printf("locally created = %d\n", locallyCreated);
    //printf("fully opened = %d\n", fullyOpen);

cleanup:
    if (srcName)
    {
        gss_release_name(&minorStatus, &srcName);
    }
    if (targetName)
    {
        gss_release_name(&minorStatus, &targetName);
    }
    gss_release_buffer(&minorStatus, &buffer);
    return dwError;
    
error:
    goto cleanup;
}

DWORD
CallServer(
    IN PCHAR pHost,
    IN USHORT usPort,
    IN PCHAR pSPN,
    IN gss_OID pObjectId,
    IN PCHAR pServiceName,
    IN PCHAR pServicePassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 nGssFlags,
    IN INT nFormat,
    IN PCHAR pMsg
    )
{
    DWORD nError = 0;
    gss_ctx_id_t pContext = GSS_C_NO_CONTEXT;
    gss_buffer_desc GssBufferIn;
    gss_buffer_desc GssBufferOut;
    INT nSocket = INVALID_SOCKET;
    INT nState = 0;
    OM_uint32 MinorStatus = 0;
    OM_uint32 nRetFlags = 0;
    gss_qop_t pQopState = 0;
    INT nBufferAcquired = 0;

    memset(&GssBufferIn, 0, sizeof(gss_buffer_desc));
    memset(&GssBufferOut, 0, sizeof(gss_buffer_desc));

    /* Open connection */
    nError = ConnectToServer(pHost, usPort, &nSocket);
    if (nError)
    {
        fprintf(stderr, "Error %d while connecting to %s:%d\n", nError, pHost, usPort);
    }
    BAIL_ON_ERROR(nError);

    /* Establish context */
    nError =
        ClientEstablishContext(
            pSPN,
            nSocket,
            pServiceName,
            pServicePassword,
            pServiceRealm,
            nGssFlags,
            pObjectId,
            &pContext,
            &nRetFlags
            );

    BAIL_ON_ERROR(nError);

    // perform queries here
    nError = ShowContextDetails(pContext);
    BAIL_ON_ERROR(nError);

    /* Seal the message */
    GssBufferIn.value = pMsg;
    GssBufferIn.length = strlen(pMsg);

    nState = (nGssFlags & GSS_C_CONF_FLAG) != 0;
    nError = gss_wrap(
        &MinorStatus,
        pContext,
        nState,
        GSS_C_QOP_DEFAULT,
        &GssBufferIn,
        &nState,
        &GssBufferOut
        );

    if ((GSS_S_FAILURE == nError) && MinorStatus)
    {
        nError = MinorStatus;
    }

    BAIL_ON_ERROR(nError);

    if (!nState && (nGssFlags & GSS_C_CONF_FLAG))
    {
        nError = GSS_S_FAILURE;
        BAIL_ON_ERROR(nError);
    }

    nBufferAcquired = 1;

    /* Send to server */
    nError =
        SendToken(
            nSocket,
            &GssBufferOut
            );

    BAIL_ON_ERROR(nError);

    if (GssBufferOut.value != GssBufferIn.value)
    {
        nError = gss_release_buffer(&MinorStatus, &GssBufferOut);

        if ((GSS_S_FAILURE == nError) && MinorStatus)
        {
            nError = MinorStatus;
        }

        BAIL_ON_ERROR(nError);

        nBufferAcquired = 0;
    }

    /* Read signature block into GssBufferOut */
    nError =
        RecvToken(
            nSocket,
            &GssBufferOut
            );

    BAIL_ON_ERROR(nError);

    /* Verify signature block */
    nError =
        gss_verify_mic(
            &MinorStatus,
            pContext,
            &GssBufferIn,
            &GssBufferOut,
            &pQopState
            );

    if ((GSS_S_FAILURE == nError) && MinorStatus)
    {
        nError = MinorStatus;
    }

    BAIL_ON_ERROR(nError);

    free(GssBufferOut.value);
    GssBufferOut.value = NULL;

    /* Delete context */
    nError =
        gss_delete_sec_context(
            &MinorStatus,
            &pContext,
            GSS_C_NO_BUFFER
            );

    if ((GSS_S_FAILURE == nError) && MinorStatus)
    {
        nError = MinorStatus;
    }

    BAIL_ON_ERROR(nError);

    close(nSocket);

finish:
    return nError;
error:
    if (INVALID_SOCKET != nSocket)
    {
        close(nSocket);
    }
    if (GSS_C_NO_CONTEXT != pContext)
    {
        gss_delete_sec_context(
            &MinorStatus,
            &pContext,
            GSS_C_NO_BUFFER
            );
    }
    if (nBufferAcquired)
    {
        gss_release_buffer(
            &MinorStatus,
            &GssBufferOut
            );
        GssBufferOut.value = NULL;
    }
    if (GssBufferOut.value)
    {
        free(GssBufferOut.value);
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
    DWORD nError = 0;
    struct sockaddr_in sAddr;
    struct hostent *pHostEnt = NULL;

    memset(&sAddr, 0, sizeof(struct sockaddr_in));

    *pSocket = INVALID_SOCKET;

    pHostEnt = gethostbyname(pHost);

    if (NULL == pHostEnt)
    {
        nError = h_errno;
        BAIL_ON_ERROR(nError);
    }

    sAddr.sin_family = (sa_family_t)pHostEnt->h_addrtype;

    memcpy((PCHAR)&sAddr.sin_addr, pHostEnt->h_addr, sizeof(sAddr.sin_addr));

    sAddr.sin_port = htons(usPort);

    *pSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == *pSocket)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

    nError =
        connect(
            *pSocket,
            (struct sockaddr *) &sAddr,
            sizeof(sAddr)
            );

    if (0 != nError)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

finish:
    return nError;
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
    IN PCHAR pSPN,
    IN INT nSocket,
    IN PCHAR pszUsername,
    IN PCHAR pszUserPassword,
    IN PCHAR pServiceRealm,
    IN OM_uint32 nGssFlags,
    IN gss_OID pObjectId,
    OUT gss_ctx_id_t *ppGssContext,
    OUT OM_uint32 *pRetFlags
    )
{
    DWORD nError = 0;

    gss_buffer_desc SendTokenBuffer;
    gss_buffer_desc RecvTokenBuffer;
    gss_buffer_desc TargetName;
    gss_buffer_desc Username;
    gss_buffer_desc AuthDataDesc;
    SEC_WINNT_AUTH_IDENTITY AuthData;
    gss_cred_id_t CredHandle = GSS_C_NO_CREDENTIAL;
    gss_buffer_t pTokenBuffer = NULL;
    gss_name_t pTargetName = NULL;
    gss_name_t pSourceName = NULL;
    OM_uint32 MajorStatus = 0;
    OM_uint32 MinorStatus = 0;
    OM_uint32 InitSecMinorStatus = 0;
    gss_OID_set_desc DesiredMechs;
    gss_OID_set ActualMechs;
    gss_buffer_desc OidString;
    gss_OID NtlmOid = NULL;
    OM_uint32 TimeRec = 0;
    gss_OID GssCredOptionPasswordOid = NULL;

    memset(&SendTokenBuffer, 0, sizeof(gss_buffer_desc));
    memset(&RecvTokenBuffer, 0, sizeof(gss_buffer_desc));

    *ppGssContext = GSS_C_NO_CONTEXT;
    *pRetFlags = 0;

    TargetName.value = pSPN;
    TargetName.length = strlen(TargetName.value);

    nError = gss_import_name(
        &MinorStatus,
        &TargetName,
        (gss_OID)GSS_KRB5_NT_PRINCIPAL_NAME,
        &pTargetName);

    display_status("gss_import_name", nError, MinorStatus);
    if ((GSS_S_FAILURE == nError) && MinorStatus)
    {
         nError = MinorStatus;
    }

    BAIL_ON_ERROR(nError);

    if (pszUsername)
    {
        if (pszUsername[0])
        {
            Username.value = pszUsername;
            Username.length = strlen(Username.value);

            nError = gss_import_name(
                &MinorStatus,
                &Username,
                GSS_C_NT_USER_NAME, //(gss_OID) gss_nt_service_name,
                &pSourceName
                );
            display_status("gss_import_name", nError, MinorStatus);
            if ((GSS_S_FAILURE == nError) && MinorStatus)
            {
                 nError = MinorStatus;
            }
            BAIL_ON_ERROR(nError);
        }

        OidString.value = GSS_NTLM_OID_STRING;
        OidString.length = strlen(GSS_NTLM_OID_STRING);

        MajorStatus = gss_str_to_oid(&MinorStatus, &OidString, &NtlmOid);
        BAIL_ON_ERROR(MajorStatus);

        DesiredMechs.count = 1;
        DesiredMechs.elements = NtlmOid;

        nError = gss_acquire_cred(
            &MinorStatus,
            pSourceName,
            0,
            &DesiredMechs,
            GSS_C_INITIATE,
            &CredHandle,
            &ActualMechs,
            &TimeRec
            );
        display_status("gss_acquire_cred", nError, MinorStatus);

        if ((GSS_S_FAILURE == nError) && MinorStatus)
        {
             nError = MinorStatus;
        }
        BAIL_ON_ERROR(nError);
    }

    if (pszUserPassword)
    {
        AuthData.User = pszUsername;
        AuthData.UserLength = strlen(pszUsername);
        AuthData.Domain = pServiceRealm;
        AuthData.DomainLength = strlen(pServiceRealm);
        AuthData.Password = pszUserPassword;
        AuthData.PasswordLength = strlen(pszUserPassword);
        AuthData.Flags = 0;

        AuthDataDesc.length = sizeof(AuthData);
        AuthDataDesc.value = &AuthData;

        OidString.value = GSS_CRED_OPT_PW_OID_STRING;
        OidString.length = strlen(GSS_CRED_OPT_PW_OID_STRING);

        MajorStatus = gss_str_to_oid(
                          &MinorStatus,
                          &OidString,
                          &GssCredOptionPasswordOid);
        BAIL_ON_ERROR(MajorStatus);

        nError = gssspi_set_cred_option(
            &MinorStatus,
            CredHandle,
            GssCredOptionPasswordOid,
            &AuthDataDesc
            );
        display_status("gssspi_set_cred_option", nError, MinorStatus);

        if ((GSS_S_FAILURE == nError) && MinorStatus)
        {
             nError = MinorStatus;
        }
        BAIL_ON_ERROR(nError);
    }

    pTokenBuffer = GSS_C_NO_BUFFER;

    do
    {
        MajorStatus =
            gss_init_sec_context(
                &InitSecMinorStatus,
                CredHandle,
                ppGssContext,
                pTargetName,
                pObjectId,
                nGssFlags,
                0,
                NULL,               /* no channel bindings */
                pTokenBuffer,
                NULL,               /* ignore mech type */
                &SendTokenBuffer,
                pRetFlags,
                NULL
                );                  /* ignore time_rec */
        display_status("gss_init_sec_context", MajorStatus, InitSecMinorStatus);

        if (RecvTokenBuffer.value != NULL)
        {
            nError = gss_release_buffer(&MinorStatus, &RecvTokenBuffer);
        }

        if (SendTokenBuffer.length != 0)
        {
            nError =
                SendToken(
                    nSocket,
                    &SendTokenBuffer
                    );

            BAIL_ON_ERROR(nError);
        }

        nError = gss_release_buffer(&MinorStatus, &SendTokenBuffer);

        if ((GSS_S_FAILURE == nError) && MinorStatus)
        {
            nError = MinorStatus;
        }

        BAIL_ON_ERROR(nError);

        if (MajorStatus != GSS_S_COMPLETE &&
           MajorStatus != GSS_S_CONTINUE_NEEDED)
        {
            nError = MajorStatus;
            if ((nError == GSS_S_FAILURE) && InitSecMinorStatus)
            {
                nError = InitSecMinorStatus;
            }
            BAIL_ON_ERROR(nError);
        }

        if (MajorStatus == GSS_S_CONTINUE_NEEDED)
        {
            nError =
                RecvToken(
                    nSocket,
                    &RecvTokenBuffer
                    );

            BAIL_ON_ERROR(nError);

            pTokenBuffer = &RecvTokenBuffer;
        }
    } while (MajorStatus == GSS_S_CONTINUE_NEEDED);

    nError = gss_release_name(&MinorStatus, &pTargetName);

    if ((GSS_S_FAILURE == nError) && MinorStatus)
    {
        nError = MinorStatus;
    }

    BAIL_ON_ERROR(nError);

    fprintf(stderr, "Successfully created context\n");

finish:

    return nError;
error:
    if (pTargetName != NULL)
    {
        gss_release_name(&MinorStatus, &pTargetName);
    }
    if (pSourceName != NULL)
    {
        gss_release_name(&MinorStatus, &pSourceName);
    }
    if (SendTokenBuffer.value != NULL)
    {
        gss_release_buffer(&MinorStatus, &SendTokenBuffer);
        SendTokenBuffer.value = NULL;
        SendTokenBuffer.length = 0;
    }
    if (*ppGssContext != GSS_C_NO_CONTEXT)
    {
        gss_delete_sec_context(
            &MinorStatus,
            ppGssContext,
            GSS_C_NO_BUFFER
            );

        *ppGssContext = GSS_C_NO_CONTEXT;
    }
    if (RecvTokenBuffer.value != NULL)
    {
        gss_release_buffer(&MinorStatus, &RecvTokenBuffer);
    }

    goto finish;
}

DWORD
SendToken(
    IN INT nSocket,
    IN gss_buffer_t pToken
    )
{
    DWORD nError = 0;
    UCHAR BufferAddr[4] = {0};
    INT nBytesWritten = 0;

    if (pToken->length > 0xffffffffUL)
    {
        abort();
    }

    BufferAddr[0] = (UCHAR)(pToken->length >> 24) & 0xff;
    BufferAddr[1] = (UCHAR)(pToken->length >> 16) & 0xff;
    BufferAddr[2] = (UCHAR)(pToken->length >> 8) & 0xff;
    BufferAddr[3] = (UCHAR)pToken->length & 0xff;

    nError = WriteAll(nSocket, (PCHAR)BufferAddr, 4, &nBytesWritten);

    BAIL_ON_ERROR(nError);

    if (4 != nBytesWritten)
    {
        nError = EINVAL;
        BAIL_ON_ERROR(nError);
    }

    nError = WriteAll(nSocket, pToken->value, pToken->length, &nBytesWritten);

    BAIL_ON_ERROR(nError);

    if (pToken->length != nBytesWritten)
    {
        nError = EINVAL;
        BAIL_ON_ERROR(nError);
    }

error:
    return nError;
}

DWORD
WriteAll(
    IN INT nSocket,
    IN PCHAR pBuffer,
    IN UINT nByte,
    OUT PINT pBytesWritten
    )
{
    DWORD nError = 0;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *pBytesWritten = 0;

    for (pTrav = pBuffer; nByte; pTrav += nReturn, nByte -= (UINT)nReturn)
    {
        nReturn = send(nSocket, pTrav, nByte, 0);

        if (nReturn < 0)
        {
            nError = errno;
            BAIL_ON_ERROR(nError);
        }
        else if (nReturn == 0)
        {
            break;
        }
    }

    *pBytesWritten = pTrav - pBuffer;

error:
    return nError;
}

DWORD
RecvToken(
    IN INT nSocket,
    OUT gss_buffer_t pToken
    )
{
    DWORD nError = 0;
    UCHAR BufferAddr[4] = {0};
    INT nBytesRead = 0;

    memset(pToken, 0, sizeof(gss_buffer_desc));

    nError = ReadAll(nSocket, (PCHAR)BufferAddr, 4, &nBytesRead);
    BAIL_ON_ERROR(nError);

    if (4 != nBytesRead)
    {
        nError = EINVAL;
        BAIL_ON_ERROR(nError);
    }

    pToken->length =
        (size_t)((BufferAddr[0] << 24)
        | (BufferAddr[1] << 16)
        | (BufferAddr[2] << 8)
        | BufferAddr[3]);

    pToken->value = (PCHAR) malloc(pToken->length ? pToken->length : 1);

    if (pToken->length && pToken->value == NULL)
    {
        nError = ENOMEM;
        BAIL_ON_ERROR(nError);
    }

    nError = ReadAll(
        nSocket,
        (PCHAR)pToken->value,
        pToken->length,
        &nBytesRead
        );
    BAIL_ON_ERROR(nError);

    if (pToken->length != nBytesRead)
    {
        nError = EINVAL;
        BAIL_ON_ERROR(nError);
    }

finish:
    return nError;
error:
    if (pToken->value)
    {
        free(pToken->value);
        pToken->length = 0;
        pToken->value = 0;
    }
    goto finish;
}

DWORD
ReadAll(
    IN INT nSocket,
    OUT PCHAR pBuffer,
    IN UINT nByte,
    OUT PINT pBytesRead
    )
{
    INT nError = 0;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *pBytesRead = 0;
    memset(pBuffer, 0, nByte);

    for (pTrav = pBuffer; nByte; pTrav += nReturn, nByte -= (UINT)nReturn)
    {
        nReturn = recv(nSocket, pTrav, nByte, 0);

        if (nReturn < 0)
        {
            nError = errno;
            BAIL_ON_ERROR(nError);
        }
        else if (nReturn == 0)
        {
            break;
        }
    }

    *pBytesRead = pTrav - pBuffer;

error:
    return nError;
}
