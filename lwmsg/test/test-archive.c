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
 *        test-archive.c
 *
 * Abstract:
 *
 *        Archive unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include <lwmsg/lwmsg.h>
#include <moonunit/interface.h>
#include <string.h>

#include "test-private.h"

typedef struct message_struct
{
    int number;
    char* string;
} message_struct;

typedef enum message_tag
{
    MESSAGE_NORMAL,
    MESSAGE_EXTRA
} message_tag;

static LWMsgTypeSpec message_struct_spec[] =
{
    LWMSG_STRUCT_BEGIN(message_struct),
    LWMSG_MEMBER_INT16(message_struct, number),
    LWMSG_MEMBER_PSTR(message_struct, string),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec archive_spec[] =
{
    LWMSG_MESSAGE(MESSAGE_NORMAL, message_struct_spec),
    LWMSG_PROTOCOL_END
};

static LWMsgProtocolSpec archive_extra_spec[] =
{
    LWMSG_MESSAGE(MESSAGE_NORMAL, message_struct_spec),
    LWMSG_MESSAGE(MESSAGE_EXTRA, message_struct_spec),
    LWMSG_PROTOCOL_END
};

static LWMsgProtocol* archive_protocol = NULL;
static LWMsgProtocol* archive_extra_protocol = NULL;
static LWMsgDataContext* dcontext = NULL;

MU_FIXTURE_SETUP(archive)
{
    MU_TRY(lwmsg_protocol_new(NULL, &archive_protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(archive_protocol, archive_spec));
    MU_TRY(lwmsg_protocol_new(NULL, &archive_extra_protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(archive_extra_protocol, archive_extra_spec));
    MU_TRY(lwmsg_data_context_new(NULL, &dcontext));
}

MU_TEST(archive, write_read)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    message_struct *result = NULL;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out = LWMSG_MESSAGE_INITIALIZER;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_NORMAL;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, write message, close */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));

    /* Open, read message, close, delete */
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ));
    MU_TRY(lwmsg_archive_read_message(archive, &out));
    MU_TRY(lwmsg_archive_close(archive));
    
    /* Compare written and read messages */
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, out.tag, MESSAGE_NORMAL);
    result = out.data;
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, result->number, payload.number);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, result->string, payload.string);

    MU_TRY(lwmsg_archive_destroy_message(archive, &out));
    lwmsg_archive_delete(archive);
}

MU_TEST(archive, write_schema_read_schema)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out = LWMSG_MESSAGE_INITIALIZER;
    char* text = NULL;
    LWMsgProtocol* blank_protocol = NULL;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_NORMAL;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, write message, close, delete */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));
    lwmsg_archive_delete(archive);

    /* Create a blank protocol which we will read the schema into */
    MU_TRY(lwmsg_protocol_new(NULL, &blank_protocol));
    MU_TRY(lwmsg_archive_new(NULL, blank_protocol, &archive));
    lwmsg_archive_set_protocol_update(archive, LWMSG_TRUE);

    /* Open, read message, close */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_read_message(archive, &out));
    MU_TRY(lwmsg_archive_close(archive));
    
    MU_TRY(lwmsg_assoc_print_message_alloc(lwmsg_archive_as_assoc(archive), &out, &text));

    MU_VERBOSE("\n%s", text);

    MU_TRY(lwmsg_archive_destroy_message(archive, &out));
    lwmsg_archive_delete(archive);
}

MU_TEST(archive, write_schema_read_schema_check)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out = LWMSG_MESSAGE_INITIALIZER;
    char* text = NULL;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_NORMAL;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, write message, close, delete */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));
    lwmsg_archive_delete(archive);

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, read message, close */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_read_message(archive, &out));
    MU_TRY(lwmsg_archive_close(archive));
    
    MU_TRY(lwmsg_assoc_print_message_alloc(lwmsg_archive_as_assoc(archive), &out, &text));
    
    MU_VERBOSE("\n%s", text);
    
    MU_TRY(lwmsg_archive_destroy_message(archive, &out));
    lwmsg_archive_delete(archive);
}

MU_TEST(archive, write_schema_read_extra_schema_check)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out = LWMSG_MESSAGE_INITIALIZER;
    char* text = NULL;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_NORMAL;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, write message, close, delete */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));
    lwmsg_archive_delete(archive);

    MU_TRY(lwmsg_archive_new(NULL, archive_extra_protocol, &archive));

    /* Open, read message, close */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_read_message(archive, &out));
    MU_TRY(lwmsg_archive_close(archive));
    
    MU_TRY(lwmsg_assoc_print_message_alloc(lwmsg_archive_as_assoc(archive), &out, &text));
    
    MU_VERBOSE("\n%s", text);
    
    MU_TRY(lwmsg_archive_destroy_message(archive, &out));
    lwmsg_archive_delete(archive);
}

MU_TEST(archive, write_extra_schema_read_schema_update)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage out = LWMSG_MESSAGE_INITIALIZER;
    char* text = NULL;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_EXTRA;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_extra_protocol, &archive));

    /* Open, write message, close, delete */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));
    lwmsg_archive_delete(archive);

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));
    lwmsg_archive_set_protocol_update(archive, LWMSG_TRUE);

    /* Open, read message, close */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_read_message(archive, &out));
    MU_TRY(lwmsg_archive_close(archive));
    
    MU_TRY(lwmsg_assoc_print_message_alloc(lwmsg_archive_as_assoc(archive), &out, &text));
    
    MU_VERBOSE("\n%s", text);
    
    MU_TRY(lwmsg_archive_destroy_message(archive, &out));
    lwmsg_archive_delete(archive);
}

MU_TEST(archive, write_extra_schema_read_schema_check_fails)
{
    LWMsgArchive* archive = NULL;
    message_struct payload;
    LWMsgMessage in = LWMSG_MESSAGE_INITIALIZER;

    payload.number = 42;
    payload.string = (char*) "Hello, world!";
    in.tag = MESSAGE_EXTRA;
    in.data = &payload;

    MU_TRY(lwmsg_archive_new(NULL, archive_extra_protocol, &archive));

    /* Open, write message, close, delete */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0600));
    MU_TRY(lwmsg_archive_open(archive, LWMSG_ARCHIVE_WRITE | LWMSG_ARCHIVE_SCHEMA));
    MU_TRY(lwmsg_archive_write_message(archive, &in));
    MU_TRY(lwmsg_archive_close(archive));
    lwmsg_archive_delete(archive);

    MU_TRY(lwmsg_archive_new(NULL, archive_protocol, &archive));

    /* Open, should fail due to schema mismatch */
    MU_TRY(lwmsg_archive_set_file(archive, TEST_ARCHIVE, 0));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA), LWMSG_STATUS_MALFORMED);

    lwmsg_archive_delete(archive);
}

