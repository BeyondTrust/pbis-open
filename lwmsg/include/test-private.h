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
 *        test-private.h
 *
 * Abstract:
 *
 *        Unit testing macros (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_TEST_PRIVATE_H__
#define __LWMSG_TEST_PRIVATE_H__

#include "status-private.h"

#define MU_TRY(_x_)                                                     \
    do                                                                  \
    {                                                                   \
        LWMsgStatus __status__ = (_x_);                                 \
        if (__status__)                                                 \
            mu_interface_result(                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                MU_STATUS_EXCEPTION,                                    \
                "%s",                                                   \
                lwmsg_status_name(__status__));                         \
    } while (0)

#define MU_TRY_DCONTEXT(_dcontext_, _x_) MU_TRY((_x_))
#define MU_TRY_PROTOCOL(_context_, _x_) MU_TRY((_x_))
#define MU_TRY_ASSOC(_context_, _x_) MU_TRY((_x_))

#define TEST_ENDPOINT "/tmp/.lwmsg_server_test_socket"
#define TEST_ARCHIVE "/tmp/.lwmsg_test_archive"

void
lwmsg_test_assoc_pair(
    LWMsgProtocolSpec* pspec,
    void (*func1) (LWMsgAssoc* assoc),
    void (*func2) (LWMsgAssoc* assoc)
    );

LWMsgBool
lwmsg_test_log_function(
    LWMsgLogLevel level,
    const char* message,
    const char* function,
    const char* filename,
    unsigned int line,
    void* data
    );

#endif
