/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        smb1.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        SMB V1 Protocol Handler API
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __SMB_V1_H__
#define __SMB_V1_H__

NTSTATUS
NfsProtocolInit_SMB_V1(
    PSMB_PROD_CONS_QUEUE pWorkQueue
    );

NTSTATUS
NfsBuildNegotiateResponse_SMB_V1_NTLM_0_12(
    PLWIO_NFS_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    USHORT               idxDialect,
    PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
NfsBuildNegotiateResponse_SMB_V1_Invalid(
    PLWIO_NFS_CONNECTION pConnection,
    PSMB_PACKET  pSmbRequest,
    PSMB_PACKET* ppSmbResponse
    );

NTSTATUS
NfsProtocolExecute_SMB_V1(
    PNFS_EXEC_CONTEXT pContext
    );

NTSTATUS
NfsProtocolBuildErrorResponse_SMB_V1(
    PLWIO_NFS_CONNECTION pConnection,
    PSMB_HEADER          pRequestHeader,
    NTSTATUS             errorStatus,
    PSMB_PACKET*         ppSmbResponse
    );

VOID
NfsProtocolFreeContext_SMB_V1(
    PNFS_EXEC_CONTEXT_SMB_V1 pProtocolContext
    );

VOID
NfsProtocolShutdown_SMB_V1(
    VOID
    );

#endif /* __SMB_V1_H__ */

