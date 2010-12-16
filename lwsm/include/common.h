/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        common.h
 *
 * Abstract:
 *
 *        Common private header file
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWSM_COMMON_H__
#define __LWSM_COMMON_H__

#include "config.h"
#include <lwsm/lwsm.h>

#define BAIL_ON_ERROR(_e_)  \
    do                      \
    {                       \
        if ((_e_))          \
        {                   \
            goto error;     \
        }                   \
    } while (0)

#define MAP_LWMSG_STATUS(_e_) (LwMapLwmsgStatusToLwError((_e_)))

#define LOCK(b, l)                              \
    do                                          \
    {                                           \
        if (!(b))                               \
        {                                       \
            pthread_mutex_lock(l);              \
            b = TRUE;                           \
        }                                       \
    } while (0)

#define UNLOCK(b, l)                            \
    do                                          \
    {                                           \
        if (b)                                  \
        {                                       \
            pthread_mutex_unlock(l);            \
            b = FALSE;                          \
        }                                       \
    } while (0)

#if defined(sun) || defined(_AIX)
#  define ONCE_INIT {PTHREAD_ONCE_INIT}
#else
#  define ONCE_INIT PTHREAD_ONCE_INIT
#endif

#define STRUCT_FROM_MEMBER(_ptr_, _type_, _field_)                      \
    ((_type_ *) ((unsigned char*) (_ptr_) - offsetof(_type_, _field_)))

#define SM_ENDPOINT (CACHEDIR "/.lwsm")

typedef enum _SM_IPC_TAG
{
    SM_IPC_ERROR,
    SM_IPC_ACQUIRE_SERVICE_HANDLE_REQ,
    SM_IPC_ACQUIRE_SERVICE_HANDLE_RES,
    SM_IPC_RELEASE_SERVICE_HANDLE_REQ,
    SM_IPC_RELEASE_SERVICE_HANDLE_RES,
    SM_IPC_ENUMERATE_SERVICES_REQ,
    SM_IPC_ENUMERATE_SERVICES_RES,
    SM_IPC_ADD_SERVICE_REQ,
    SM_IPC_ADD_SERVICE_RES,
    SM_IPC_REMOVE_SERVICE_REQ,
    SM_IPC_REMOVE_SERVICE_RES,
    SM_IPC_START_SERVICE_REQ,
    SM_IPC_START_SERVICE_RES,
    SM_IPC_STOP_SERVICE_REQ,
    SM_IPC_STOP_SERVICE_RES,
    SM_IPC_REFRESH_SERVICE_REQ,
    SM_IPC_REFRESH_SERVICE_RES,
    SM_IPC_QUERY_SERVICE_STATUS_REQ,
    SM_IPC_QUERY_SERVICE_STATUS_RES,
    SM_IPC_QUERY_SERVICE_INFO_REQ,
    SM_IPC_QUERY_SERVICE_INFO_RES,
    SM_IPC_QUERY_SERVICE_PROCESS_REQ,
    SM_IPC_QUERY_SERVICE_PROCESS_RES,
    SM_IPC_WAIT_SERVICE_REQ,
    SM_IPC_WAIT_SERVICE_RES,
    SM_IPC_SET_LOG_INFO_REQ,
    SM_IPC_SET_LOG_INFO_RES,
    SM_IPC_GET_LOG_INFO_REQ,
    SM_IPC_GET_LOG_INFO_RES,
    SM_IPC_SET_LOG_LEVEL_REQ,
    SM_IPC_SET_LOG_LEVEL_RES,
    SM_IPC_GET_LOG_LEVEL_REQ,
    SM_IPC_GET_LOG_LEVEL_RES,
    SM_IPC_REFRESH_REQ,
    SM_IPC_REFRESH_RES,
    SM_IPC_SHUTDOWN_REQ,
    SM_IPC_SHUTDOWN_RES
} SM_IPC_TAG;

typedef struct _SM_IPC_WAIT_STATE_CHANGE_REQ
{
    LW_SERVICE_HANDLE hHandle;
    LW_SERVICE_STATE state;
} SM_IPC_WAIT_STATE_CHANGE_REQ, *PSM_IPC_WAIT_STATE_CHANGE_REQ;

typedef struct _SM_IPC_LOG_INFO
{
    LW_SM_LOGGER_TYPE type;
    PSTR pszTarget;
} SM_IPC_LOG_INFO, *PSM_IPC_LOG_INFO;

typedef struct _SM_LINK
{
    struct _SM_LINK *pNext, *pPrev;
} SM_LINK, *PSM_LINK;

LWMsgProtocolSpec*
LwSmIpcGetProtocolSpec(
    VOID
    );

PWSTR
LwSmWc16sLastChar(
    PWSTR pwszHaystack,
    WCHAR needle
    );

VOID
LwSmFreeStringList(
    PWSTR* ppwszStrings
    );

size_t
LwSmStringListLength(
    PWSTR* ppwszStrings
    );

size_t
LwSmStringListLengthA(
    PSTR* ppwszStrings
    );

DWORD
LwSmCopyStringList(
    PWSTR* ppwszStrings,
    PWSTR** pppwszCopy
    );

BOOLEAN
LwSmStringListContains(
    PWSTR* ppwszStrings,
    PWSTR pwszNeedle
    );

DWORD
LwSmStringListAppend(
    PWSTR** pppwszStrings,
    PWSTR pwszElement
    );

DWORD
LwSmCopyString(
    PCWSTR pwszString,
    PWSTR* ppwszCopy
    );

DWORD
LwSmCopyServiceInfo(
    PLW_SERVICE_INFO pInfo,
    PLW_SERVICE_INFO* ppCopy
    );

VOID
LwSmCommonFreeServiceInfo(
    PLW_SERVICE_INFO pInfo
    );

static inline
VOID
LwSmLinkInit(
    PSM_LINK pLink
    )
{
    pLink->pNext = pLink->pPrev = pLink;
}

static inline
void
LwSmLinkInsertAfter(
    PSM_LINK pAnchor,
    PSM_LINK pElement
    )
{
    pElement->pNext = pAnchor->pNext;
    pElement->pPrev = pAnchor;
    
    pAnchor->pNext->pPrev = pElement;
    pAnchor->pNext = pElement;
}

static inline
void
LwSmLinkInsertBefore(
    PSM_LINK pAnchor,
    PSM_LINK pElement
    )
{
    pElement->pNext = pAnchor;
    pElement->pPrev = pAnchor->pPrev;

    pAnchor->pPrev->pNext = pElement;
    pAnchor->pPrev = pElement;
}

static inline
void
LwSmLinkRemove(
    PSM_LINK pElement
    )
{
    pElement->pPrev->pNext = pElement->pNext;
    pElement->pNext->pPrev = pElement->pPrev;
    LwSmLinkInit(pElement);
}

#define SM_LINK_ITERATE(anchor, link)           \
    ((link) == NULL ? (anchor)->pNext :         \
     ((link)->pNext == (anchor) ? NULL :        \
      (link)->pNext))

static inline
PSM_LINK
LwSmLinkBegin(
    PSM_LINK pAnchor
    )
{
    return pAnchor->pNext;
}

static inline
BOOL
LwSmLinkValid(
    PSM_LINK pAnchor,
    PSM_LINK pLink
    )
{
    return pLink != pAnchor;
}

static inline
PSM_LINK
LwSmLinkNext(
    PSM_LINK pLink
    )
{
    return pLink->pNext;
}

#endif
