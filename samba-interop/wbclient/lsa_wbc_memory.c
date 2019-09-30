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
 *        lsa_wbc_memory.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MEM_MAGIC  0x28736512
struct _wbc_mem_header {
    uint32_t magic;
    mem_destructor_t free_fn;
};

static size_t MEM_HDR_SIZE = sizeof(struct _wbc_mem_header);

void *_wbc_malloc(size_t size, mem_destructor_t destructor)
{
    void *p = NULL;
    struct _wbc_mem_header *p_hdr;

    if (size == 0)
        return NULL;

    if ((p = malloc(size+MEM_HDR_SIZE)) == NULL) {
        return NULL;
    }

    p_hdr = (struct _wbc_mem_header*)p;

    p_hdr->magic = MEM_MAGIC;
    p_hdr->free_fn = destructor;

    return (p+MEM_HDR_SIZE);
}

void *_wbc_realloc(void *p, size_t new_size)
{
    struct _wbc_mem_header *chunk;
    void *new_p = NULL;

    chunk = (struct _wbc_mem_header*)(p - MEM_HDR_SIZE);
           assert(chunk->magic == MEM_MAGIC);

    if (new_size == 0) {
        _WBC_FREE(p);
        return NULL;
    }

    if ((new_p = realloc(chunk, new_size+MEM_HDR_SIZE)) == NULL) {
        return NULL;
    }

    return (new_p+MEM_HDR_SIZE);
}

void *_wbc_malloc_zero(size_t size, mem_destructor_t destructor)
{
    void *p;

    if (size == 0)
        return NULL;

    if ((p = _wbc_malloc(size, destructor)) == NULL){
        return NULL;
    }

    memset(p, 0x0, size);

    return p;
}


void _wbc_free(void *p)
{
    struct _wbc_mem_header *chunk;

    if (!p)
        return;

    chunk = (struct _wbc_mem_header*)(p - MEM_HDR_SIZE);

           assert(chunk->magic == MEM_MAGIC);

    if (chunk->free_fn)
        chunk->free_fn(p);

    free(chunk);
}

char* _wbc_strdup(const char *str)
{
    size_t len = 0;
    char *p = NULL;

    if (!str)
        return NULL;

    len = strlen(str);
    if ((p = _wbc_malloc(len+1, NULL)) == NULL) {
        return NULL;
    }

    /* Copy and NULL terminate */

    strncpy(p, str, len);
    p[len] = '\0';

    return p;
}

int _wbc_free_string_array(void *p)
{
    char **members = (char**)p;
    int i = 0;

    if (!p)
        return 0;

    for (i=0; members[i]; i++) {
        _WBC_FREE(members[i]);
    }

    return 0;
}
