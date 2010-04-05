/*
 * Copyright (c) 2008, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <uipc/time.h>
#include <sys/time.h>
#include <time.h>

static void
uipc_time_normalize(uipc_time* time)
{
    /* Ensure microseconds are within -10^6 and 10^6 exclusive */
    if (time->microseconds < 0)
    {
        while (time->microseconds <= -1000000)
        {
            time->microseconds += 1000000;
            time->seconds--;
        }
    }
    else
    {
        while (time->microseconds >= 1000000)
        {
            time->microseconds -= 1000000;
            time->seconds++;
        }
    }

    /* Ensure both values have same sign or are zero */
    if (time->seconds > 0 && time->microseconds < 0)
    {
        time->seconds--;
        time->microseconds += 1000000;
    }

    if (time->seconds < 0 && time->microseconds > 0)
    {
        time->seconds++;
        time->microseconds -= 1000000;
    }
}

void
uipc_time_current(uipc_time* time)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    time->seconds = (unsigned long) tv.tv_sec;
    time->microseconds = (unsigned long) tv.tv_usec;
}

void
uipc_time_current_offset(uipc_time* time, long s, long us)
{
    uipc_time_current(time);
    
    time->seconds += s;
    time->microseconds += us;

    uipc_time_normalize(time);
}

int
uipc_time_is_past(uipc_time* time)
{
    uipc_time now;

    uipc_time_current(&now);

    return (now.seconds > time->seconds ||
            (now.seconds == time->seconds &&
             now.microseconds > time->microseconds));
}

void
uipc_time_difference(uipc_time* from, uipc_time* to, uipc_time* res)
{
    res->seconds = to->seconds - from->seconds;
    res->microseconds = to->microseconds - from->microseconds;

    uipc_time_normalize(res);
}

