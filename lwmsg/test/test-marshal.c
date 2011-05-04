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
 *        test-marshal.c
 *
 * Abstract:
 *
 *        Marshalling unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include <lwmsg/lwmsg.h>
#include <moonunit/interface.h>
#include <string.h>

#include "test-private.h"
#include "type-private.h"
#include "context-private.h"

static LWMsgContext* context = NULL;
static LWMsgDataContext* dcontext = NULL;
static size_t allocs = 0;

const char pattern[16] = "deadbeefdeadbeef";

static LWMsgStatus
debug_alloc (
    size_t size,
    void** out,
    void* data)
{
    void* object = malloc(size + sizeof(pattern));

    if (!object)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        memset(object, 0, size + sizeof(pattern));

        memcpy(object, pattern, sizeof(pattern));

        *out = ((unsigned char*) object) + sizeof(pattern);

        allocs++;

        return LWMSG_STATUS_SUCCESS;
    }
}

static
void
debug_free (
    void* object,
    void* data
    )
{
    if (object)
    {
        object = ((unsigned char*) object) - sizeof(pattern);

        MU_ASSERT(!memcmp(object, pattern, sizeof(pattern)));
        memset(object, 0, sizeof(pattern));

        allocs--;

        free(object);
    }
}

static LWMsgStatus
debug_realloc (
    void* object,
    size_t old_size,
    size_t new_size,
    void** new_object,
    void* data)
{
    if (object)
    {
        object = ((unsigned char*) object) - sizeof(pattern);

        MU_ASSERT(!memcmp(object, pattern, sizeof(pattern)));

        memset(object, 0, sizeof(pattern));
    }

    void* nobj = realloc(object, new_size + sizeof(pattern));

    if (!nobj)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        memcpy(nobj, pattern, sizeof(pattern));

        if (new_size > old_size)
        {
            memset(nobj + sizeof(pattern) + old_size, 0, new_size - old_size);
        }
        *new_object = ((unsigned char*) nobj) + sizeof(pattern);

        if (!object)
        {
            allocs++;
        }

        return LWMSG_STATUS_SUCCESS;
    }
}

static void
allocate_buffer(LWMsgBuffer* buffer)
{
    size_t length = 2047;
 
    buffer->base = malloc(length);
    buffer->cursor = buffer->base;
    buffer->end = buffer->base + length;
    buffer->wrap = NULL;
    buffer->data = NULL;
}

static void
rewind_buffer(LWMsgBuffer* buffer)
{
    buffer->cursor = buffer->base;
}

MU_FIXTURE_SETUP(marshal)
{
    MU_TRY(lwmsg_context_new(NULL, &context));

    lwmsg_context_set_memory_functions(
               context,
               debug_alloc,
               debug_free,
               debug_realloc,
               NULL);

    lwmsg_context_set_log_function(
        context,
        lwmsg_test_log_function,
        NULL);

    MU_TRY(lwmsg_data_context_new(context, &dcontext));
}

MU_FIXTURE_TEARDOWN(marshal)
{
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, allocs, 0);
}

typedef struct basic_struct
{
    short foo;
    size_t len;
    long *long_ptr;
} basic_struct;

static
LWMsgStatus
basic_verify_foo(
    LWMsgDataContext* dcontext,
    LWMsgBool unmarshalling,
    void* object,
    void* data
    )
{
    short* fooptr = (short*) object;

    if (*fooptr != -42)
    {
        return LWMSG_STATUS_MALFORMED;
    }
    else
    {
        return LWMSG_STATUS_SUCCESS;
    }
}

#define LEN_MAX ((size_t) -1)


static LWMsgTypeSpec basic_spec[] =
{
    LWMSG_STRUCT_BEGIN(basic_struct),
    LWMSG_MEMBER_INT16(basic_struct, foo), LWMSG_ATTR_VERIFY(basic_verify_foo, NULL),
    LWMSG_MEMBER_UINT64(basic_struct, len), LWMSG_ATTR_RANGE(1, LEN_MAX / 2),
    LWMSG_MEMBER_POINTER(basic_struct, long_ptr, LWMSG_INT64(long)), LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(basic_struct, len),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, basic)
{
    static const unsigned char expected[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* 2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };
    LWMsgTypeSpec* type = basic_spec;
    void* buffer;
    size_t length;
    basic_struct basic;
    basic_struct *out;
    long longs[2];

    basic.foo = (short) -42;
    basic.len = 2;
    basic.long_ptr = longs;
    longs[0] = 1234;
    longs[1] = 4321;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &basic, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT(basic.foo == out->foo);
    MU_ASSERT(basic.len == out->len);
    MU_ASSERT(basic.long_ptr[0] == out->long_ptr[0]);
    MU_ASSERT(basic.long_ptr[1] == out->long_ptr[1]);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
    lwmsg_context_free(context, buffer);
}

MU_TEST(marshal, basic_into)
{
    static const unsigned char expected[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* 2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };
    LWMsgTypeSpec* type = basic_spec;
    void* buffer;
    size_t length;
    basic_struct basic;
    basic_struct out;
    long longs[2];
    LWMsgBuffer mbuf = {0};

    basic.foo = (short) -42;
    basic.len = 2;
    basic.long_ptr = longs;
    longs[0] = 1234;
    longs[1] = 4321;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &basic, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    mbuf.base = buffer;
    mbuf.end = buffer + length;
    mbuf.cursor = mbuf.base;
   
    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_into(dcontext, type, &mbuf, &out, sizeof(out)));

    MU_ASSERT(basic.foo == out.foo);
    MU_ASSERT(basic.len == out.len);
    MU_ASSERT(basic.long_ptr[0] == out.long_ptr[0]);
    MU_ASSERT(basic.long_ptr[1] == out.long_ptr[1]);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_destroy_graph(dcontext, type, &out));
    lwmsg_context_free(context, buffer);
}

MU_TEST(marshal, basic_verify_marshal_failure)
{
    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer;
    basic_struct basic;
    long longs[2];

    allocate_buffer(&buffer);

    basic.foo = (short) 12;
    basic.len = 2;
    basic.long_ptr = longs;
    longs[0] = 1234;
    longs[1] = 4321;

    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_marshal(dcontext, type, &basic, &buffer),
        LWMSG_STATUS_MALFORMED);
}

MU_TEST(marshal, basic_verify_unmarshal_failure)
{
    static const unsigned char bytes[] =
    {
        /* 12 */
        0x00, 0x0C,
        /* 2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };

    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer = {0};
    basic_struct *out;

    buffer.base = buffer.cursor = (void*) bytes;
    buffer.end = buffer.base + sizeof(bytes);
    
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_MALFORMED);
}

MU_TEST(marshal, basic_verify_range_failure_low)
{
    static const unsigned char bytes[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* 0 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* pointer */
        0xFF,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };

    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer = {0};
    basic_struct *out;

    buffer.base = buffer.cursor = (void*) bytes;
    buffer.end = buffer.base + sizeof(bytes);
    
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_MALFORMED);
}

MU_TEST(marshal, basic_verify_range_failure_high)
{
    static const unsigned char bytes_64[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* a lot */
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* pointer */
        0xFF,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };

    static const unsigned char bytes_32[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* a lot */
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
        /* pointer */
        0xFF,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1
    };
    const unsigned char* bytes = sizeof(size_t) == 8 ? bytes_64 : bytes_32;

    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer = {0};
    basic_struct *out;

    buffer.base = buffer.cursor = (void*) bytes;
    buffer.end = buffer.base + sizeof(bytes_64);

    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_MALFORMED);
}


MU_TEST(marshal, basic_verify_null_failure)
{
    static const unsigned char bytes[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* 2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    };

    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer = {0};
    basic_struct *out;

    buffer.base = buffer.cursor = (void*) bytes;
    buffer.end = buffer.base + sizeof(bytes);
    
    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_EOF);
}

MU_TEST(marshal, basic_verify_overflow_failure)
{
    static const unsigned char bytes_64[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* max size_t / 2 */
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    static const unsigned char bytes_32[] =
    {
        /* -42 */
        0xFF, 0xD6,
        /* max size_t / 2 */
        0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF
    };

    const unsigned char* bytes = sizeof(size_t) == 8 ? bytes_64 : bytes_32;
    LWMsgTypeSpec* type = basic_spec;
    LWMsgBuffer buffer = {0};
    basic_struct *out;

    buffer.base = buffer.cursor = (void*) bytes;
    buffer.end = buffer.base + sizeof(bytes_32);

    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_OVERFLOW);
}

typedef struct alias_struct
{
    long *ptr1;
    long *ptr2;
} alias_struct;

static LWMsgTypeSpec long_pointer_spec[] =
{
    LWMSG_POINTER(LWMSG_INT64(long)),
    LWMSG_ATTR_LENGTH_STATIC(2),
    LWMSG_TYPE_END
};

static LWMsgTypeSpec alias_spec[] =
{
    LWMSG_STRUCT_BEGIN(alias_struct),
    LWMSG_MEMBER_TYPESPEC(alias_struct, ptr1, long_pointer_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_ALIASABLE,
    LWMSG_MEMBER_TYPESPEC(alias_struct, ptr2, long_pointer_spec),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, alias)
{
    static const unsigned char expected[] =
    {
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1,
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01
    };

    LWMsgTypeSpec* type = alias_spec;
    void* buffer;
    size_t length;
    alias_struct alias;
    alias_struct *out;
    long longs[2];
    char* text = NULL;

    alias.ptr1 = longs;
    alias.ptr2 = longs;

    longs[0] = 1234;
    longs[1] = 4321;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &alias, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT(alias.ptr1[0] == out->ptr1[0]);
    MU_ASSERT(alias.ptr1[1] == out->ptr1[1]);
    MU_ASSERT(out->ptr1 == out->ptr2);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, out, &text));

    MU_VERBOSE("\n%s", text);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
    lwmsg_context_free(context, text);
    lwmsg_context_free(context, buffer);
}

MU_TEST(marshal, alias_null)
{
    static const unsigned char expected[] =
    {
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1,
        /* object id (NULL) */
        0x00, 0x00, 0x00, 0x00
    };

    LWMsgTypeSpec* type = alias_spec;
    void* buffer;
    size_t length;
    alias_struct alias;
    alias_struct *out;
    long longs[2];

    alias.ptr1 = longs;
    alias.ptr2 = NULL;

    longs[0] = 1234;
    longs[1] = 4321;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &alias, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT(alias.ptr1[0] == out->ptr1[0]);
    MU_ASSERT(alias.ptr1[1] == out->ptr1[1]);
    MU_ASSERT(out->ptr2 == NULL);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
    lwmsg_context_free(context, buffer);
}

MU_TEST(marshal, alias_verify_null_marshal_failure)
{
    LWMsgTypeSpec* type = alias_spec;
    void* buffer;
    size_t length;
    alias_struct alias;
    long longs[2];
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    alias.ptr2 = longs;
    alias.ptr1 = NULL;

    longs[0] = 1234;
    longs[1] = 4321;

    status = lwmsg_data_marshal_flat_alloc(dcontext, type, &alias, &buffer, &length);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_MALFORMED);
}

MU_TEST(marshal, alias_verify_null_unmarshal_failure)
{
    static const unsigned char data[] =
    {
        /* object id (NULL) */
        0x00, 0x00, 0x00, 0x00,
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01,
        /* 1234 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xD2,
        /* 4321 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xE1,
    };

    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = alias_spec;
    alias_struct *out;

    status = lwmsg_data_unmarshal_flat(dcontext, type, data, sizeof(data), (void**) (void*) &out);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_MALFORMED);
}

typedef struct ring_struct
{
    int number;
    struct ring_struct* next;
} ring_struct;

static LWMsgTypeSpec ring_ptr_spec[];

static LWMsgTypeSpec ring_spec[] =
{
    LWMSG_STRUCT_BEGIN(ring_struct),
    LWMSG_MEMBER_INT32(ring_struct, number),
    LWMSG_MEMBER_TYPESPEC(ring_struct, next, ring_ptr_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec ring_ptr_spec[] =
{
    LWMSG_POINTER(LWMSG_TYPESPEC(ring_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_TYPE_END
};

MU_TEST(marshal, ring)
{
    static const unsigned char expected[] =
    {
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01,
        /* 10 */
        0x00, 0x00, 0x00, 0x0A,
        /* object id 2 */
        0x00, 0x00, 0x00, 0x02,
        /* 20 */
        0x00, 0x00, 0x00, 0x14,
        /* object id 3 */
        0x00, 0x00, 0x00, 0x03,
        /* 30 */
        0x00, 0x00, 0x00, 0x1e,
        /* object id 1 */
        0x00, 0x00, 0x00, 0x01
    };

    LWMsgTypeSpec* type = ring_ptr_spec;
    void* buffer;
    size_t length;
    ring_struct ring1, ring2, ring3;
    ring_struct *out;
    char* text = NULL;

    ring1.number = 10;
    ring1.next = &ring2;
    ring2.number = 20;
    ring2.next = &ring3;
    ring3.number = 30;
    ring3.next = &ring1;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &ring1, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out->number, 10);
    MU_ASSERT(out->next != NULL);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out->next->number, 20);
    MU_ASSERT(out->next->next != NULL);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out->next->next->number, 30);
    MU_ASSERT_EQUAL(MU_TYPE_POINTER, out, out->next->next->next);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, out, &text));

    MU_VERBOSE("\n%s", text);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
    lwmsg_context_free(context, text);
    lwmsg_context_free(context, buffer);
}

MU_TEST(marshal, ring_print_type)
{
    LWMsgTypeSpec* type = ring_ptr_spec;
    char* text = NULL;

    MU_TRY(lwmsg_type_print_spec_alloc(context, type, &text));

    MU_VERBOSE("\n%s", text);

    lwmsg_context_free(context, text);
}

typedef struct
{
    const char* foo;
    const char* bar;
} string_struct;

static LWMsgTypeSpec string_spec[] =
{
    LWMSG_STRUCT_BEGIN(string_struct),
    LWMSG_MEMBER_PSTR(string_struct, foo),
    LWMSG_ATTR_MAX_ALLOC(256),
    LWMSG_MEMBER_PSTR(string_struct, bar),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, string)
{
    static const unsigned char expected[] =
    {
        /* foo set */
        0xFF,
        /* foo length */
        0x00, 0x00, 0x00, 0x03,
        /* foo contents */
        'f', 'o', 'o',
        /* bar set */
        0xFF,
        /* bar length */
        0x00, 0x00, 0x00, 0x03,
        /* bar contents */
        'b', 'a', 'r'
    };

    LWMsgTypeSpec* type = string_spec;
    LWMsgBuffer buffer;
    string_struct strings;
    string_struct* out;
    char* text = NULL;

    strings.foo = "foo";
    strings.bar = "bar";
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &strings, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, out, &text));

    MU_DEBUG("%s", text);

    MU_ASSERT_EQUAL(MU_TYPE_STRING, strings.foo, out->foo);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, strings.bar, out->bar);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
    lwmsg_context_free(context, text);
}

MU_TEST(marshal, string_max_alloc_fail)
{
    LWMsgTypeSpec* type = string_spec;
    LWMsgBuffer buffer;
    string_struct strings;
    string_struct* out = NULL;
    static char foo[512] = {0};

    memset(foo, 'x', sizeof(foo) - 1);

    strings.foo = foo;
    strings.bar = "bar";
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &strings, &buffer));

    rewind_buffer(&buffer);

    MU_ASSERT_EQUAL(
        MU_TYPE_INTEGER,
        lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out),
        LWMSG_STATUS_OVERFLOW);
}

typedef struct
{
    unsigned int len;
    struct struct_array_inner
    {
        char a;
        char b;
    } *foo;
} struct_array_struct;

static LWMsgTypeSpec struct_array_spec[] =
{
    LWMSG_STRUCT_BEGIN(struct_array_struct),
    LWMSG_MEMBER_UINT32(struct_array_struct, len),
    LWMSG_MEMBER_POINTER_BEGIN(struct_array_struct, foo),
    LWMSG_STRUCT_BEGIN(struct struct_array_inner),
    LWMSG_MEMBER_INT8(struct struct_array_inner, a),
    LWMSG_MEMBER_INT8(struct struct_array_inner, b),
    LWMSG_STRUCT_END,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(struct_array_struct, len),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, struct_array)
{
    static const unsigned char expected[] =
    {
        /* len = 2 */
        0x00, 0x00, 0x00, 0x02,
        /* foo set */
        0xFF,
        /* first struct */
        'a', 'b',
        /* second struct */
        'c', 'd'
    };

    LWMsgTypeSpec* type = struct_array_spec;
    LWMsgBuffer buffer;
    struct_array_struct structs;
    struct_array_struct *out;
    struct struct_array_inner inner[2] = { {'a', 'b'}, {'c', 'd'} };

    structs.len = 2;
    structs.foo = inner;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &structs, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT(structs.len == out->len);
    MU_ASSERT(structs.foo[0].a == out->foo[0].a);
    MU_ASSERT(structs.foo[0].b == out->foo[0].b);
    MU_ASSERT(structs.foo[1].a == out->foo[1].a);
    MU_ASSERT(structs.foo[1].b == out->foo[1].b);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

typedef struct
{
    char ** strings;
} string_array_struct;

static LWMsgTypeSpec string_array_spec[] =
{
    LWMSG_STRUCT_BEGIN(string_array_struct),
    LWMSG_MEMBER_POINTER_BEGIN(string_array_struct, strings),
    LWMSG_PSTR,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, string_array)
{
    static const unsigned char expected[] =
    {
        /* strings = (set) */
        0xFF,
        /* (implicit) length of "strings" = 2 */
        0x00, 0x00, 0x00, 0x02,
        /* strings[0] len */
        0x00, 0x00, 0x00, 0x03,
        /* strings[0] value */
        'f', 'o', 'o',
        /* strings[1] len */
        0x00, 0x00, 0x00, 0x03,
        /* strings[1] value */
        'b', 'a', 'r'
    };

    LWMsgTypeSpec* type = string_array_spec;
    LWMsgBuffer buffer;
    string_array_struct strings;
    string_array_struct *out;
    char* inner[] = { "foo", "bar", NULL };

    strings.strings = (char**) inner;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &strings, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_STRING, strings.strings[0], out->strings[0]);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, strings.strings[1], out->strings[1]);
    MU_ASSERT(out->strings[2] == NULL);

    lwmsg_data_free_graph(dcontext, type, out);
}

MU_TEST(marshal, string_array_empty)
{
    static const unsigned char expected[] =
    {
        /* strings = (set) */
        0xFF,
        /* (implicit) length of "strings" = 0 */
        0x00, 0x00, 0x00, 0x00
    };

    LWMsgTypeSpec* type = string_array_spec;
    LWMsgBuffer buffer;
    string_array_struct strings;
    string_array_struct *out;
    char* inner[] = { NULL };

    strings.strings = (char**) inner;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &strings, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, strings.strings[0], out->strings[0]);

    lwmsg_data_free_graph(dcontext, type, out);
}

MU_TEST(marshal, string_array_null)
{
    static const unsigned char expected[] =
    {
        /* strings = (not set) */
        0x00
    };

    LWMsgTypeSpec* type = string_array_spec;
    LWMsgBuffer buffer;
    string_array_struct strings;
    string_array_struct *out;

    strings.strings = NULL;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &strings, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, out->strings, NULL);

    lwmsg_data_free_graph(dcontext, type, out);
}


#define TAG_NUMBER 1
#define TAG_STRING 2
#define TAG_EMPTY  3

typedef union
{
    int number;
    char* string;
} number_string_union;

static LWMsgTypeSpec number_string_spec[] =
{
    LWMSG_UNION_BEGIN(number_string_union),
    LWMSG_MEMBER_INT32(number_string_union, number),
    LWMSG_ATTR_TAG(TAG_NUMBER),
    LWMSG_MEMBER_PSTR(number_string_union, string),
    LWMSG_ATTR_TAG(TAG_STRING),
    LWMSG_VOID,
    LWMSG_ATTR_TAG(TAG_EMPTY),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

typedef struct
{
    int tag1;
    number_string_union* u1;
    int tag2;
    number_string_union* u2;
} two_union_struct;

static LWMsgTypeSpec two_union_spec[] =
{
    LWMSG_STRUCT_BEGIN(two_union_struct),
    LWMSG_MEMBER_INT8(two_union_struct, tag1),
    LWMSG_MEMBER_POINTER_BEGIN(two_union_struct, u1),
    LWMSG_TYPESPEC(number_string_spec),
    LWMSG_ATTR_DISCRIM(two_union_struct, tag1),
    LWMSG_POINTER_END,
    LWMSG_MEMBER_INT8(two_union_struct, tag2),
    LWMSG_MEMBER_POINTER_BEGIN(two_union_struct, u2),
    LWMSG_TYPESPEC(number_string_spec),
    LWMSG_ATTR_DISCRIM(two_union_struct, tag2),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, two_union)
{
    static const unsigned char expected[] =
    {
        /* tag1 = 1 */
        0x01,
        /* u1 = set */
        0xFF,
        /* u1->number = 42 */
        0x00, 0x00, 0x00, 0x2A,
        /* tag2 = 2 */
        0x02,
        /* u2 = set */
        0xFF,
        /* u2->string = set */
        0xFF,
        /* u2->string length is 3 (implicit) */
        0x00, 0x00, 0x00, 0x03,
        /* u2->string pointee */
        'f', 'o', 'o'
    };

    LWMsgTypeSpec* type = two_union_spec;
    LWMsgBuffer buffer;
    two_union_struct unions;
    two_union_struct* out;
    number_string_union u1, u2;

    u1.number = 42;
    u2.string = (char*) "foo";

    unions.tag1 = TAG_NUMBER;
    unions.u1 = &u1;
    unions.tag2 = TAG_STRING;
    unions.u2 = &u2;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &unions, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag1, out->tag1);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag2, out->tag2);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.u1->number, out->u1->number);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, unions.u2->string, out->u2->string);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

typedef struct
{
    int foo;
    struct inner_struct {
        int foo;
        const char* bar;
    } inner;
    int bar;
} nested_struct_struct;

static LWMsgTypeSpec nested_struct_spec[] =
{
    LWMSG_STRUCT_BEGIN(nested_struct_struct),
    LWMSG_MEMBER_INT32(nested_struct_struct, foo),
    LWMSG_MEMBER_STRUCT_BEGIN(nested_struct_struct, inner),
    LWMSG_MEMBER_INT32(struct inner_struct, foo),
    LWMSG_MEMBER_PSTR(struct inner_struct, bar),
    LWMSG_STRUCT_END,
    LWMSG_MEMBER_INT32(nested_struct_struct, bar),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, nested_struct)
{
    static const unsigned char expected[] =
    {
        /* foo = 42 */
        0x00, 0x00, 0x00, 0x2A,
        /* inner.foo = 24 */
        0x00, 0x00, 0x00, 0x18,
        /* inner.bar = (set) */
        0xFF,
        /* inner.bar length (implicit) */
        0x00, 0x00, 0x00, 0x03,
        /* inner.bar value */
        'b', 'a', 'r',
        /* bar = -12 */
        0xFF, 0xFF, 0xFF, 0xF4,
    };

    LWMsgTypeSpec* type = nested_struct_spec;
    LWMsgBuffer buffer;
    nested_struct_struct nested;
    nested_struct_struct* out;

    nested.foo = 42;
    nested.inner.foo = 24;
    nested.inner.bar = "bar";
    nested.bar = -12;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &nested, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, nested.foo, out->foo);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, nested.inner.foo, out->inner.foo);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, nested.inner.bar, out->inner.bar);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, nested.bar, out->bar);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

typedef struct
{
    int tag1;
    number_string_union u1;
    int tag2;
    number_string_union u2;
} nested_union_struct;

static LWMsgTypeSpec nested_union_spec[] =
{
    LWMSG_STRUCT_BEGIN(nested_union_struct),
    LWMSG_MEMBER_INT8(nested_union_struct, tag1),
    LWMSG_MEMBER_TYPESPEC(nested_union_struct, u1, number_string_spec),
    LWMSG_ATTR_DISCRIM(nested_union_struct, tag1),
    LWMSG_MEMBER_INT8(nested_union_struct, tag2),
    LWMSG_MEMBER_TYPESPEC(nested_union_struct, u2, number_string_spec),
    LWMSG_ATTR_DISCRIM(nested_union_struct, tag2),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, nested_union)
{
    static const unsigned char expected[] =
    {
        /* tag1 = 2 */
        0x02,
        /* u1.string = (set) */
        0xFF,
        /* u1.string length is 3 (implicit) */
        0x00, 0x00, 0x00, 0x03,
        /* u1.string value */
        'f', 'o', 'o',
        /* tag2 = 1 */
        0x01,
        /* u2.number = 42 */
        0x00, 0x00, 0x00, 0x2A
    };

    LWMsgTypeSpec* type = nested_union_spec;
    LWMsgBuffer buffer;
    nested_union_struct unions;
    nested_union_struct* out;

    unions.tag1 = TAG_STRING;
    unions.u1.string = (char*) "foo";
    unions.tag2 = TAG_NUMBER;
    unions.u2.number = 42;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &unions, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag1, out->tag1);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag2, out->tag2);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, unions.u1.string, out->u1.string);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.u2.number, out->u2.number);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

MU_TEST(marshal, nested_union_empty)
{
    static const unsigned char expected[] =
    {
        /* tag1 = 3 (empty) */
        0x03,
        /* tag2 = 1 */
        0x01,
        /* u2.number = 42 */
        0x00, 0x00, 0x00, 0x2A
    };

    LWMsgTypeSpec* type = nested_union_spec;
    LWMsgBuffer buffer;
    nested_union_struct unions;
    nested_union_struct* out;

    unions.tag1 = TAG_EMPTY;
    unions.tag2 = TAG_NUMBER;
    unions.u2.number = 42;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &unions, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag1, out->tag1);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.tag2, out->tag2);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, unions.u2.number, out->u2.number);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

typedef struct
{
    int foo;
    int array[2];
    int bar;
} inline_array_struct;

static LWMsgTypeSpec inline_array_spec[] =
{
    LWMSG_STRUCT_BEGIN(inline_array_struct),
    LWMSG_MEMBER_INT16(inline_array_struct, foo),
    LWMSG_MEMBER_ARRAY_BEGIN(inline_array_struct, array),
    LWMSG_INT16(int),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_STATIC(2),
    LWMSG_MEMBER_INT16(inline_array_struct, bar),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, inline_array)
{
    static const unsigned char expected[] =
    {
        /* foo = 1 */
        0x00, 0x01,
        /* array[0] = 2 */
        0x00, 0x02,
        /* array[1] = 3 */
        0x00, 0x03,
        /* bar = 4 */
        0x00, 0x04
    };

    LWMsgTypeSpec* type = inline_array_spec;
    LWMsgBuffer buffer;
    inline_array_struct in;
    inline_array_struct* out;

    in.foo = 1;
    in.array[0] = 2;
    in.array[1] = 3;
    in.bar = 4;
    
    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &in, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.foo, out->foo);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.array[0], out->array[0]);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.array[1], out->array[1]);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.bar, out->bar);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

typedef struct
{
    int foo;
    char string[];
} flexible_string_struct;

static LWMsgTypeSpec flexible_string_spec[] =
{
    LWMSG_STRUCT_BEGIN(flexible_string_struct),
    LWMSG_MEMBER_INT16(flexible_string_struct, foo),
    LWMSG_MEMBER_ARRAY_BEGIN(flexible_string_struct, string),
    LWMSG_UINT8(char),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_STRING,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, flexible_string)
{
    static const unsigned char expected[] =
    {
        /* foo = 42 */
        0x00, 0x2A,
        /* string length = 3 (implicit) */
        0x00, 0x00, 0x00, 0x03,
        /* string value */
        'f', 'o', 'o'
    };

    LWMsgTypeSpec* type = flexible_string_spec;
    LWMsgBuffer buffer;
    flexible_string_struct* in;
    flexible_string_struct* out;

    in = malloc(sizeof(*in) + 4);
    in->foo = 42;
    strcpy(in->string, "foo");

    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, in, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in->foo, out->foo);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, in->string, out->string);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));

    free(in);
}

MU_TEST(marshal, flexible_string_print_type)
{
    LWMsgTypeSpec* type = flexible_string_spec;
    char* text = NULL;

    MU_TRY(lwmsg_type_print_spec_alloc(context, type, &text));

    MU_VERBOSE("\n%s", text);

    lwmsg_context_free(context, text);
}

typedef struct
{
    int len;
    int array[];
} flexible_array_struct;

static LWMsgTypeSpec flexible_array_spec[] =
{
    LWMSG_STRUCT_BEGIN(flexible_array_struct),
    LWMSG_MEMBER_INT16(flexible_array_struct, len),
    LWMSG_MEMBER_ARRAY_BEGIN(flexible_array_struct, array),
    LWMSG_INT16(int),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_MEMBER(flexible_array_struct, len),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, flexible_array)
{
    static const unsigned char expected[] =
    {
        /* len = 2 */
        0x00, 0x02,
        /* array[0] = 1 */
        0x00, 0x01,
        /* array[1] = 2 */
        0x00, 0x02
    };

    LWMsgTypeSpec* type = flexible_array_spec;
    LWMsgBuffer buffer;
    flexible_array_struct* in;
    flexible_array_struct* out;

    in = malloc(sizeof(*in) + sizeof(int) * 2);
    in->len = 2;
    in->array[0] = 1;
    in->array[1] = 2;

    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, in, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in->len, out->len);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in->array[0], out->array[0]);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in->array[1], out->array[1]);

    free(in);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

MU_TEST(marshal, flexible_array_print_type)
{
    LWMsgTypeSpec* type = flexible_array_spec;
    char* text = NULL;

    MU_TRY(lwmsg_type_print_spec_alloc(context, type, &text));
    MU_VERBOSE("\n%s", text);

    lwmsg_context_free(context, text);
}


typedef struct info_level_struct
{
    unsigned int length;
    int level;
    union level_union
    {
        struct level_1_struct
        {
            short number1;
        } *level_1;
        struct level_2_struct
        {
            short number1;
            short number2;
        } *level_2;
    } array;
} info_level_struct;

static LWMsgTypeSpec info_level_spec[] =
{
    LWMSG_STRUCT_BEGIN(struct info_level_struct),
    LWMSG_MEMBER_UINT32(struct info_level_struct, length),
    LWMSG_MEMBER_INT8(struct info_level_struct, level),
    LWMSG_MEMBER_UNION_BEGIN(struct info_level_struct, array),
    LWMSG_MEMBER_POINTER_BEGIN(union level_union, level_1),
    LWMSG_STRUCT_BEGIN(struct level_1_struct),
    LWMSG_MEMBER_INT16(struct level_1_struct, number1),
    LWMSG_STRUCT_END,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(struct info_level_struct, length),
    LWMSG_ATTR_TAG(1),
    LWMSG_MEMBER_POINTER_BEGIN(union level_union, level_2),
    LWMSG_STRUCT_BEGIN(struct level_2_struct),
    LWMSG_MEMBER_INT16(struct level_2_struct, number1),
    LWMSG_MEMBER_INT16(struct level_2_struct, number2),
    LWMSG_STRUCT_END,
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(struct info_level_struct, length),
    LWMSG_ATTR_TAG(2),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(struct info_level_struct, level),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

MU_TEST(marshal, info_level_print_type)
{
    char* text = NULL;

    MU_TRY(lwmsg_type_print_spec_alloc(context, info_level_spec, &text));
    MU_VERBOSE("\n%s", text);

    lwmsg_context_free(context, text);
}

MU_TEST(marshal, info_level_1)
{
    static const unsigned char expected[] =
    {
        /* length */
        0x00, 0x00, 0x00, 0x02,
        /* level */
        0x01,
        /* array.level_1 set */
        0xFF,
        /* array.level_1[0].number1 */
        0x00, 0x2A,
        /* array.level_1[1].number1 */
        0x00, 0x54
    };

    static struct level_1_struct array[] = 
    {
        {42},
        {84}
    };

    LWMsgTypeSpec* type = info_level_spec;
    LWMsgBuffer buffer;
    info_level_struct in;
    info_level_struct* out;
    int i;

    in.length = sizeof(array) / sizeof(*array);
    in.level = 1;
    in.array.level_1 = array;

    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &in, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.length, out->length);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.level, out->level);
    
    for (i = 0; i < out->length; i++)
    {
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.array.level_1[0].number1, out->array.level_1[0].number1);
    }

    lwmsg_data_free_graph(dcontext, type, out);
}

MU_TEST(marshal, info_level_2)
{
    static const unsigned char expected[] =
    {
        /* length */
        0x00, 0x00, 0x00, 0x02,
        /* level */
        0x02,
        /* array.level_2 set */
        0xFF,
        /* array.level_2[0].number1 */
        0x00, 0x2A,
        /* array.level_2[0].number2 */
        0xFF, 0xD6,
        /* array.level_2[1].number1 */
        0x00, 0x54,
        /* array.level_2[1].number2 */
        0xFF, 0xAC
    };

    static struct level_2_struct array[] = 
    {
        {42, -42},
        {84, -84}
    };

    LWMsgTypeSpec* type = info_level_spec;
    LWMsgBuffer buffer;
    info_level_struct in;
    info_level_struct* out;
    int i;

    in.length = sizeof(array) / sizeof(*array);
    in.level = 2;
    in.array.level_2 = array;

    allocate_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal(dcontext, type, &in, &buffer));

    MU_ASSERT(!memcmp(buffer.base, expected, sizeof(expected)));

    rewind_buffer(&buffer);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal(dcontext, type, &buffer, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.length, out->length);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.level, out->level);
    
    for (i = 0; i < out->length; i++)
    {
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.array.level_2[0].number1, out->array.level_2[0].number1);
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in.array.level_2[0].number2, out->array.level_2[0].number2);
    }

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

MU_TEST(marshal, type_rep_print_type)
{
    char* text = NULL;

    MU_TRY(lwmsg_type_print_spec_alloc(context, lwmsg_type_rep_spec, &text));

    MU_VERBOSE("\n%s", text);
    lwmsg_context_free(context, text);
}

MU_TEST(marshal, type_spec_ping_pong)
{
    LWMsgMemoryList list;
    LWMsgTypeRep* rep = NULL;
    LWMsgTypeSpec* spec = NULL;
    char* text = NULL;

    lwmsg_memlist_init(&list, context);

    MU_TRY(lwmsg_type_rep_from_spec(context, lwmsg_type_rep_spec, &rep));
    MU_TRY(lwmsg_type_spec_from_rep(lwmsg_memlist_context(&list), rep, &spec));
    MU_TRY(lwmsg_type_print_spec_alloc(context, spec, &text));

    MU_VERBOSE("\n%s", text);
    lwmsg_context_free(context, text);
    lwmsg_type_free_rep(context, rep);
    lwmsg_memlist_destroy(&list);
}

MU_TEST(marshal, type_rep_rep_self_assignable)
{
    LWMsgTypeRep* rep = NULL;

    MU_TRY(lwmsg_type_rep_from_spec(context, lwmsg_type_rep_spec, &rep));

    MU_TRY(lwmsg_type_rep_is_assignable(rep, rep));

    lwmsg_type_free_rep(context, rep);
}

typedef enum MixedEnum
{
    MIXED_VALUE_1 = 1,
    MIXED_VALUE_2 = 2,
    MIXED_MASK_1 = 0x1000,
    MIXED_MASK_2 = 0x2000,
} MixedEnum;

static LWMsgTypeSpec MixedEnum_spec[] =
{
    LWMSG_ENUM_BEGIN(MixedEnum, 2, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(MIXED_VALUE_1),
    LWMSG_ENUM_VALUE(MIXED_VALUE_2),
    LWMSG_ENUM_MASK(MIXED_MASK_1),
    LWMSG_ENUM_MASK(MIXED_MASK_2),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END,
};

MU_TEST(marshal, mixed_enum_mixed)
{
    static const unsigned char expected[] =
    {
        /* value */
        0x20, 0x01
    };

    LWMsgTypeSpec* type = MixedEnum_spec;
    void* buffer = NULL;
    MixedEnum in = MIXED_VALUE_1 | MIXED_MASK_2;
    MixedEnum* out = 0;
    char* text = NULL;
    size_t length = 0;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &in, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in, *out);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, &in, &text));

    MU_VERBOSE("%s", text);

    lwmsg_context_free(context, text);
    lwmsg_context_free(context, buffer);
    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

MU_TEST(marshal, mixed_enum_value)
{
    static const unsigned char expected[] =
    {
        /* value */
        0x00, 0x01
    };

    LWMsgTypeSpec* type = MixedEnum_spec;
    void* buffer = NULL;
    MixedEnum in = MIXED_VALUE_1;
    MixedEnum* out = 0;
    char* text = NULL;
    size_t length = 0;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &in, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in, *out);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, &in, &text));

    MU_VERBOSE("%s", text);

    lwmsg_context_free(context, text);
    lwmsg_context_free(context, buffer);
    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}

MU_TEST(marshal, mixed_enum_unmarshal_zero_fail)
{
    static const unsigned char data[] =
    {
        /* value */
        0x10, 0x00
    };

    LWMsgTypeSpec* type = MixedEnum_spec;
    MixedEnum* out = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_data_unmarshal_flat(dcontext, type, data, sizeof(data), (void**) (void*) &out);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_MALFORMED);
}

MU_TEST(marshal, mixed_enum_unmarshal_bogus_fail)
{
    static const unsigned char data[] =
    {
        /* value */
        0x00, 0x03
    };

    LWMsgTypeSpec* type = MixedEnum_spec;
    MixedEnum* out = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_data_unmarshal_flat(dcontext, type, data, sizeof(data), (void**) (void*) &out);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_MALFORMED);
}

typedef enum MaskEnum
{
    MASK_MASK_1 = 0x1000,
    MASK_MASK_2 = 0x2000,
} MaskEnum;

static LWMsgTypeSpec MaskEnum_spec[] =
{
    LWMSG_ENUM_BEGIN(MaskEnum, 2, LWMSG_UNSIGNED),
    LWMSG_ENUM_MASK(MASK_MASK_1),
    LWMSG_ENUM_MASK(MASK_MASK_2),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END,
};

MU_TEST(marshal, mask_enum_zero)
{
    static const unsigned char expected[] =
    {
        /* value */
        0x00, 0x00
    };

    LWMsgTypeSpec* type = MaskEnum_spec;
    void* buffer = NULL;
    MaskEnum in = 0;
    MaskEnum* out = NULL;
    char* text = NULL;
    size_t length = 0;

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_marshal_flat_alloc(dcontext, type, &in, &buffer, &length));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, sizeof(expected), length);
    MU_ASSERT(!memcmp(buffer, expected, sizeof(expected)));

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_unmarshal_flat(dcontext, type, buffer, length, (void**) (void*) &out));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, in, *out);

    MU_TRY_DCONTEXT(dcontext, lwmsg_data_print_graph_alloc(dcontext, type, &in, &text));

    MU_VERBOSE("%s", text);

    lwmsg_context_free(context, text);
    lwmsg_context_free(context, buffer);
    MU_TRY_DCONTEXT(dcontext, lwmsg_data_free_graph(dcontext, type, out));
}
