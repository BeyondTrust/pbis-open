/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "includes.h"

static
LW_NTSTATUS
LwSrvInfoGetMarshalContext(
    LWMsgContext** ppMarshalContext
    );

static
void
LwSrvInfoInitMarshalContext(
    void
    );

#ifndef BROKEN_ONCE_INIT
#if defined(sun) || defined(_AIX)
#define BROKEN_ONCE_INIT 1
#else
#define BROKEN_ONCE_INIT 0
#endif
#endif

#if BROKEN_ONCE_INIT
#  define ONCE_INIT {PTHREAD_ONCE_INIT}
#else
#  define ONCE_INIT PTHREAD_ONCE_INIT
#endif

static pthread_once_t gShareMarshalContextControl = ONCE_INIT;
static LWMsgContext*  gpMarshalContext            = NULL;
static LWMsgStatus    gMarshalContextStatus       = LWMSG_STATUS_SUCCESS;

LW_NTSTATUS
LwSrvInfoAcquireDataContext(
    LWMsgDataContext** ppDataContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LWMsgContext* pContext = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoGetMarshalContext(&pContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_NT_STATUS(Status);

    *ppDataContext = pDataContext;

cleanup:

    return Status;

error:

    *ppDataContext = NULL;

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    goto cleanup;
}

static
LW_NTSTATUS
LwSrvInfoGetMarshalContext(
    LWMsgContext** ppMarshalContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int err = 0;

    err = pthread_once(&gShareMarshalContextControl, LwSrvInfoInitMarshalContext);
    if (err)
    {
        Status = LwErrnoToNtStatus(err);
        BAIL_ON_NT_STATUS(Status);
    }

    Status = MAP_LWMSG_STATUS(gMarshalContextStatus);
    BAIL_ON_NT_STATUS(Status);

    *ppMarshalContext = gpMarshalContext;

cleanup:

    return Status;

error:

    *ppMarshalContext = NULL;

    goto cleanup;
}

static
void
LwSrvInfoInitMarshalContext(
    void
    )
{
    gMarshalContextStatus = lwmsg_context_new(NULL, &gpMarshalContext);
}

void
LwSrvInfoReleaseDataContext(
    LWMsgDataContext* pDataContext
    )
{
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
}
