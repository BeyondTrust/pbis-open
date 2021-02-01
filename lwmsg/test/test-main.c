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
 *        test-main.c
 *
 * Abstract:
 *
 *        Unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include <moonunit/moonunit.h>
#include <signal.h>
#include <lwmsg/lwmsg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#include "test-private.h"

MU_LIBRARY_DESTRUCT()
{
    unlink(TEST_ENDPOINT);
    unlink(TEST_ARCHIVE);
}

MU_LIBRARY_SETUP()
{
    signal(SIGPIPE, SIG_IGN);
}

/* Helper functions */

typedef struct StartInfo
{
    LWMsgBool connect;
    LWMsgAssoc* assoc;
    void (*func) (LWMsgAssoc* assoc);
} StartInfo;

static
void*
lwmsg_test_assoc_thread(
    void* data
    )
{
    StartInfo* info = data;
    
    if (info->connect)
    {
        MU_TRY_ASSOC(info->assoc, lwmsg_assoc_connect(info->assoc, NULL));
    }
    else
    {
        MU_TRY_ASSOC(info->assoc, lwmsg_assoc_accept(info->assoc, NULL));
    }

    info->func(info->assoc);

    return NULL;
}

void
lwmsg_test_assoc_pair(
    LWMsgProtocolSpec* pspec,
    void (*func1) (LWMsgAssoc* assoc),
    void (*func2) (LWMsgAssoc* assoc)
    )
{
    StartInfo s1 = {0};
    StartInfo s2 = {0};
    pthread_t thread1, thread2;
    int sockets[2];
    static LWMsgProtocol* protocol = NULL;
    
    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY_PROTOCOL(protocol, lwmsg_protocol_add_protocol_spec(protocol, pspec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(NULL,
               protocol,
               &s1.assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               s1.assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));
    
    MU_TRY(lwmsg_connection_new(NULL,
               protocol,
               &s2.assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               s2.assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    s1.func = func1;
    s2.func = func2;

    s1.connect = LWMSG_TRUE;

    if (pthread_create(&thread1, NULL, lwmsg_test_assoc_thread, &s1) ||
        pthread_create(&thread2, NULL, lwmsg_test_assoc_thread, &s2))
    {
        MU_FAILURE("pthread_create() failed");
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}

LWMsgBool
lwmsg_test_log_function(
    LWMsgLogLevel level,
    const char* message,
    const char* function,
    const char* filename,
    unsigned int line,
    void* data
    )
{
    MuLogLevel mu_level = 0;

    if (message)
    {
        switch (level)
        {
        case LWMSG_LOGLEVEL_ALWAYS:
        case LWMSG_LOGLEVEL_ERROR:
            /* MoonUnit does not have an error loglevel because errors
               are considered a test failure.  Log a warning
               instead */
            mu_level = MU_LEVEL_WARNING;
            break;
        case LWMSG_LOGLEVEL_WARNING:
            mu_level = MU_LEVEL_WARNING;
            break;
        case LWMSG_LOGLEVEL_INFO:
            mu_level = MU_LEVEL_VERBOSE;
            break;
        case LWMSG_LOGLEVEL_VERBOSE:
            mu_level = MU_LEVEL_VERBOSE;
            break;
        case LWMSG_LOGLEVEL_DEBUG:
            mu_level = MU_LEVEL_DEBUG;
        break;
        case LWMSG_LOGLEVEL_TRACE:
            mu_level = MU_LEVEL_TRACE;
            break;
        }

        mu_interface_event(filename, line, mu_level, "%s", message);
    }

    return LWMSG_TRUE;
}
