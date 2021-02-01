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
 *        lwma-main.c
 *
 * Abstract:
 *
 *        Likewise Message Archive tool
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <lwmsg/lwmsg.h>
#include "util-private.h"
#include "data-private.h"
#include "status-private.h"
#include "protocol-private.h"
#include "buffer-private.h"

static
LWMsgStatus
archive_dump(
    const char* filename
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgProtocol* protocol = NULL;
    LWMsgArchive* archive = NULL;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    char* text = NULL;
    unsigned int i = 0;

    BAIL_ON_ERROR(status = lwmsg_protocol_new(NULL, &protocol));
    BAIL_ON_ERROR(status = lwmsg_archive_new(NULL, protocol, &archive));
    lwmsg_archive_set_protocol_update(archive, LWMSG_TRUE);
    BAIL_ON_ERROR(status = lwmsg_archive_set_file(archive, filename, 0));
    BAIL_ON_ERROR(status = lwmsg_archive_open(archive, LWMSG_ARCHIVE_READ | LWMSG_ARCHIVE_SCHEMA));

    printf("------\n");
    printf("Schema\n");
    printf("------\n\n");

    BAIL_ON_ERROR(status = lwmsg_protocol_print_alloc(protocol, 0, &text));
    printf("%s\n\n", text);
    free(text);
    text = NULL;

    printf("--------\n");
    printf("Messages\n");
    printf("--------\n\n");

    for (i = 0; ; i++)
    {
        status = lwmsg_archive_read_message(archive, &message);

        if (status == LWMSG_STATUS_EOF)
        {
            status = LWMSG_STATUS_SUCCESS;
            break;
        }

        BAIL_ON_ERROR(status);

        BAIL_ON_ERROR(status = lwmsg_assoc_print_message_alloc(lwmsg_archive_as_assoc(archive), &message, &text));
        printf("[%i] %s\n\n", i, text);

        free(text);
        text = NULL;
        lwmsg_archive_destroy_message(archive, &message);
    }

error:

    if (text)
    {
        free(text);
    }

    if (archive)
    {
        lwmsg_archive_destroy_message(archive, &message);
        lwmsg_archive_close(archive);
        lwmsg_archive_delete(archive);
    }

    if (protocol)
    {
        lwmsg_protocol_delete(protocol);
    }

    return status;
}

static
void
help()
{
    printf(
        "Usage: lwma <command> <file>\n\n"
        "Commands:\n"
        "  dump                Dump contents of archive\n\n");
}

int
main(
    int argc,
    char** argv
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (argc < 3)
    {
        help();
        exit(1);
    }

    if (!strcmp(argv[1], "dump"))
    {
        BAIL_ON_ERROR(status = archive_dump(argv[2]));
    }
    else
    {
        help();
        exit(1);
    }

error:

    if (status)
    {
        fprintf(stderr, "Error: %i\n", status);
        return 1;
    }
    else
    {
        return 0;
    }
}
