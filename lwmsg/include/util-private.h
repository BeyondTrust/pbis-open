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
 *        util-private.h
 *
 * Abstract:
 *
 *        Utility functions (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_UTIL_PRIVATE_H__
#define __LWMSG_UTIL_PRIVATE_H__

#include "status-private.h"
#include <lwmsg/context.h>
#include <lwmsg/buffer.h>
#include <lwmsg/time.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct msghdr;

#define LWMSG_ASSERT_SUCCESS(_x_)                                       \
    do                                                                  \
    {                                                                   \
        LWMsgStatus __status__ = (_x_);                                 \
        if (__status__)                                                 \
        {                                                               \
            fprintf(stderr,                                             \
                    "%s:%i: Assertion failed with status %i: %s\n",     \
                    __FILE__, __LINE__, __status__, #_x_);              \
            abort();                                                    \
        }                                                               \
    } while (0)

#define LWMSG_ASSERT(_x_)                                               \
    do                                                                  \
    {                                                                   \
        if (!(_x_))                                                     \
        {                                                               \
            fprintf(stderr,                                             \
                    "%s:%i: Assertion failed: %s\n",                    \
                    __FILE__, __LINE__, #_x_);                          \
            abort();                                                    \
        }                                                               \
    } while (0)

#define LWMSG_ASSERT_NOT_REACHED()         \
    do                                     \
    {                                      \
        fprintf(stderr,                    \
            "%s:%i: Should not be here\n", \
            __FILE__, __LINE__);           \
            abort();                       \
    } while (0)

#define BAIL_ON_ERROR(_x_)                      \
    do                                          \
    {                                           \
        if ((_x_))                              \
        {                                       \
            goto error;                         \
        }                                       \
    } while (0)

#define RAISE_ERROR(_context_, _status_, ...) \
    BAIL_ON_ERROR(status = RAISE((_context_), (_status_), __VA_ARGS__))

#define LWMSG_ALLOC_ARRAY(_count_, _obj_) \
    ((*(_obj_) = calloc((_count_), sizeof **(_obj_))) ? LWMSG_STATUS_SUCCESS : LWMSG_STATUS_MEMORY)
    
#define LWMSG_ALLOC(_obj_) (LWMSG_ALLOC_ARRAY(1, _obj_))

#define LWMSG_CONTEXT_ALLOC_ARRAY(_ctxt_, _count_, _obj_)               \
    (lwmsg_context_alloc((_ctxt_), (_count_) * sizeof **(_obj_), (void**) (void*) (_obj_)))
    
#define LWMSG_CONTEXT_ALLOC(_ctxt_, _obj_) (LWMSG_CONTEXT_ALLOC_ARRAY((_ctxt_), 1, (_obj_)))

#define LWMSG_POINTER_AS_ULONG(ptr) ((unsigned long) (size_t) (ptr))

char* lwmsg_formatv(const char* fmt, va_list ap);
char* lwmsg_format(const char* fmt, ...);

ssize_t
lwmsg_convert_string_alloc(
    void* input,
    size_t input_len,
    void** output,
    const char* input_type,
    const char* output_type
    );

ssize_t
lwmsg_convert_string_buffer(
    void* input,
    size_t input_len,
    void* output,
    size_t output_len,
    const char* input_type,
    const char* output_type
    );

LWMsgStatus
lwmsg_add_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    );

LWMsgStatus
lwmsg_multiply_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    );

typedef struct LWMsgRing
{
    struct LWMsgRing* prev;
    struct LWMsgRing* next;
} LWMsgRing;

static inline
void
lwmsg_ring_init(
    LWMsgRing* ring
    )
{
    ring->prev = ring->next = ring;
}

static inline
void
lwmsg_ring_sanity(
    LWMsgRing* ring
    )
{
    LWMSG_ASSERT(ring->prev->next == ring && ring->next->prev == ring);
}

static inline
void
lwmsg_ring_insert_after(
    LWMsgRing* anchor,
    LWMsgRing* element
    )
{
    lwmsg_ring_sanity(anchor);
    lwmsg_ring_sanity(element);
    LWMSG_ASSERT(element->prev == element->next && element->prev == element);

    element->next = anchor->next;
    element->prev = anchor;
    
    anchor->next->prev = element;
    anchor->next = element;
}

static inline
void
lwmsg_ring_insert_before(
    LWMsgRing* anchor,
    LWMsgRing* element
    )
{
    lwmsg_ring_sanity(anchor);
    lwmsg_ring_sanity(element);
    LWMSG_ASSERT(element->prev == element->next && element->prev == element);

    element->next = anchor;
    element->prev = anchor->prev;

    anchor->prev->next = element;
    anchor->prev = element;
}

static inline
void
lwmsg_ring_remove(
    LWMsgRing* element
    )
{
    lwmsg_ring_sanity(element);
    element->prev->next = element->next;
    element->next->prev = element->prev;
    lwmsg_ring_init(element);
}

static inline
void
lwmsg_ring_enqueue(
    LWMsgRing* anchor,
    LWMsgRing* element
    )
{
    lwmsg_ring_insert_before(anchor, element);
}

static inline
void
lwmsg_ring_dequeue(
    LWMsgRing* anchor,
    LWMsgRing** element
    )
{
    *element = anchor->next;
    lwmsg_ring_remove(*element);
}

static inline
void
lwmsg_ring_move(
    LWMsgRing* from,
    LWMsgRing* to
    )
{
    LWMsgRing* from_first = from->next;
    LWMsgRing* from_last = from->prev;
    LWMsgRing* to_last = to->prev;

    lwmsg_ring_sanity(from);
    lwmsg_ring_sanity(to);

    if (from->next != from)
    {
        /* Link from to_last and from_first */
        to_last->next = from_first;
        from_first->prev = to_last;
        
        /* Link from_last into to */
        from_last->next = to;
        to->prev = from_last;
        
        from->next = from->prev = from;
    }
}

static inline
size_t
lwmsg_ring_count(
    LWMsgRing* ring
    )
{
    LWMsgRing* iter = NULL;
    size_t count = 0;

    lwmsg_ring_sanity(ring);

    for (iter = ring->next; iter != ring; iter = iter->next, count++);

    return count;
}

static inline
LWMsgBool
lwmsg_ring_is_empty(
    LWMsgRing* ring
    )
{
    lwmsg_ring_sanity(ring);

    return ring->next == ring;
}

LWMsgStatus
lwmsg_strerror(
    int err,
    char** message
    );

LWMsgStatus
lwmsg_set_close_on_exec(
    int fd
    );

LWMsgStatus
lwmsg_set_block_sigpipe(
    int fd
    );

#define LWMSG_OBJECT_FROM_MEMBER(_ptr_, _type_, _field_) \
    ((_type_ *) ((unsigned char*) (_ptr_) - offsetof(_type_, _field_)))

#define _LWMSG_SWAP16(val)                   \
    ((((uint16_t)(val) & 0xFF00) >> 8) |     \
     (((uint16_t)(val) & 0x00FF) << 8))

#define _LWMSG_SWAP32(val)                         \
        ((((uint32_t)(val) & 0xFF000000L) >> 24) | \
         (((uint32_t)(val) & 0x00FF0000L) >>  8) | \
         (((uint32_t)(val) & 0x0000FF00L) <<  8) | \
         (((uint32_t)(val) & 0x000000FFL) << 24))

#define _LWMSG_SWAP64(val)                                              \
    (((uint64_t)(_SMB_ENDIAN_SWAP32(((uint64_t)(val) & 0xFFFFFFFF00000000LL) >> 32))) | \
   (((uint64_t)_SMB_ENDIAN_SWAP32(((uint64_t)(val) & 0x00000000FFFFFFFFLL))) << 32))

#define LWMSG_SWAP16(val, src_order, dst_order) \
    ((src_order) == (dst_order) ?               \
     (val) :                                    \
     _LWMSG_SWAP16((val)))

#define LWMSG_SWAP32(val, src_order, dst_order) \
    ((src_order) == (dst_order) ?               \
     (val) :                                    \
     _LWMSG_SWAP32((val)))

#define LWMSG_SWAP64(val, src_order, dst_order) \
    ((src_order) == (dst_order) ?               \
     (val) :                                    \
     _LWMSG_SWAP64((val)))

#define LWMSG_LOCK(b, l)                        \
    do                                          \
    {                                           \
        if (!(b))                               \
        {                                       \
            pthread_mutex_lock((l));            \
            (b) = LWMSG_TRUE;                   \
        }                                       \
    } while(0)

#define LWMSG_UNLOCK(b, l)                      \
    do                                          \
    {                                           \
        if ((b))                                \
        {                                       \
            pthread_mutex_unlock((l));          \
            (b) = LWMSG_FALSE;                  \
        }                                       \
    } while(0)

static inline
const char*
lwmsg_string_without_prefix(
    const char* str,
    const char* prefix
    )
{
    if (!strncmp(str, prefix, strlen(prefix)))
    {
        return str + strlen(prefix);
    }
    else
    {
        return str;
    }
}

ssize_t
lwmsg_recvmsg_timeout(
    int sock,
    struct msghdr* msg,
    int flags,
    LWMsgTime* time
    );

ssize_t
lwmsg_sendmsg_timeout(
    int sock,
    const struct msghdr* msg,
    int flags,
    LWMsgTime* time
    );

typedef void*
(*LWMsgHashGetKeyFunc)(
    const void* entry
    );

typedef size_t
(*LWMsgHashDigestFunc)(
    const void* key
    );

typedef LWMsgBool (*LWMsgHashEqualFunc)(
    const void* key1,
    const void* key2
    );

typedef struct LWMsgHashTable
{
    size_t capacity;
    size_t count;
    LWMsgRing* buckets;
    LWMsgHashGetKeyFunc get_key;
    LWMsgHashDigestFunc digest;
    LWMsgHashEqualFunc equal;
    size_t ring_offset;
} LWMsgHashTable;

typedef struct LWMsgHashIter
{
    LWMsgRing* bucket;
    LWMsgRing* ring;
} LWMsgHashIter;

LWMsgStatus
lwmsg_hash_init(
    LWMsgHashTable* table,
    size_t capacity,
    LWMsgHashGetKeyFunc get_key,
    LWMsgHashDigestFunc digest,
    LWMsgHashEqualFunc equal,
    size_t ring_offset
    );

size_t
lwmsg_hash_get_count(
    LWMsgHashTable* table
    );

void
lwmsg_hash_insert_entry(
    LWMsgHashTable* table,
    void* entry
    );

void*
lwmsg_hash_find_key(
    LWMsgHashTable* table,
    const void* key
    );

LWMsgStatus
lwmsg_hash_remove_key(
    LWMsgHashTable* table,
    const void* key
    );

LWMsgStatus
lwmsg_hash_remove_entry(
    LWMsgHashTable* table,
    void* entry
    );

void
lwmsg_hash_iter_begin(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    );

void*
lwmsg_hash_iter_next(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    );

void
lwmsg_hash_iter_end(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    );

void
lwmsg_hash_destroy(
    LWMsgHashTable* table
    );

static inline
LWMsgStatus
lwmsg_strdup(
    const struct LWMsgContext* context,
    const char* src,
    char** dst
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!src)
    {
        *dst = NULL;
        goto error;
    }

    BAIL_ON_ERROR(status = lwmsg_context_alloc(
                      context,
                      strlen(src) + 1,
                      (void**) (void*) dst));

    strcpy(*dst, src);

error:

    return status;
}

#endif
