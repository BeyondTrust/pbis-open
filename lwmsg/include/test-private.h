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
            Mu_Interface_Result(                                        \
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
