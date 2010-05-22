/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwiosrvstatprovider.h
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem (LWIO)
 *
 *        SRV Driver
 *
 *        Statistics Provider Interface
 *
 * Author: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#ifndef __LWIO_SRV_STATPROVIDER_H__
#define __LWIO_SRV_STATPROVIDER_H__

typedef enum
{
    SRV_STAT_SMB_VERSION_UNKNOWN = 0,
    SRV_STAT_SMB_VERSION_1,
    SRV_STAT_SMB_VERSION_2
} SRV_STAT_SMB_VERSION;

typedef struct _SRV_STAT_CONNECTION_INFO
{
    struct sockaddr clientAddress;
    size_t          clientAddrLen;
    ULONG           ulResourceId;

} SRV_STAT_CONNECTION_INFO, *PSRV_STAT_CONNECTION_INFO;

typedef struct _SRV_STAT_SESSION_INFO
{
    PWSTR   pwszUserPrincipal;
    ULONG64 ullSessionId;

} SRV_STAT_SESSION_INFO, *PSRV_STAT_SESSION_INFO;

typedef struct _SRV_STAT_TREE_INFO
{
    PWSTR pwszShareName;
    ULONG ulTreeId;

} SRV_STAT_TREE_INFO, *PSRV_STAT_TREE_INFO;

typedef struct _SRV_STAT_FILE_INFO
{
    PWSTR   pwszFileName;
    ULONG64 ullVolatileFileId;
    ULONG64 ullPersistentFileId;

} SRV_STAT_FILE_INFO, *PSRV_STAT_FILE_INFO;

typedef struct _SRV_STAT_CREATE_PARAMS_SMB_V1
{

} SRV_STAT_CREATE_PARAMS_SMB_V1, *PSRV_STAT_CREATE_PARAMS_SMB_V1;

typedef struct _SRV_STAT_CREATE_PARAMS_SMB_V2
{

} SRV_STAT_CREATE_PARAMS_SMB_V2, *PSRV_STAT_CREATE_PARAMS_SMB_V2;

typedef struct _SRV_STAT_CLOSE_PARAMS_SMB_V1
{

} SRV_STAT_CLOSE_PARAMS_SMB_V1, *PSRV_STAT_CLOSE_PARAMS_SMB_V1;

typedef struct _SRV_STAT_CLOSE_PARAMS_SMB_V2
{

} SRV_STAT_CLOSE_PARAMS_SMB_V2, *PSRV_STAT_CLOSE_PARAMS_SMB_V2;

typedef struct _SRV_STAT_REQUEST_CONTEXT* PSRV_STAT_REQUEST_CONTEXT;

typedef struct _SRV_STAT_REQUEST_PARAMETERS
{
    PSRV_STAT_CONNECTION_INFO pConnectionInfo;
    PSRV_STAT_SESSION_INFO    pSessionInfo;
    PSRV_STAT_TREE_INFO       pTreeInfo;

    ULONG                     ulOpCode;
    union
    {
        PSRV_STAT_CREATE_PARAMS_SMB_V1 pCreateParams_SMB_V1;
        PSRV_STAT_CREATE_PARAMS_SMB_V2 pCreateParams_SMB_V2;

        PSRV_STAT_CLOSE_PARAMS_SMB_V1  pCloseParams_SMB_V1;
        PSRV_STAT_CLOSE_PARAMS_SMB_V2  pCloseParams_SMB_V2;
    };

} SRV_STAT_REQUEST_PARAMETERS, *PSRV_STAT_REQUEST_PARAMETERS;

typedef NTSTATUS
        (*PFN_SRV_STAT_CREATE_REQUEST_CONTEXT)(
            PSRV_STAT_REQUEST_CONTEXT* ppContext           /*    OUT          */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_REQUEST_INFO)(
            PSRV_STAT_REQUEST_CONTEXT  pContext,           /* IN              */
            SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
            ULONG                      ulRequestLength     /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_PUSH_MESSAGE)(
            PSRV_STAT_REQUEST_CONTEXT    pContext,         /* IN              */
            ULONG                        ulOpcode,         /* IN              */
            PSRV_STAT_REQUEST_PARAMETERS pParams,
            PBYTE                        pMessage,         /* IN     OPTIONAL */
            ULONG                        ulMessageLen      /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_SUB_OP_CODE)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            ULONG                     ulSubOpcode          /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_IOCTL)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            ULONG                     ulIoCtlCode          /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SESSION_CREATED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_TREE_CREATED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
            PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_FILE_CREATED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
            PSRV_STAT_TREE_INFO       pTreeInfo,           /* IN              */
            PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_FILE_CLOSED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_TREE_CLOSED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SESSION_CLOSED)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_POP_MESSAGE)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            ULONG                     ulOpCode,            /* IN              */
            NTSTATUS                  msgStatus            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_RESPONSE_INFO)(
            PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
            NTSTATUS                  responseStatus,      /* IN              */
            PBYTE                     pResponseBuffer,     /* IN     OPTIONAL */
            ULONG                     ulResponseLength     /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_CLOSE_REQUEST_CONTEXT)(
            PSRV_STAT_REQUEST_CONTEXT pContext             /* IN              */
            );

typedef struct _LWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE
{
    PFN_SRV_STAT_CREATE_REQUEST_CONTEXT pfnCreateRequestContext;
    PFN_SRV_STAT_SET_REQUEST_INFO       pfnSetRequestInfo;
    PFN_SRV_STAT_PUSH_MESSAGE           pfnPushMessage;
    PFN_SRV_STAT_SET_SUB_OP_CODE        pfnSetSubOpCode;
    PFN_SRV_STAT_SET_IOCTL              pfnSetIOCTL;
    PFN_SRV_STAT_SESSION_CREATED        pfnSessionCreated;
    PFN_SRV_STAT_TREE_CREATED           pfnTreeCreated;
    PFN_SRV_STAT_FILE_CREATED           pfnFileCreated;
    PFN_SRV_STAT_FILE_CLOSED            pfnFileClosed;
    PFN_SRV_STAT_TREE_CLOSED            pfnTreeClosed;
    PFN_SRV_STAT_SESSION_CLOSED         pfnSessionClosed;
    PFN_SRV_STAT_POP_MESSAGE            pfnPopMessage;
    PFN_SRV_STAT_SET_RESPONSE_INFO      pfnSetResponseInfo;
    PFN_SRV_STAT_CLOSE_REQUEST_CONTEXT  pfnCloseRequestContext;

}   LWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE,
  *PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE;

typedef NTSTATUS (*PFN_INIT_SRV_STAT_PROVIDER)(
                    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE* ppFnTable /* OUT */
                    );

typedef NTSTATUS (*PFN_SHUTDOWN_SRV_STAT_PROVIDER)(
                    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pFnTable   /* IN  */
                    );

#define LWIO_SYMBOL_NAME_INIT_SRV_STAT_PROVIDER  "LwioSrvStatInitializeProvider"
#define LWIO_SYMBOL_NAME_CLOSE_SRV_STAT_PROVIDER "LwioSrvStatShutdownProvider"

#endif /* __LWIO_SRV_STATPROVIDER_H__ */
