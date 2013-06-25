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
 *        util.c
 *
 * Abstract:
 *
 *        Utility functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "util-private.h"
#include "xnet-private.h"
#include "buffer-private.h"

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#ifndef SOLARIS_11
#if HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif
#endif
#include <iconv.h>
#include <errno.h>
#ifdef HAVE_PTHREAD_SIGMASK_IN_LIBC
#include <pthread.h>
#endif
#include <signal.h>
#include <poll.h>

char* lwmsg_formatv(const char* fmt, va_list ap)
{
    int len;
    va_list my_ap;
    char* str, *str_new;

    /* Some versions of vsnprintf won't accept
       a null or zero-length buffer */
    str = malloc(1);

    if (!str)
    {
        return NULL;
    }
    
    va_copy(my_ap, ap);
    
    len = vsnprintf(str, 1, fmt, my_ap);
    
    /* Some versions of vsnprintf return -1 when
       the buffer was too small rather than the
       number of characters that would be written,
       so we have loop in search of a large enough
       buffer */
    if (len == -1)
    {
        int capacity = 16;
        do
        {
            capacity *= 2;
            va_copy(my_ap, ap);
            str_new = realloc(str, capacity);
            if (!str_new)
            {
                free(str);
                return NULL;
            }
            str = str_new;
        } while ((len = vsnprintf(str, capacity, fmt, my_ap)) == -1 || capacity <= len);
        str[len] = '\0';
        
        return str;
    }
    else
    {
        va_copy(my_ap, ap);
        
        str_new = realloc(str, len+1);
        
        if (!str_new)
        {
            free(str);
            va_end(my_ap);
            return NULL;
        }

        str = str_new;

        if (vsnprintf(str, len+1, fmt, my_ap) < len)
        {
            free(str);
            va_end(my_ap);
            return NULL;
        }
        else
        {
            va_end(my_ap);
            return str;
        }
    }
}

char* lwmsg_format(const char* fmt, ...)
{
    va_list ap;
    char* str;
    va_start(ap, fmt);
    str = lwmsg_formatv(fmt, ap);
    va_end(ap);
    return str;
}

ssize_t
lwmsg_convert_string_alloc(
    void* input,
    size_t input_len,
    void** output,
    const char* input_type,
    const char* output_type
    )
{
    ssize_t cblen = 0;
    char *buffer = NULL;

    if (input == NULL)
    {
        goto error;
    }

    cblen = lwmsg_convert_string_buffer(input, input_len, NULL, 0, input_type, output_type);

    if (cblen < 0)
    {
        goto error;
    }

    buffer = malloc(cblen);
    if (buffer == NULL)
    {
        goto error;
    }

    if (lwmsg_convert_string_buffer(input, input_len, buffer, cblen, input_type, output_type) != cblen)
    {
        goto error;
    }

    *output = buffer;

done:

    return cblen;

error:

    cblen = -1;
    if (buffer)
    {
        free(buffer);
    }
    goto done;
}

ssize_t
lwmsg_convert_string_buffer(
    void* input,
    size_t input_len,
    void* output,
    size_t output_len,
    const char* input_type,
    const char* output_type
    )
{
    iconv_t handle = iconv_open(output_type, input_type);
    char *inbuf = (char *)input;
    char *outbuf = (char *)output;
    size_t cbin = input_len;
    size_t cbout = output_len;
    size_t converted;

    if(outbuf == NULL)
    {
        char buffer[100];
        size_t cblen = 0;
        while(cbin > 0)
        {
            outbuf = buffer;
            cbout = sizeof(buffer);
            converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);
            if(converted == (size_t)-1)
            {
                if(errno != E2BIG)
                {
                    cblen = -1;
                    break;
                }
            }
            cblen += sizeof(buffer) - cbout;
        }
        iconv_close(handle);
        return cblen;
    }
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);
    
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return -1;
    else
        return output_len - cbout;
}

LWMsgStatus
lwmsg_add_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t sum = operand_a + operand_b;

    if (sum < operand_a)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }
    
    *result = sum;

error:

    return status;
}

#define SIZE_T_BITS (sizeof(size_t) * 8)
#define SIZE_T_HALF_BITS (SIZE_T_BITS / 2)
#define SIZE_T_LOWER_MASK (((size_t) (ssize_t) -1) >> SIZE_T_HALF_BITS)
#define SIZE_T_UPPER_MASK (~SIZE_T_LOWER_MASK)

LWMsgStatus
lwmsg_multiply_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    )
{
    /* Multiplication with overflow checking:
       
       Operands: x and y
       
       Let        r = 2^((8 * sizeof size_t) / 2)
       Let        x = ra + b
       Let        y = rc + d
       Therefore  xy = (ra + b) * (rc + d)
       Therefore  xy = (r^2)ac + rbc + rad + bd
       Assume     y <= x
       Therefore  c <= a
       If         c != 0
       Then       (r^2)ac >= r^2
       Therefore  xy >= r^2
       Therefore  raise overflow
       Otherwise  c = 0
       Therefore  xy = rad + bd
       If         ad >= r
       Then       rad >= r^2
       Therefore  xy >= r^2
       Therefore  raise overflow
       Perform    rad + bd with add overflow check
    */

    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t x;
    size_t y;
    size_t a;
    size_t b;
    size_t c;
    size_t d;
    size_t ad;
    size_t bd;
    size_t rad;
    size_t product;

    /* Ensure y is the smaller operand */
    if (operand_b <= operand_a)
    {
        x = operand_a;
        y = operand_b;
    }
    else
    {
        x = operand_b;
        y = operand_a;
    }

    /* Calculate decomposition of operands */
    c = y >> SIZE_T_HALF_BITS;
    if (c != 0)
    {
        /* If c is not 0, the first term will overflow */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    a = x >> SIZE_T_HALF_BITS;
    b = x & SIZE_T_LOWER_MASK;
    d = y & SIZE_T_LOWER_MASK;

    ad = a * d;

    if ((ad & SIZE_T_UPPER_MASK) != 0)
    {
        /* ad >= r, so rad will overflow */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    rad = ad << SIZE_T_HALF_BITS;

    /* Never overflows */
    bd = b * d;
    
    product = rad + bd;
    
    if (product < rad)
    {
        /* Addition overflowed */
        BAIL_ON_ERROR(status = LWMSG_STATUS_OVERFLOW);
    }

    *result = product;
    
error:

    return status;
}

#ifdef HAVE_STRERROR_R
#ifdef STRERROR_R_CHAR_P
LWMsgStatus
lwmsg_strerror(
    int err,
    char** message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* my_message = NULL;
    char buffer[512];
    const char* text = NULL;
    
    text = strerror_r(err, buffer, sizeof(buffer));
    
    if (!text)
    {
        text = "UNKNOWN";
    }

    my_message = strdup(text);
    
    if (!my_message)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    *message = my_message;

done:

    return status;

error:

    if (my_message)
    {
        free(my_message);
    }

    goto done;
}
#else
LWMsgStatus
lwmsg_strerror(
    int err,
    char** message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* my_message = NULL;
    char* temp = NULL;
    size_t capacity = 128;

    my_message = malloc(capacity);

    if (!my_message)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }


    while (strerror_r(err, my_message, capacity) == -1 &&
           errno == ERANGE)
    {
        capacity *= 2;
        temp = realloc(my_message, capacity);
        if (!temp)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
        my_message = temp;
    }

    my_message[capacity-1] = '\0';
    *message = my_message;

done:

    return status;

error:

    if (my_message)
    {
        free(my_message);
    }

    goto done;
}
#endif
#else
LWMsgStatus
lwmsg_strerror(
    int err,
    char** message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* err_message = NULL;
    char* my_message = NULL;

    err_message = strerror(err);
    my_message = strdup(err_message);

    if (!my_message)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    *message = my_message;

done:

    return status;

error:

    if (my_message)
    {
        free(my_message);
    }

    goto done;
}
#endif

LWMsgStatus
lwmsg_set_close_on_exec(
    int fd
    )
{
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
    {
        return lwmsg_status_map_errno(errno);
    }
    else
    {
        return LWMSG_STATUS_SUCCESS;
    }
}

LWMsgStatus
lwmsg_set_block_sigpipe(
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
#ifdef SO_NOSIGPIPE
    int on = 1;
    
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

error:
#endif   
    return status;
}

static
int
lwmsg_begin_ignore_sigpipe(
    LWMsgBool* unblock,
    sigset_t* original
    )
{
    int ret = 0;
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE) && defined(HAVE_PTHREAD_SIGMASK_IN_LIBC)
    sigset_t sigpipemask;
    sigset_t pending;

    sigpending(&pending);

    /* If SIGPIPE was not already pending against the thread,
       we need to modify the thread signal mask to block it
       and then unblock it again once we are done */
    if (!sigismember(&pending, SIGPIPE))
    {
        sigemptyset(&sigpipemask);
        sigaddset(&sigpipemask, SIGPIPE);

        if (pthread_sigmask(SIG_BLOCK, &sigpipemask, original) < 0)
        {
            ret = -1;
            goto error;
        }

        *unblock = LWMSG_TRUE;
    }
    else
    {
        *unblock = LWMSG_FALSE;
    }
    
error:
#endif
    return ret;
}

static
int
lwmsg_end_ignore_sigpipe(
    LWMsgBool* unblock,
    sigset_t* original
    )
{
    int ret = 0;
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE) && defined(HAVE_PTHREAD_SIGMASK_IN_LIBC)
    sigset_t sigpipemask;
    sigset_t pending;
    siginfo_t info;
    struct timespec timeout = {0, 0};

    if (*unblock)
    {
        /* We need to restore the original signal mask for this thread.
           Before doing so, we need to clear any pending SIGPIPE so it
           does not trigger a signal handler as soon as we restore the mask. */
        sigpending(&pending);

        if (sigismember(&pending, SIGPIPE))
        {
            sigemptyset(&sigpipemask);
            sigaddset(&sigpipemask, SIGPIPE);
            
            do
            {
                ret = sigtimedwait(&sigpipemask, &info, &timeout);
            } while (ret < 0 && errno == EINTR);

            ret = 0;
        }
        
        if (pthread_sigmask(SIG_SETMASK, original, NULL))
        {
            ret = -1;
            goto error;
        }
        
        *unblock = LWMSG_FALSE;
    }
        
error:
#endif
    return ret;
}

ssize_t
lwmsg_recvmsg_timeout(
    int sock,
    struct msghdr* msg,
    int flags,
    LWMsgTime* time
    )
{
    int timeout = -1;
    struct pollfd pfd = {0};
    int ret = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    if (time && lwmsg_time_is_positive(time))
    {
        timeout = time->seconds * 1000 + time->microseconds / 1000;

        pfd.fd = sock;
        pfd.events = POLLIN;

        ret = poll(&pfd, 1, timeout);
        if (ret < 0)
        {
            return ret;
        }
        else if (ret == 0)
        {
            errno = EAGAIN;
            return -1;
        }
    }

    return recvmsg(sock, msg, flags);
}

ssize_t
lwmsg_sendmsg_timeout(
    int sock,
    const struct msghdr* msg,
    int flags,
    LWMsgTime* time
    )
{
    int timeout = -1;
    struct pollfd pfd = {0};
    int ret = 0;
    sigset_t original;
    LWMsgBool unblock = LWMSG_FALSE;
    int err = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    if (time && lwmsg_time_is_positive(time))
    {
        timeout = time->seconds * 1000 + time->microseconds / 1000;

        pfd.fd = sock;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, timeout);
        if (ret < 0)
        {
            return ret;
        }
        else if (ret == 0)
        {
            errno = EAGAIN;
            return -1;
        }
    }

    if (lwmsg_begin_ignore_sigpipe(&unblock, &original) < 0)
    {
        return -1;
    }

    ret = sendmsg(sock, msg, flags);
    err = errno;

    if (lwmsg_end_ignore_sigpipe(&unblock, &original) < 0)
    {
        return -1;
    }

    errno = err;
    return ret;
}

static
LWMsgRing*
lwmsg_hash_ring_from_entry(
    LWMsgHashTable* table,
    void* entry
    )
{
    return (LWMsgRing*) ((unsigned char*) entry + table->ring_offset);
}

static
void*
lwmsg_hash_entry_from_ring(
    LWMsgHashTable* table,
    LWMsgRing* ring
    )
{
    return (void*) ((unsigned char*) ring - table->ring_offset);
}

LWMsgStatus
lwmsg_hash_init(
    LWMsgHashTable* table,
    size_t capacity,
    LWMsgHashGetKeyFunc get_key,
    LWMsgHashDigestFunc digest,
    LWMsgHashEqualFunc equal,
    size_t ring_offset
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i = 0;

    table->count = 0;
    table->capacity = capacity;
    table->get_key = get_key;
    table->digest = digest;
    table->equal = equal;
    table->ring_offset = ring_offset;

    BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(capacity, &table->buckets));

    for (i = 0; i < capacity; i++)
    {
        lwmsg_ring_init(&table->buckets[i]);
    }

error:

    return status;
}

size_t
lwmsg_hash_get_count(
    LWMsgHashTable* table
    )
{
    return table->count;
}

void
lwmsg_hash_insert_entry(
    LWMsgHashTable* table,
    void* entry
    )
{
    void* key = table->get_key(entry);
    size_t hash = table->digest(key);
    LWMsgRing* bucket = &table->buckets[hash % table->capacity];
    LWMsgRing* ring = lwmsg_hash_ring_from_entry(table, entry);

    lwmsg_ring_remove(ring);
    lwmsg_ring_insert_after(bucket, ring);

    table->count++;
}

void*
lwmsg_hash_find_key(
    LWMsgHashTable* table,
    const void* key
    )
{
    size_t hash = table->digest(key);
    LWMsgRing* bucket = &table->buckets[hash % table->capacity];
    LWMsgRing* ring = NULL;
    void* entry;

    for (ring = bucket->next; ring != bucket; ring = ring->next)
    {
        entry = lwmsg_hash_entry_from_ring(table, ring);
        
        if (table->equal(key, table->get_key(entry)))
        {
            return entry;
        }
    }

    return NULL;
}

LWMsgStatus
lwmsg_hash_remove_key(
    LWMsgHashTable* table,
    const void* key
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* entry = lwmsg_hash_find_key(table, key);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }
    
    lwmsg_ring_remove(lwmsg_hash_ring_from_entry(table, entry));
    
    table->count--;

error:

    return status;
}

LWMsgStatus
lwmsg_hash_remove_entry(
    LWMsgHashTable* table,
    void* entry
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing* ring = lwmsg_hash_ring_from_entry(table, entry);

    if (lwmsg_ring_is_empty(ring))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }
    
    lwmsg_ring_remove(lwmsg_hash_ring_from_entry(table, entry));
    
    table->count--;

error:

    return status;
}

void
lwmsg_hash_iter_begin(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    )
{
    if (table->buckets)
    {
        iter->bucket = table->buckets;
        iter->ring = iter->bucket->next;
    }
    else
    {
        iter->bucket = NULL;
        iter->ring = NULL;
    }
}

void*
lwmsg_hash_iter_next(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    )
{
    void* entry = NULL;

    if (!iter->bucket)
    {
        return NULL;
    }

    while (iter->ring == iter->bucket)
    {
        if ((iter->bucket - table->buckets) == table->capacity - 1)
        {
            return NULL;
        }

        iter->bucket++;
        iter->ring = iter->bucket->next;
    }

    entry = lwmsg_hash_entry_from_ring(table, iter->ring);
    iter->ring = iter->ring->next;

    return entry;
}

void
lwmsg_hash_iter_end(
    LWMsgHashTable* table,
    LWMsgHashIter* iter
    )
{
    iter->bucket = NULL;
    iter->ring = NULL;
}

void
lwmsg_hash_destroy(
    LWMsgHashTable* table
    )
{
    if (table->buckets)
    {
        free(table->buckets);
        table->buckets = NULL;
    }
}
