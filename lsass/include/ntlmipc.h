/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lsaipc.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __NTLMIPC_H__
#define __NTLMIPC_H__

#include <lwmsg/lwmsg.h>
#include <ntlm/sspintlm.h>

#define NTLM_SERVER_FILENAME ".ntlmd"

typedef enum __NTLM_IPC_TAG
{
    NTLM_R_GENERIC_FAILURE,
    NTLM_Q_ACCEPT_SEC_CTXT,
    NTLM_R_ACCEPT_SEC_CTXT_SUCCESS,
    NTLM_Q_ACQUIRE_CREDS,
    NTLM_R_ACQUIRE_CREDS_SUCCESS,
    NTLM_Q_DECRYPT_MSG,
    NTLM_R_DECRYPT_MSG_SUCCESS,
    NTLM_Q_DELETE_SEC_CTXT,
    NTLM_R_DELETE_SEC_CTXT_SUCCESS,
    NTLM_Q_ENCRYPT_MSG,
    NTLM_R_ENCRYPT_MSG_SUCCESS,
    NTLM_Q_EXPORT_SEC_CTXT,
    NTLM_R_EXPORT_SEC_CTXT_SUCCESS,
    NTLM_Q_FREE_CREDS,
    NTLM_R_FREE_CREDS_SUCCESS,
    NTLM_Q_IMPORT_SEC_CTXT,
    NTLM_R_IMPORT_SEC_CTXT_SUCCESS,
    NTLM_Q_INIT_SEC_CTXT,
    NTLM_R_INIT_SEC_CTXT_SUCCESS,
    NTLM_Q_MAKE_SIGN,
    NTLM_R_MAKE_SIGN_SUCCESS,
    NTLM_Q_QUERY_CREDS,
    NTLM_R_QUERY_CREDS_SUCCESS,
    NTLM_Q_QUERY_CTXT,
    NTLM_R_QUERY_CTXT_SUCCESS,
    NTLM_Q_SET_CREDS,
    NTLM_R_SET_CREDS_SUCCESS,
    NTLM_Q_VERIFY_SIGN,
    NTLM_R_VERIFY_SIGN_SUCCESS
} NTLM_IPC_TAG;

/******************************************************************************/

typedef struct __NTLM_IPC_ERROR
{
    DWORD dwError;
} NTLM_IPC_ERROR, *PNTLM_IPC_ERROR;

/******************************************************************************/

typedef struct __NTLM_IPC_ACCEPT_SEC_CTXT_REQ
{
    LWMsgHandle* hCredential;
    LWMsgHandle* hContext;
    const SecBuffer* pInput;
    DWORD fContextReq;
    DWORD TargetDataRep;
} NTLM_IPC_ACCEPT_SEC_CTXT_REQ, *PNTLM_IPC_ACCEPT_SEC_CTXT_REQ;

typedef struct __NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE
{
    LWMsgHandle* hNewContext;
    SecBuffer Output;
    DWORD  fContextAttr;
    TimeStamp tsTimeStamp;
    DWORD dwStatus;
} NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, *PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_ACQUIRE_CREDS_REQ
{
    const SEC_CHAR *pszPrincipal;
    const SEC_CHAR *pszPackage;
    DWORD fCredentialUse;
    PLUID pvLogonID;
    PVOID pAuthData;
} NTLM_IPC_ACQUIRE_CREDS_REQ, *PNTLM_IPC_ACQUIRE_CREDS_REQ;

typedef struct __NTLM_IPC_ACQUIRE_CREDS_RESPONSE
{
    LWMsgHandle* hCredential;
    TimeStamp tsExpiry;
} NTLM_IPC_ACQUIRE_CREDS_RESPONSE, *PNTLM_IPC_ACQUIRE_CREDS_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_DECRYPT_MSG_REQ
{
    LWMsgHandle* hContext;
    const SecBufferDesc* pMessage;
    DWORD MessageSeqNo;
} NTLM_IPC_DECRYPT_MSG_REQ, *PNTLM_IPC_DECRYPT_MSG_REQ;

typedef struct __NTLM_IPC_DECRYPT_MSG_RESPONSE
{
    SecBufferDesc Message;
    BOOLEAN bEncrypted;
} NTLM_IPC_DECRYPT_MSG_RESPONSE, *PNTLM_IPC_DECRYPT_MSG_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_DELETE_SEC_CTXT_REQ
{
    LWMsgHandle* hContext;
} NTLM_IPC_DELETE_SEC_CTXT_REQ, *PNTLM_IPC_DELETE_SEC_CTXT_REQ;

// No Response

/******************************************************************************/

typedef struct __NTLM_IPC_ENCRYPT_MSG_REQ
{
    LWMsgHandle* hContext;
    BOOLEAN bEncrypt;
    const SecBufferDesc* pMessage;
    DWORD MessageSeqNo;
} NTLM_IPC_ENCRYPT_MSG_REQ, *PNTLM_IPC_ENCRYPT_MSG_REQ;

typedef struct __NTLM_IPC_ENCRYPT_MSG_RESPONSE
{
    SecBufferDesc Message;
} NTLM_IPC_ENCRYPT_MSG_RESPONSE, *PNTLM_IPC_ENCRYPT_MSG_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_EXPORT_SEC_CTXT_REQ
{
    LWMsgHandle* hContext;
    DWORD fFlags;
} NTLM_IPC_EXPORT_SEC_CTXT_REQ, *PNTLM_IPC_EXPORT_SEC_CTXT_REQ;

typedef struct __NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE
{
    SecBuffer PackedContext;
} NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE, *PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_FREE_CREDS_REQ
{
    LWMsgHandle* hCredential;
} NTLM_IPC_FREE_CREDS_REQ, *PNTLM_IPC_FREE_CREDS_REQ;

// No Response

/******************************************************************************/

typedef struct __NTLM_IPC_IMPORT_SEC_CTXT_REQ
{
    PSecBuffer pPackedContext;
} NTLM_IPC_IMPORT_SEC_CTXT_REQ, *PNTLM_IPC_IMPORT_SEC_CTXT_REQ;

typedef struct __NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE
{
    LWMsgHandle* hContext;
} NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE, *PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_INIT_SEC_CTXT_REQ
{
    LWMsgHandle* hCredential;
    LWMsgHandle* hContext;
    SEC_CHAR * pszTargetName;
    DWORD fContextReq;
    DWORD Reserved1;
    DWORD TargetDataRep;
    const SecBuffer* pInput;
    DWORD Reserved2;
} NTLM_IPC_INIT_SEC_CTXT_REQ, *PNTLM_IPC_INIT_SEC_CTXT_REQ;

typedef struct __NTLM_IPC_INIT_SEC_CTXT_RESPONSE
{
    LWMsgHandle* hNewContext;
    SecBuffer Output;
    DWORD fContextAttr;
    TimeStamp tsExpiry;
    DWORD dwStatus;
} NTLM_IPC_INIT_SEC_CTXT_RESPONSE, *PNTLM_IPC_INIT_SEC_CTXT_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_MAKE_SIGN_REQ
{
    LWMsgHandle* hContext;
    DWORD dwQop;
    const SecBufferDesc* pMessage;
    DWORD MessageSeqNo;
} NTLM_IPC_MAKE_SIGN_REQ, *PNTLM_IPC_MAKE_SIGN_REQ;

typedef struct __NTLM_IPC_MAKE_SIGN_RESPONSE
{
    SecBufferDesc Message;
} NTLM_IPC_MAKE_SIGN_RESPONSE, *PNTLM_IPC_MAKE_SIGN_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_QUERY_CREDS_REQ
{
    LWMsgHandle* hCredential;
    DWORD ulAttribute;
} NTLM_IPC_QUERY_CREDS_REQ, *PNTLM_IPC_QUERY_CREDS_REQ;

typedef struct __NTLM_IPC_QUERY_CREDS_RESPONSE
{
    DWORD ulAttribute;
    SecPkgCred Buffer;
} NTLM_IPC_QUERY_CREDS_RESPONSE, *PNTLM_IPC_QUERY_CREDS_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_QUERY_CTXT_REQ
{
    LWMsgHandle* hContext;
    DWORD ulAttribute;
} NTLM_IPC_QUERY_CTXT_REQ, *PNTLM_IPC_QUERY_CTXT_REQ;

typedef struct __NTLM_IPC_QUERY_CTXT_RESPONSE
{
    DWORD ulAttribute;
    SecPkgContext Buffer;
} NTLM_IPC_QUERY_CTXT_RESPONSE, *PNTLM_IPC_QUERY_CTXT_RESPONSE;

/******************************************************************************/

typedef struct __NTLM_IPC_SET_CREDS_REQ
{
    LWMsgHandle* hCredential;
    DWORD ulAttribute;
    SecPkgCred Buffer;
} NTLM_IPC_SET_CREDS_REQ, *PNTLM_IPC_SET_CREDS_REQ;

// No Response

/******************************************************************************/

typedef struct __NTLM_IPC_VERIFY_SIGN_REQ
{
    LWMsgHandle* hContext;
    const SecBufferDesc* pMessage;
    DWORD MessageSeqNo;
} NTLM_IPC_VERIFY_SIGN_REQ, *PNTLM_IPC_VERIFY_SIGN_REQ;

typedef struct __NTLM_IPC_VERIFY_SIGN_RESPONSE
{
    DWORD dwQop;
} NTLM_IPC_VERIFY_SIGN_RESPONSE, *PNTLM_IPC_VERIFY_SIGN_RESPONSE;

/******************************************************************************/

#define NTLM_MAP_LWMSG_ERROR(_e_) (LwMapLwmsgStatusToLwError(_e_))
#define MAP_NTLM_ERROR_IPC(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

LWMsgProtocolSpec*
NtlmIpcGetProtocolSpec(
    VOID
    );

LWMsgDispatchSpec*
NtlmSrvGetDispatchSpec(
    VOID
    );

#endif /*__NTLMIPC_H__*/


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
