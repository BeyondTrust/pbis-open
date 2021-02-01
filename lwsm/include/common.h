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

#define CONTAINER_PROCESS_NAME "lw-container"
#define CONTROL_LOCK CACHEDIR "/.lwsmd-lock"

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

#define STRUCT_FROM_MEMBER(_ptr_, _type_, _field_)                      \
    ((_type_ *) ((unsigned char*) (_ptr_) - offsetof(_type_, _field_)))

#define SM_ENDPOINT (CACHEDIR "/.lwsm")

#define SC_ENDPOINT (CACHEDIR "/.lwsc")

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
    SM_IPC_RESET_LOG_DEFAULTS_REQ,
    SM_IPC_RESET_LOG_DEFAULTS_RES,
    SM_IPC_SET_LOG_INFO_REQ,
    SM_IPC_SET_LOG_INFO_RES,
    SM_IPC_SET_LOG_LEVEL_REQ,
    SM_IPC_SET_LOG_LEVEL_RES,
    SM_IPC_GET_LOG_STATE_REQ,
    SM_IPC_GET_LOG_STATE_RES,
    SM_IPC_ENUM_FACILITIES_REQ,
    SM_IPC_ENUM_FACILITIES_RES,
    SM_IPC_REFRESH_REQ,
    SM_IPC_REFRESH_RES,
    SM_IPC_SHUTDOWN_REQ,
    SM_IPC_SHUTDOWN_RES,
    SM_IPC_SET_GLOBAL_REQ,
    SM_IPC_SET_GLOBAL_RES,
    SM_IPC_GET_GLOBAL_REQ,
    SM_IPC_GET_GLOBAL_RES
} SM_IPC_TAG;

typedef struct _SM_IPC_WAIT_STATE_CHANGE_REQ
{
    LWMsgHandle* hHandle;
    LW_SERVICE_STATE state;
} SM_IPC_WAIT_STATE_CHANGE_REQ, *PSM_IPC_WAIT_STATE_CHANGE_REQ;

typedef struct _SM_RESET_LOG_DEFAULTS_REQ
{
    LWMsgHandle* hHandle;
} SM_RESET_LOG_DEFAULTS_REQ, *PSM_RESET_LOG_DEFAULTS_REQ;

typedef struct _SM_SET_LOG_INFO_REQ
{
    LWMsgHandle* hHandle;
    PSTR pFacility;
    LW_SM_LOGGER_TYPE type;
    PSTR pszTarget;
    LW_BOOL PersistFlag;
} SM_SET_LOG_INFO_REQ, *PSM_SET_LOG_INFO_REQ;

typedef struct _SM_IPC_SET_LOG_LEVEL_REQ
{
    LWMsgHandle* hHandle;
    PSTR pFacility;
    LW_SM_LOG_LEVEL Level;
    LW_BOOL PersistFlag;
} SM_SET_LOG_LEVEL_REQ, *PSM_SET_LOG_LEVEL_REQ;

typedef struct _SM_IPC_GET_LOG_STATE_REQ
{
    LWMsgHandle* hHandle;
    PSTR pFacility;
} SM_GET_LOG_STATE_REQ, *PSM_GET_LOG_STATE_REQ;

typedef struct _SM_IPC_GET_LOG_STATE_RES
{
    LW_SM_LOGGER_TYPE type;
    PSTR pszTarget;
    LW_SM_LOG_LEVEL Level;
} SM_GET_LOG_STATE_RES, *PSM_GET_LOG_STATE_RES;

typedef enum _SM_GLOBAL_TYPE
{
    SM_GLOBAL_TYPE_BOOLEAN
} SM_GLOBAL_TYPE, *PSM_GLOBAL_TYPE;

typedef union _SM_GLOBAL_UNION
{
    BOOLEAN Boolean;
} SM_GLOBAL_UNION, *PSM_GLOBAL_UNION;

typedef struct _SM_GLOBAL_VALUE
{
    SM_GLOBAL_TYPE Type;
    SM_GLOBAL_UNION Value;
} SM_GLOBAL_VALUE, *PSM_GLOBAL_VALUE;

typedef struct _SM_SET_GLOBAL_REQ
{
    LW_SM_GLOBAL_SETTING Setting;
    SM_GLOBAL_VALUE Value;
} SM_SET_GLOBAL_REQ, *PSM_SET_GLOBAL_REQ;

typedef struct _SM_GET_GLOBAL_REQ
{
    LW_SM_GLOBAL_SETTING Setting;
} SM_GET_GLOBAL_REQ, *PSM_GET_GLOBAL_REQ;

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
