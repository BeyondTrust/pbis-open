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
 * Copyright (C) 2004,2005 by the Massachusetts Institute of Technology.
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

#include "gss-server.h"

INT
main(
    IN INT argc,
    IN PCHAR* argv
    )
{
    DWORD nError = 0;
    gss_cred_id_t pServerCreds;
    OM_uint32 MinorStat;
    USHORT usPort = 4444;
    INT once = 0;
    INT nAcceptSocket = INVALID_SOCKET;
    INT nListenSocket = INVALID_SOCKET;

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
        else if (strcmp(*argv, "-once") == 0)
        {
            once = 1;
        }
        else
        {
            nError = Usage();
            BAIL_ON_ERROR(nError);
        }

        argc--;
        argv++;
    }

    nError = CreateSocket(usPort, &nListenSocket);
    BAIL_ON_ERROR(nError);

    nError = listen(nListenSocket, 0);

    if (0 != nError)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

    do
    {
        nError = ServerAcquireCreds(&pServerCreds);
        BAIL_ON_ERROR(nError);

        /* Accept a TCP connection */
        printf("Listening...\n");
        nAcceptSocket = accept(nListenSocket, NULL, 0);
        if (INVALID_SOCKET == nAcceptSocket)
        {
            nError = errno;
            BAIL_ON_ERROR(nError);
        }

        nError = SignServer(nAcceptSocket, pServerCreds);
        //BAIL_ON_ERROR(nError);

        if (nError)
        {
            printf("Finished with error code: %d\n", nError);
        }

        nError = gss_release_cred(&MinorStat, &pServerCreds);

        close(nAcceptSocket);

    } while (!once);

    close(nListenSocket);

    if ((GSS_S_FAILURE == nError) && MinorStat)
    {
        nError = MinorStat;
    }

    BAIL_ON_ERROR(nError);

finish:
    return nError;
error:
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
Usage(void)
{
    fprintf(stderr, "Usage: gsssimple-server [-port port] [-once]\n");
    return EINVAL;
}

DWORD
ServerAcquireCreds(
    OUT gss_cred_id_t *ppServerCreds
    )
{
    DWORD nError = 0;
    gss_buffer_desc OidString;
    OM_uint32 MajorStat = 0;
    OM_uint32 MinorStat = 0;
    gss_OID_set DesiredMechs;
    gss_OID_set ActualMechs;
    OM_uint32 TimeRec = 0;
    gss_OID NtlmOid = NULL;

    memset(&DesiredMechs, 0, sizeof(DesiredMechs));
    memset(&ActualMechs, 0, sizeof(ActualMechs));

    *ppServerCreds = NULL;

    MajorStat = gss_create_empty_oid_set(&MinorStat, &DesiredMechs);
    BAIL_ON_ERROR(MajorStat);

    OidString.value = GSS_NTLM_OID_STRING;
    OidString.length = strlen(GSS_NTLM_OID_STRING);

    MajorStat = gss_str_to_oid(&MinorStat, &OidString, &NtlmOid);
    BAIL_ON_ERROR(MajorStat);

    MajorStat = gss_add_oid_set_member(&MinorStat, NtlmOid, &DesiredMechs);
    BAIL_ON_ERROR(MajorStat);

    MajorStat = gss_add_oid_set_member(
            &MinorStat,
            (gss_OID)gss_mech_krb5,
            &DesiredMechs);
    BAIL_ON_ERROR(MajorStat);

    nError =
        gss_acquire_cred(
            &MinorStat,
            NULL,
            0,
            DesiredMechs,
            GSS_C_ACCEPT,
            ppServerCreds,
            &ActualMechs,
            &TimeRec
            );

    if ((GSS_S_FAILURE == nError) && MinorStat)
    {
        nError = MinorStat;
    }

    BAIL_ON_ERROR(nError);

finish:
    return nError;
error:
    goto finish;
}

DWORD
CreateSocket(
    IN USHORT usPort,
    OUT PINT pSocket
    )
{
    DWORD nError = 0;
    struct sockaddr_in SocketAddr;
    INT nOn = 1;

    *pSocket = INVALID_SOCKET;

    SocketAddr.sin_family = AF_INET;
    SocketAddr.sin_port = htons(usPort);
    SocketAddr.sin_addr.s_addr = INADDR_ANY;

    *pSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == *pSocket)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

    /* Let the socket be reused right away */
    nError =
        setsockopt(
            *pSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (PCHAR) &nOn,
            sizeof(nOn)
            );

    if (0 != nError)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

    nError =
        bind(
            *pSocket,
            (struct sockaddr *) &SocketAddr,
            sizeof(SocketAddr)
            );

    if (0 != nError)
    {
        nError = errno;
        BAIL_ON_ERROR(nError);
    }

    nError = listen(*pSocket, 5);
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
SignServer(
    IN INT nSocket,
    IN gss_cred_id_t pServerCreds
    )
{
    DWORD nError = 0;
    gss_buffer_desc ClientName;
    gss_buffer_desc TransmitBuffer;
    gss_buffer_desc MsgBuffer;
    gss_ctx_id_t pContext = NULL;
    OM_uint32 MajorStat = GSS_S_COMPLETE;
    OM_uint32 MinorStat = GSS_S_COMPLETE;
    gss_buffer_set_t SessionKey = NULL;
    gss_name_t UserName = NULL;
    DWORD nIndex = 0;
    gss_OID nameOid = {0};
    gss_buffer_desc ClientNameBuffer = {0};
    INT nConfState = 0;

    memset(&ClientName, 0, sizeof(gss_buffer_desc));
    memset(&TransmitBuffer, 0, sizeof(gss_buffer_desc));
    memset(&MsgBuffer, 0, sizeof(gss_buffer_desc));

    /* Establish a context with the client */
    nError = ServerEstablishContext(
        nSocket,
        pServerCreds,
        &pContext
        );
    BAIL_ON_ERROR(nError);

    MajorStat = gss_inquire_sec_context_by_oid(
        &MinorStat,
        pContext,
        GSS_C_NT_STRING_UID_NAME,
        &SessionKey);
    if (MajorStat == GSS_S_UNAVAILABLE)
    {
        printf("Context username oid unavailable\n");
    }
    else
    {
        if (MajorStat != GSS_S_COMPLETE)
        {
            BAIL_ON_ERROR(MinorStat);
        }

        printf("Context for username (by oid): ");
        if(!SessionKey->elements->length)
        {
            printf("(no name... is this a server context?)");
        }
        else
        {
            for(nIndex = 0; nIndex < SessionKey->elements->length; nIndex++)
            {
                printf("%c", ((PCHAR)(SessionKey->elements->value))[nIndex]);
            }
        }
        printf("\n");
    }

    MajorStat = gss_inquire_context(
        &MinorStat,
        pContext,
        &UserName,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (MajorStat != GSS_S_COMPLETE)
    {
        BAIL_ON_ERROR(MinorStat);
    }

    MajorStat = gss_display_name(
        &MinorStat,
        UserName,
        &ClientNameBuffer,
        &nameOid);
    if (MajorStat != GSS_S_COMPLETE)
    {
        BAIL_ON_ERROR(MinorStat);
    }

    printf("Context for username: ");
    if(!ClientNameBuffer.length)
    {
        printf("(no name... is this a server context?)");
    }
    else
    {
        for(nIndex = 0; nIndex < ClientNameBuffer.length; nIndex++)
        {
            printf("%c", ((PCHAR)(ClientNameBuffer.value))[nIndex]);
        }
    }
    printf("\n");

    MajorStat = gss_release_buffer(&MinorStat, &ClientNameBuffer);
    if (MajorStat != GSS_S_COMPLETE)
    {
        BAIL_ON_ERROR(MajorStat);
    }

    MajorStat = gss_release_name(&MinorStat, &UserName);
    if (MajorStat != GSS_S_COMPLETE)
    {
        BAIL_ON_ERROR(MajorStat);
    }

    MajorStat = gss_inquire_sec_context_by_oid(
        &MinorStat,
        pContext,
        GSS_C_INQ_SSPI_SESSION_KEY,
        &SessionKey);
    if (MajorStat != GSS_S_COMPLETE)
    {
        BAIL_ON_ERROR(MinorStat);
    }

    printf("Session Key:\n");
    PrintHexDump(SessionKey->elements->length,
            SessionKey->elements->value,
            FALSE);
    printf("\n");

    /* Receive the message token */
    nError = RecvToken(nSocket, &TransmitBuffer);
    BAIL_ON_ERROR(nError);

    printf("RECEIVED:\n");
    PrintHexDump(TransmitBuffer.length, TransmitBuffer.value, FALSE);
    printf("\n");

    nError =
        gss_unwrap(
            &MinorStat,
            pContext,
            &TransmitBuffer,
            &MsgBuffer,
            &nConfState,
            (gss_qop_t *) NULL
            );

    if ((GSS_S_FAILURE == nError) && MinorStat)
    {
        nError = MinorStat;
    }

    BAIL_ON_ERROR(nError);

    printf("Successfully decoded '%.*s'\n",
            (int)MsgBuffer.length,
            (char *)MsgBuffer.value);
    if (!nConfState)
    {
        printf("Message was unencrypted\n");
    }
    else
    {
        printf("Message was encrypted\n");
    }

    if (TransmitBuffer.value)
    {
        free(TransmitBuffer.value);
        TransmitBuffer.value = 0;
    }

    /* Produce a signature block for the message */
    nError = gss_get_mic(
        &MinorStat,
        pContext,
        GSS_C_QOP_DEFAULT,
        &MsgBuffer,
        &TransmitBuffer
        );

    if ((GSS_S_FAILURE == nError) && MinorStat)
    {
        nError = MinorStat;
    }

    BAIL_ON_ERROR(nError);

    if (MsgBuffer.value)
    {
        free(MsgBuffer.value);
        MsgBuffer.value = 0;
    }

    /* Send the signature block to the client */
    nError = SendToken(nSocket, &TransmitBuffer);
    BAIL_ON_ERROR(nError);

    if (TransmitBuffer.value)
    {
        free(TransmitBuffer.value);
        TransmitBuffer.value = 0;
    }

    if (pContext != GSS_C_NO_CONTEXT)
    {
        // Delete context
        nError = gss_delete_sec_context(&MinorStat, &pContext, NULL);

        if ((GSS_S_FAILURE == nError) && MinorStat)
        {
            nError = MinorStat;
        }

        BAIL_ON_ERROR(nError);
    }

finish:
    return nError;
error:
    if (TransmitBuffer.value)
    {
        free(TransmitBuffer.value);
        TransmitBuffer.value = 0;
    }
    if (MsgBuffer.value)
    {
        free(MsgBuffer.value);
        MsgBuffer.value = 0;
    }
    if (pContext != GSS_C_NO_CONTEXT)
    {
        // Delete context
        gss_delete_sec_context(&MinorStat, &pContext, NULL);
    }
    goto finish;
}

DWORD
ServerEstablishContext(
    IN INT nSocket,
    IN gss_cred_id_t pServerCreds,
    OUT gss_ctx_id_t *ppContext
    )
{
    DWORD nError = 0;
    gss_buffer_desc SendTokenBuffer;
    gss_buffer_desc RecvTokenBuffer;
    gss_OID pObjectId = NULL;
    OM_uint32 MajorStat = 0;
    OM_uint32 MinorStat = 0;
    OM_uint32 AccSecMinorStat = 0;

    memset(&SendTokenBuffer, 0, sizeof(gss_buffer_desc));
    memset(&RecvTokenBuffer, 0, sizeof(gss_buffer_desc));

    *ppContext = GSS_C_NO_CONTEXT;

    do
    {
        nError = RecvToken(nSocket, &RecvTokenBuffer);
        BAIL_ON_ERROR(nError);

        printf("RECEIVED:\n");
        PrintHexDump(RecvTokenBuffer.length, RecvTokenBuffer.value, TRUE);
        printf("\n");

        MajorStat =
            gss_accept_sec_context(
                &AccSecMinorStat,
                ppContext,
                pServerCreds,
                &RecvTokenBuffer,
                GSS_C_NO_CHANNEL_BINDINGS,
                NULL,
                &pObjectId,
                &SendTokenBuffer,
                NULL,
                NULL,   /* ignore time_rec */
                NULL
                );  /* ignore del_cred_handle */

        if (MajorStat != GSS_S_COMPLETE &&
            MajorStat != GSS_S_CONTINUE_NEEDED)
        {
            nError = MajorStat;
            if ((GSS_S_FAILURE == nError) && AccSecMinorStat)
            {
                nError = AccSecMinorStat;
            }
            BAIL_ON_ERROR(nError);
        }

        if (MajorStat == GSS_S_CONTINUE_NEEDED)
        {
            printf("Context partially accepted...\n");
            PrintHexDump(SendTokenBuffer.length, SendTokenBuffer.value, TRUE);
            printf("\n");
        }
        else
        {
            printf("Context FULLY accepted!\n");
        }

        if (RecvTokenBuffer.value)
        {
            free(RecvTokenBuffer.value);
            RecvTokenBuffer.value = NULL;
        }

        if (SendTokenBuffer.length != 0)
        {
            nError = SendToken(nSocket, &SendTokenBuffer);
            BAIL_ON_ERROR(nError);

            (void) gss_release_buffer(&MinorStat, &SendTokenBuffer);
        }

    } while (MajorStat == GSS_S_CONTINUE_NEEDED);

finish:
    return nError;
error:
    if (*ppContext != GSS_C_NO_CONTEXT)
    {
        gss_delete_sec_context(
            &MinorStat,
            ppContext,
            GSS_C_NO_BUFFER
            );
        *ppContext = GSS_C_NO_CONTEXT;
    }

    if (RecvTokenBuffer.value)
    {
        free(RecvTokenBuffer.value);
        RecvTokenBuffer.value = NULL;
    }

    if (SendTokenBuffer.length != 0)
    {
        gss_release_buffer(&MinorStat, &SendTokenBuffer);
    }
    goto finish;
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

    pToken->length = (size_t)((BufferAddr[0] << 24)
           | (BufferAddr[1] << 16)
           | (BufferAddr[2] << 8)
           | BufferAddr[3]);

    pToken->value = (PCHAR) malloc(pToken->length ? pToken->length : 1);

    if (pToken->length && pToken->value == NULL)
    {
        nError = ENOMEM;
        BAIL_ON_ERROR(nError);
    }

    nError =
        ReadAll(
            nSocket,
            (PCHAR) pToken->value,
            (UINT)pToken->length,
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
    DWORD nError = 0;
    INT nReturn = 0;
    PCHAR pTrav = NULL;

    *pBytesRead = 0;
    memset(pBuffer, 0, nByte);

    for (pTrav = pBuffer; nByte; pTrav += nReturn, nByte -= (UINT)nReturn)
    {
        nReturn = (INT)recv(nSocket, pTrav, nByte, 0);

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

    *pBytesRead = (INT)(pTrav - pBuffer);

error:
    return nError;
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

    nError =
        WriteAll(
            nSocket,
            pToken->value,
            (UINT)pToken->length,
            &nBytesWritten
            );

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
        nReturn = (INT)send(nSocket, pTrav, nByte, 0);

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

    *pBytesWritten = (INT)(pTrav - pBuffer);

error:
    return nError;
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
    PVOID pBuffer,
    INT bIsNtlmMessage
    )
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    UCHAR cbLine;
    PBYTE pToken = (PBYTE)pBuffer;
    PBYTE pLine = (PBYTE)pBuffer;

    for (index = 0; dwLength;
        dwLength -= count, pLine += count, index += count)
    {
        count = (dwLength > 16) ? 16:dwLength;

        sprintf(rgbLine, "%4.4x  ",index);
        cbLine = 6;

        for (i=0;i<count;i++)
        {
            rgbLine[cbLine++] = rgbDigits[pLine[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[pLine[i] & 0x0f];
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
            if (pLine[i] < 32 || pLine[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = pLine[i];
            }
        }

        rgbLine[cbLine++] = 0;
        printf("%s\n", rgbLine);
    }

    if (bIsNtlmMessage)
    {
        DumpNtlmMessage(pToken, dwLength);
    }
}
