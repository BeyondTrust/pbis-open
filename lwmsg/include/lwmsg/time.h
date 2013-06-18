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
    return (time->seconds >= 0 && time->microseconds >= 0);
}

static inline
LWMsgBool
lwmsg_time_is_zero(
    LWMsgTime* time
    )
{
    return (time->seconds == 0 && time->microseconds == 0);
}



#endif
