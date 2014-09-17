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
 *        time.c
 *
 * Abstract:
 *
 *        Time API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <lwmsg/time.h>

#include "util-private.h"

LWMsgStatus
lwmsg_time_now(
    LWMsgTime* out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
    {
        BAIL_ON_ERROR(status = lwmsg_status_map_errno(errno));
    }

    out->seconds = (ssize_t) tv.tv_sec;
    out->microseconds = (ssize_t) tv.tv_usec;

error:

    return status;
}

void
lwmsg_time_difference(
    LWMsgTime* from,
    LWMsgTime* to,
    LWMsgTime* out
    )
{
    out->seconds = to->seconds - from->seconds;
    out->microseconds = to->microseconds - from->microseconds;
    lwmsg_time_normalize(out);
}

void
lwmsg_time_sum(
    LWMsgTime* base,
    LWMsgTime* offset,
    LWMsgTime* out
    )
{
    out->seconds = base->seconds + offset->seconds;
    out->microseconds = base->microseconds + offset->microseconds;
    lwmsg_time_normalize(out);
}

LWMsgTimeComparison
lwmsg_time_compare(
    LWMsgTime* a,
    LWMsgTime* b
    )
{
    LWMsgTime diff;

    lwmsg_time_difference(a, b, &diff);

    if (diff.seconds == 0 && diff.microseconds == 0)
    {
        return LWMSG_TIME_EQUAL;
    }
    else if (diff.seconds < 0 || diff.microseconds < 0)
    {
        return LWMSG_TIME_GREATER;
    }
    else
    {
        return LWMSG_TIME_LESSER;
    }
}

void
lwmsg_time_normalize(
    LWMsgTime* time
    )
{
    /* First, make both values the same sign unless one is zero */
    while (time->seconds < 0 && time->microseconds > 0)
    {
        time->seconds += 1;
        time->microseconds -= 1000000;
    }
    while (time->microseconds < 0 && time->seconds > 0)
    {
        time->seconds -= 1;
        time->microseconds += 1000000;
    }

    /* Second, make sure the microseconds value is < 1,000,000 */
    while (time->microseconds <= -1000000)
    {
        time->microseconds += 1000000;
        time->seconds -= 1;
    }
    while (time->microseconds >= 1000000)
    {
        time->microseconds -= 1000000;
        time->seconds += 1;
    }
}

void
lwmsg_clock_init(
    LWMsgClock* clock
    )
{
    memset(clock, 0, sizeof(*clock));
}

static
LWMsgStatus
lwmsg_clock_update(
    LWMsgClock* clock
    )
{
    LWMsgTime now, diff;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;


    BAIL_ON_ERROR(status = lwmsg_time_now(&now));

    if (clock->last_time.seconds == 0 &&
        clock->last_time.microseconds == 0)
    {
        clock->adjust.seconds = -now.seconds;
        clock->adjust.microseconds = -now.microseconds;
    }
    else if (lwmsg_time_compare(&now, &clock->last_time) <= LWMSG_TIME_EQUAL)
    {
        lwmsg_time_difference(&now, &clock->last_time, &diff);
        diff.microseconds++;
        lwmsg_time_sum(&clock->adjust, &diff, &clock->adjust);
    }

    clock->last_time = now;
    
error:

    return status;
}

LWMsgStatus
lwmsg_clock_get_wall_time(
    LWMsgClock* clock,
    LWMsgTime* time
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_clock_update(clock));

    *time = clock->last_time;

error:
    
    return status;
}

LWMsgStatus
lwmsg_clock_get_monotonic_time(
    LWMsgClock* clock,
    LWMsgTime* time
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_clock_update(clock));

    lwmsg_time_sum(&clock->last_time, &clock->adjust, time);

error:
    
    return status; 
}
