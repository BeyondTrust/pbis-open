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

typedef NTSTATUS
        (*PFN_SRV_STAT_CREATE_REQUEST_CONTEXT)(
            PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
            SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
            ULONG                      ulRequestLength,    /* IN              */
            PHANDLE                    hContext            /*    OUT          */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_PUSH_MESSAGE)(
            HANDLE                       hContext,         /* IN              */
            ULONG                        ulOpcode,         /* IN              */
            PBYTE                        pMessage,         /* IN     OPTIONAL */
            ULONG                        ulMessageLen      /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_SESSION_INFO)(
            HANDLE                    hContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_SUB_OP_CODE)(
            HANDLE                     hContext,           /* IN              */
            ULONG                      ulSubOpcode         /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_IOCTL)(
            HANDLE                    hContext,            /* IN              */
            ULONG                     ulIoCtlCode          /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_POP_MESSAGE)(
            HANDLE                    hContext,            /* IN              */
            ULONG                     ulOpCode,            /* IN              */
            NTSTATUS                  msgStatus            /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_SET_RESPONSE_INFO)(
            HANDLE                    hContext,            /* IN              */
            NTSTATUS                  responseStatus,      /* IN              */
            PBYTE                     pResponseBuffer,     /* IN     OPTIONAL */
            ULONG                     ulResponseLength     /* IN              */
            );

typedef NTSTATUS
        (*PFN_SRV_STAT_CLOSE_REQUEST_CONTEXT)(
            HANDLE                     hContext            /* IN              */
            );

typedef struct _LWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE
{
    PFN_SRV_STAT_CREATE_REQUEST_CONTEXT pfnCreateRequestContext;
    PFN_SRV_STAT_PUSH_MESSAGE           pfnPushMessage;
    PFN_SRV_STAT_SET_SUB_OP_CODE        pfnSetSubOpCode;
    PFN_SRV_STAT_SET_IOCTL              pfnSetIOCTL;
    PFN_SRV_STAT_SET_SESSION_INFO       pfnSetSessionInfo;
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
