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
 *        time.h
 *
 * Abstract:
 *
 *        Time API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_TIME_H__
#define __LWMSG_TIME_H__

#include <stdlib.h>
#include <unistd.h>
#include <lwmsg/status.h>
#include <lwmsg/common.h>

typedef struct LWMsgTime
{
    ssize_t seconds;
    ssize_t microseconds;
} LWMsgTime;

typedef struct LWMsgClock
{
    LWMsgTime last_time;
    LWMsgTime adjust;
} LWMsgClock;

typedef enum LWMsgTimeComparison
{
    LWMSG_TIME_EQUAL = 0,
    LWMSG_TIME_GREATER = 1,
    LWMSG_TIME_LESSER = -1
} LWMsgTimeComparison;

LWMsgStatus
lwmsg_time_now(
    LWMsgTime* out
    );

void
lwmsg_time_difference(
    LWMsgTime* from,
    LWMsgTime* to,
    LWMsgTime* out
    );

void
lwmsg_time_sum(
    LWMsgTime* base,
    LWMsgTime* offset,
    LWMsgTime* out
    );

LWMsgTimeComparison
lwmsg_time_compare(
    LWMsgTime* a,
    LWMsgTime* b
    );

void
lwmsg_time_normalize(
    LWMsgTime* time
    );

void
lwmsg_clock_init(
    LWMsgClock* clock
    );

LWMsgStatus
lwmsg_clock_get_wall_time(
    LWMsgClock* clock,
    LWMsgTime* time
    );

LWMsgStatus
lwmsg_clock_get_monotonic_time(
    LWMsgClock* clock,
    LWMsgTime* time
    );

static inline
LWMsgBool
lwmsg_time_is_positive(
    LWMsgTime* time
    )
{
    return (time->seconds >= 0 && time->microseconds >= 0) ? LWMSG_TRUE : LWMSG_FALSE;
}

static inline
LWMsgBool
lwmsg_time_is_zero(
    LWMsgTime* time
    )
{
    return (time->seconds == 0 && time->microseconds == 0) ? LWMSG_TRUE : LWMSG_FALSE;
}



#endif
