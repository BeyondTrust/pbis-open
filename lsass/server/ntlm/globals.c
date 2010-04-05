/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NTLM Server API Globals
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "ntlmsrvapi.h"

LWMsgDispatchSpec gNtlmMessageHandlers[] =
{
    LWMSG_DISPATCH(NTLM_Q_ACCEPT_SEC_CTXT, NtlmSrvIpcAcceptSecurityContext),
    LWMSG_DISPATCH(NTLM_Q_ACQUIRE_CREDS, NtlmSrvIpcAcquireCredentialsHandle),
    LWMSG_DISPATCH(NTLM_Q_DECRYPT_MSG, NtlmSrvIpcDecryptMessage),
    LWMSG_DISPATCH(NTLM_Q_DELETE_SEC_CTXT, NtlmSrvIpcDeleteSecurityContext),
    LWMSG_DISPATCH(NTLM_Q_ENCRYPT_MSG, NtlmSrvIpcEncryptMessage),
    LWMSG_DISPATCH(NTLM_Q_EXPORT_SEC_CTXT, NtlmSrvIpcExportSecurityContext),
    LWMSG_DISPATCH(NTLM_Q_FREE_CREDS, NtlmSrvIpcFreeCredentialsHandle),
    LWMSG_DISPATCH(NTLM_Q_IMPORT_SEC_CTXT, NtlmSrvIpcImportSecurityContext),
    LWMSG_DISPATCH(NTLM_Q_INIT_SEC_CTXT, NtlmSrvIpcInitializeSecurityContext),
    LWMSG_DISPATCH(NTLM_Q_MAKE_SIGN, NtlmSrvIpcMakeSignature),
    LWMSG_DISPATCH(NTLM_Q_QUERY_CREDS, NtlmSrvIpcQueryCredentialsAttributes),
    LWMSG_DISPATCH(NTLM_Q_QUERY_CTXT, NtlmSrvIpcQueryContextAttributes),
    LWMSG_DISPATCH(NTLM_Q_VERIFY_SIGN, NtlmSrvIpcVerifySignature),
    LWMSG_DISPATCH_END
};

WIN_VERSION_INFO gW2KSpoof = {5, 0, 2195, 0x0f000000};
WIN_VERSION_INFO gXpSpoof = {5, 1, 2600, 0x0f000000};
