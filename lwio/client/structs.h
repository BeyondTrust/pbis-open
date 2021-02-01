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
