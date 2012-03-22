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

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __LW_IO_CONNECTION
{
    LWMsgPeer* pClient;
    LWMsgSession* pSession;
} IO_CONNECTION, *PIO_CONNECTION;

typedef struct __LW_IO_THREAD_STATE
{
    PIO_CREDS pCreds;
} IO_THREAD_STATE, *PIO_THREAD_STATE;

typedef struct IO_PATH_CREDS
{
    UNICODE_STRING PathPrefix;
    PIO_CREDS pCreds;
    LW_LIST_LINKS link;
} IO_PATH_CREDS, *PIO_PATH_CREDS;

typedef VOID
(*IO_CLIENT_ASYNC_COMPLETE_FUNCTION) (
    struct _IO_ASYNC_CANCEL_CONTEXT* pContext,
    NTSTATUS status
    );
    
typedef struct _IO_ASYNC_CANCEL_CONTEXT
{
    LONG volatile lRefcount;
    LWMsgCall* pCall;
    LWMsgParams in;
    LWMsgParams out;
    LWMsgTag responseType;
    IO_CLIENT_ASYNC_COMPLETE_FUNCTION pfnComplete;
    PIO_ASYNC_CONTROL_BLOCK pControl;
} IO_CLIENT_ASYNC_CONTEXT, *PIO_CLIENT_ASYNC_CONTEXT;

#endif /* __STRUCTS_H__ */
