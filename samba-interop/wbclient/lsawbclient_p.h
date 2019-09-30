/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lsawbclirent_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#ifndef _LSAWBCLIENT_P_H
#define _LSAWBCLIENT_P_H

#include "wbclient.h"
#include <lsa/lsa.h>
#include "util_str.h"
#include <uuid/uuid.h>

#define LSA_WBC_LIBRARY_MAJOR_VERSION          0
#define LSA_WBC_LIBRARY_MINOR_VERSION          1
#define LSA_WBC_LIBRARY_VENDOR_STRING          "Likewise Security Authority"

#define LSA_WBC_INTERFACE_VERSION              1001
#define LSA_WBC_WINBIND_VERSION                "5.0.0"

/* Common macros */

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#define _WBC_FREE(x)                \
    do {                    \
        _wbc_free((void *)x);            \
        (x) = NULL;            \
    } while (0);

#define _WBC_FREE_CONST_DISCARD(x)        \
    do {                    \
        _wbc_free((void*)x);        \
        (x) = NULL;            \
    } while (0);

#define BAIL_ON_WBC_ERR(x)            \
    do {                    \
        if (x != WBC_ERR_SUCCESS)    \
            goto cleanup;        \
    } while(0);

#define BAIL_ON_LSA_ERR(x)            \
    do {                    \
        if (x != LW_ERROR_SUCCESS)    \
            goto cleanup;        \
    } while(0);

#define BAIL_ON_NETLOGON_ERR(x)            \
    do {                    \
        if (x != ERROR_SUCCESS)    \
            goto cleanup;            \
    } while(0);


#define BAIL_ON_NULL_PTR(x, y)                \
    do {                        \
        if (x == NULL) {            \
            y = LW_ERROR_OUT_OF_MEMORY;    \
            goto cleanup;            \
        }                    \
    } while(0);

#define BAIL_ON_NULL_PTR_PARAM(x, y)                \
    do {                            \
        if (x == NULL) {                \
            y = LW_ERROR_INVALID_PARAMETER;    \
            goto cleanup;                \
        }                        \
    } while(0);

#define LW_ERROR_IS_OK(x)         ((x) == LW_ERROR_SUCCESS)


/* local private functions */

DWORD
wbcFillErrorInfo(
    DWORD dwError,
    struct wbcAuthErrorInfo **ppWbcError
    );

DWORD
map_wbc_to_lsa_error(
    wbcErr
    );

wbcErr
map_error_to_wbc_status(
    DWORD
    );

wbcErr
wbcSidAppendRid(
    struct wbcDomainSid *sid,
    DWORD rid
    );

wbcErr
wbcSidCopy(
    struct wbcDomainSid *dst,
    struct wbcDomainSid *src
    );

DWORD
wbcFindSecurityObjectBySid(
    IN const struct wbcDomainSid *sid,
    PLSA_SECURITY_OBJECT* ppResult
    );

void
wbcUuidToWbcGuid(
    IN const uuid_t uuid,
    OUT struct wbcGuid *pGuid
    );

/* memory management */

typedef int (*mem_destructor_t)(void*);

void *_wbc_malloc(size_t size, mem_destructor_t destructor);
void *_wbc_realloc(void *p, size_t new_size);
void *_wbc_malloc_zero(size_t size, mem_destructor_t destructor);
void  _wbc_free(void *p);
char* _wbc_strdup(const char *str);
int   _wbc_free_string_array(void *p);

#endif /* _LSAWBCLIENT_P_H */
