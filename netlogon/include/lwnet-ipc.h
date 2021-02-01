/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwnet-ipc.h
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 *
 *        Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LWNETIPC_H__
#define __LWNETIPC_H__

#include <lwmsg/lwmsg.h>
#include <lwerror.h>

#include "lwnet-utils.h"

typedef struct _LWNET_IPC_ERROR
{
    DWORD dwError;
    PCSTR pszErrorMessage;
} LWNET_IPC_ERROR, *PLWNET_IPC_ERROR;

typedef struct _LWNET_IPC_CONST_STRING {
    PCSTR pszString;
} LWNET_IPC_CONST_STRING, *PLWNET_IPC_CONST_STRING;

typedef struct _LWNET_IPC_STRING {
    PSTR pszString;
} LWNET_IPC_STRING, *PLWNET_IPC_STRING;

typedef struct _LWNET_IPC_TIME {
    LWNET_UNIX_TIME_T Time;
} LWNET_IPC_TIME, *PLWNET_IPC_TIME;

typedef struct _LWNET_IPC_GET_DC
{
    PCSTR pszServerFQDN;
    PCSTR pszDomainFQDN;
    PCSTR pszSiteName;
    PCSTR pszPrimaryDomain;
    DWORD dwFlags;
    DWORD dwBlackListCount;
    PSTR* ppszAddressBlackList;
} LWNET_IPC_GET_DC, *PLWNET_IPC_GET_DC;

typedef struct _LWNET_IPC_DC_LIST {
    PLWNET_DC_ADDRESS pDcList;
    DWORD dwDcCount;
} LWNET_IPC_DC_LIST, *PLWNET_IPC_DC_LIST;

typedef struct LWNET_RESOLVE_NAME_ADDRESS
{
    PWSTR pwszHostName;
} LWNET_RESOLVE_NAME_ADDRESS;

typedef struct LWNET_RESOLVE_NAME_ADDRESS_RESPONSE
{
    PWSTR pwszCanonName;
    PLWNET_RESOLVE_ADDR *ppAddressList;
    DWORD dwAddressListLen;
} LWNET_RESOLVE_NAME_ADDRESS_RESPONSE, *PLWNET_RESOLVE_NAME_ADDRESS_RESPONSE;

typedef enum _LWNET_IPC_TAG
{
    LWNET_R_ERROR, // LWNET_IPC_ERROR
    LWNET_Q_GET_DC_TIME, // LWNET_IPC_CONST_STRING
    LWNET_R_GET_DC_TIME, // LWNET_IPC_TIME
    LWNET_Q_GET_DC_NAME, // LWNET_IPC_GET_DC
    LWNET_R_GET_DC_NAME, // LWNET_DC_INFO
    LWNET_Q_GET_DOMAIN_CONTROLLER, // LWNET_IPC_CONST_STRING
    LWNET_R_GET_DOMAIN_CONTROLLER, // LWNET_IPC_STRING
    LWNET_Q_GET_DC_LIST, // LWNET_IPC_GET_DC
    LWNET_R_GET_DC_LIST, // LWNET_IPC_DC_LIST
    LWNET_Q_RESOLVE_NAME, // LWNET_IPC_RESOLVE_NAME
    LWNET_R_RESOLVE_NAME, // LWNET_IPC_RESOLVE_NAME_REPLY
} LWNET_IPC_TAG;

LWMsgProtocolSpec*
LWNetIPCGetProtocolSpec(
    void
    );

#define MAP_LWMSG_ERROR(_e_) ((_e_) ? LwMapLwmsgStatusToLwError(_e_) : 0)
#define MAP_LWNET_ERROR(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

#endif /*__LWNETIPC_H__*/
