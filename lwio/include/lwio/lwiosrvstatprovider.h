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

/**
 * @brief Protocol Version of the SMB request
 *
 */
typedef enum
{
    SRV_STAT_SMB_VERSION_UNKNOWN = 0,
    SRV_STAT_SMB_VERSION_1,
    SRV_STAT_SMB_VERSION_2
} SRV_STAT_SMB_VERSION;

/**
 * @brief Information about the client connection.
 *
 * This is used to classify the statistics per client.
 *
 */
typedef struct _SRV_STAT_CONNECTION_INFO
{
    /**
     * IP Address of the (remote) client.
     */
    const struct sockaddr* pClientAddress;
    /**
     * Length of the client IP Address
     */
    size_t          clientAddrLen;
    /**
     * IP Address of the (local) end point on the server.
     *
     * This is used to determine which network interface is being used.
     */
    const struct sockaddr* pServerAddress;
    /**
     * Length of the server IP Address
     */
    size_t          serverAddrLen;
    /**
     * The resource id assigned (by the srv driver) to the client.
     */
    ULONG           ulResourceId;

} SRV_STAT_CONNECTION_INFO, *PSRV_STAT_CONNECTION_INFO;

/**
 * @brief Information about the credentials used to authenticate the connection.
 *
 */
typedef struct _SRV_STAT_SESSION_INFO
{
    /**
     * User principal used to authenticate the connection.
     */
    PCWSTR  pwszUserPrincipal;
    /**
     * User Id associated with the authentication credentials.
     */
    ULONG   ulUid;
    /**
     * Primary group id associated with the authentication credentials.
     */
    ULONG   ulGid;
    /**
     * Session id associated with the connection in the current process.
     * If the protocol version is SMB1, this will be a 16 bit value.
     * If the protocol version is SMB2, this will be a 64 bit value.
     */
    ULONG64 ullSessionId;

} SRV_STAT_SESSION_INFO, *PSRV_STAT_SESSION_INFO;

/**
 * @brief Method to create the statistics context that is associated with each
 *        incoming SMB request. This method will be called once for each SMB
 *        request being processed by the server.
 *
 * @param[in] pConnection     Information about the client connection
 * @param[in] protocolVersion SMB Protocol Version for this SMB request
 * @param[in] ulRequestLength Length of the entire SMB request. This will
 *                            include the length of the NETBIOS header (4 bytes)
 * @param[out] hContext       Handle to the statistics context
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_CREATE_REQUEST_CONTEXT)(
            PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
            SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
            ULONG                      ulRequestLength,    /* IN              */
            PHANDLE                    hContext            /*    OUT          */
            );

/**
 * @brief Method to indicate the beginning of processing for an SMB message
 *        that belongs in the current SMB request, associated with the
 *        statistics context. This method will be called once for each chained
 *        message within an SMB request.
 *
 * @param[in] hContext     The statistics context to which the current message
 *                         is a part of.
 * @param[in] ulOpcode     The SMB operation code. This value is dependent on
 *                         the protocol version.
 * @param[in] ulMessageLen Length of the SMB message. This will not include the
 *                         NetBIOS header (4 bytes). In the case of an SMB1
 *                         message, the first call will include the SMB header
 *                         and subsequent calls for any chained messages will
 *                         include the length from the AndX offset. In the case
 *                         of SMB2 messages, this value indicates the length of
 *                         each chained SMB2 message including the SMB2 header.
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_PUSH_MESSAGE)(
            HANDLE                       hContext,         /* IN              */
            ULONG                        ulOpcode,         /* IN              */
            ULONG                        ulMessageLen      /* IN              */
            );

/**
 * @brief Method to set credential information in the current statistics
 *        context. This method is called once after creating the context.
 *
 * @param[in] hContext     Handle to the current statistics context.
 * @param[in] pSessionInfo Credential information
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_SET_SESSION_INFO)(
            HANDLE                    hContext,            /* IN              */
            PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
            );

/**
 * @brief Method to set the sub operation code in the current message being
 *        processed. This method will be called once for each chained message
 *        in the SMB request. The sub operation code is always associated with
 *        (main) operation code that was specified when starting to process
 *        the chained message.
 *
 * @param[in] hContext    Handle to the current statistics context.
 * @param[in] ulSubOpcode Sub operation code.
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_SET_SUB_OP_CODE)(
            HANDLE                     hContext,           /* IN              */
            ULONG                      ulSubOpcode         /* IN              */
            );

/**
 * @brief Method to set the IOCTL code belonging to a chained message.
 *        This method is called only if the current chained message is an IOCTL
 *        request. This method may be called once per chained message.
 *
 * @param[in] hContext    Handle to the current statistics context.
 * @param[in] ulIoCtlCode IOCTL code.
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_SET_IOCTL)(
            HANDLE                    hContext,            /* IN              */
            ULONG                     ulIoCtlCode          /* IN              */
            );

/**
 * @brief Method to indicate that the driver has completed processing the
 *        current message. Each invocation to this method must be matched to
 *        exactly one call to push the corresponding message.
 *
 * @param[in] hContext         Handle to the current statistics context.
 * @param[in] ulOpCode         Operation code belonging to the current message
 *                             that is being processed.
 * @param[in] ulResponseLength Length of the response for this message.
 * @param[in] msgStatus        Resultant NTSTATUS code of the (chained) message.
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_POP_MESSAGE)(
            HANDLE                    hContext,            /* IN              */
            ULONG                     ulOpCode,            /* IN              */
            ULONG                     ulResponseLength,    /* IN              */
            NTSTATUS                  msgStatus            /* IN              */
            );

/**
 * @brief Method to set information about the response to the entire SMB
 *        SMB request that is being processed. This call can be considered as
 *        the end of processing for the SMB request and the statistics context.
 *
 * @param[in] hContext         Handle to the current statistics context
 *                             associated with the SMB request.
 * @param[in] ulResponseLength Length of the response to the entire SMB request.
 *                             This will include the responses of any chained
 *                             messages. Also, this response length includes the
 *                             length of the NETBIOS header (4 bytes).
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_SET_RESPONSE_INFO)(
            HANDLE                    hContext,            /* IN              */
            ULONG                     ulResponseLength     /* IN              */
            );

/**
 * @brief Method to free the statistics context that was associated with an
 *        SMB request. This method is called when the execution context for the
 *        SMB request is being freed.
 *
 * @param[in] hContext Handle to the current statistics context.
 */
typedef NTSTATUS
        (*PFN_SRV_STAT_CLOSE_REQUEST_CONTEXT)(
            HANDLE                     hContext            /* IN              */
            );

/**
 * @brief Primary interface/contract between the srv driver and a third
 *        party statistics driver. The entry points within this function table
 *        are invoked to provide information about various phases of processing
 *        an SMB request.
 */
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

/**
 * @brief Function prototype of the main entry point within the statistics
 *        provider. The srv driver invokes this method after dynamically
 *        loading the statistics provider. The statistics provider is expected
 *        to return an interface table of methods which are called during
 *        various phases of request processing in srv.
 *
 *        If the logging of statistics in the srv driver was disabled when srv
 *        was activated, the statistics provider will not be dynamically loaded.
 *        If the statistics logging was dynamically disabled at runtime, the
 *        srv driver will not dynamically unload the statistics provider.
 */
typedef NTSTATUS (*PFN_INIT_SRV_STAT_PROVIDER)(
                    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE* ppFnTable /* OUT */
                    );

/**
 * @brief Function prototype of the entry point within the statistics provider.
 *        The srv driver invokes this method before dynamically unloading the
 *        statistics provider.
 */
typedef NTSTATUS (*PFN_SHUTDOWN_SRV_STAT_PROVIDER)(
                    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pFnTable   /* IN  */
                    );

/**
 * @brief Name of the symbol that the srv driver will search for in the
 *        statistics provider after dynamically loading it. The statistics
 *        provider must export this symbol which should have a function
 *        prototype corresponding to the signature of PFN_INIT_SRV_STAT_PROVIDER
 *        defined above.
 */
#define LWIO_SYMBOL_NAME_INIT_SRV_STAT_PROVIDER  "LwioSrvStatInitializeProvider"

/**
 * @brief Name of the symbol that the srv driver will search for in the
 *        statistics provider before dynamically unloading it. The statistics
 *        provider must export this symbol which should have a function
 *        prototype corresponding to the signature of
 *        PFN_SHUTDOWN_SRV_STAT_PROVIDER defined above.
 */
#define LWIO_SYMBOL_NAME_CLOSE_SRV_STAT_PROVIDER "LwioSrvStatShutdownProvider"

#endif /* __LWIO_SRV_STATPROVIDER_H__ */
