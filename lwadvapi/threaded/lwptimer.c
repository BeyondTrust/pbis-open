/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright BeyondTrust Software
 * All rights reserved.
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
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU LESSER GENERAL
 * PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR
 * WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY
 * BEYONDTRUST, PLEASE CONTACT BEYONDTRUST AT beyondtrust.com/contact
 */


/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwptimer.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        pthread timer implementation. This
 *        uses simple pthread mechanisms as 
 *        not all pthread features are supported
 *        on all target platforms (e.g. os x 
 *        doesn't support timers).
 *
 *        Internal functions log, and return standard errno
 *        error codes. Public functions return LW error codes.
 */
#include "includes.h"
#include <lwerror.h>
#include <lw/rtlgoto.h>
#include <lw/rtllog.h>
#include <lw/rtlmemory.h>
#include <lw/rtlstring.h>
#include <lw/errno.h>


/* @brief Performs a timed wait on the timer pthread cond until the 
 * timeout expires or timer->cancelled is set.
 *
 * @return NULL
 */
static void * timer_loop(void *arg) {
    const PLW_TIMER timer = (PLW_TIMER)arg;
    struct timespec timeout = { .tv_sec = time(NULL) + timer->delaySeconds};
    int status = 0; 

    status = pthread_mutex_lock(&(timer->mutex));
    if (status) {
        LW_RTL_LOG_ERROR("Failed to lock %s timer mutex: error %s (%d); timer will NOT run.", 
                timer->tag, ErrnoToName(status), status);
        GOTO_ERROR_ON_STATUS(status);
    }

    while (!timer->cancelled) {
        status = pthread_cond_timedwait(&(timer->cond), &(timer->mutex), &timeout);

        if (status == ETIMEDOUT) {
           break;
        } else if (status != 0) {
            /* break the loop for errors which indicate programming errors 
             * so this doesn't run endlessly */ 
            if (status == EINVAL || status == EPERM) {
              LW_RTL_LOG_ERROR("Error waiting on %s timer: error %s (%d).", 
                      timer->tag, ErrnoToName(status), status);
              break;
            }
        }
    }

    if (!timer->cancelled) {
        LW_RTL_LOG_WARNING("%s timer expired, executing timer expired action.",
                timer->tag); 
        if (timer->pfnAction) {
            timer->pfnAction(timer->actionData);
        }
    }

    /* ignore unlock failures */
    pthread_mutex_unlock(&(timer->mutex));

error:
    return NULL;
}


/* Creates the timer thread 
 * 
 * This does not detach the thread as joining the
 * thread allows the free routine to know the 
 * timer thread has completed.
 */
static int timer_start_internal(PLW_TIMER timer) {
    int status = pthread_create(&timer->thread, NULL, &timer_loop, (void *)timer);
    if (status) {
        LW_RTL_LOG_ERROR("Could not create the %s timer thread: error %s (%d).",
                timer->tag, ErrnoToName(status), status);
        GOTO_ERROR_ON_STATUS(status);
    }

error:
   return status;
}

static int timer_cancel_internal(PLW_TIMER timer) {
    int status = 0;

    status = pthread_mutex_lock(&(timer->mutex));
    if (status) {
        LW_RTL_LOG_ERROR("Failed to lock mutex: error %s (%d). Can NOT cancel %s timer.", 
                ErrnoToName(status), status, timer->tag);
        GOTO_ERROR_ON_STATUS(status);
    }

    timer->cancelled = 1;

    status = pthread_mutex_unlock(&(timer->mutex));
    if (status) {
        LW_RTL_LOG_ERROR("Failed to unlock mutex: error %s (%d). Can NOT cancel %s timer.", 
                ErrnoToName(status), status, timer->tag);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = pthread_cond_signal(&(timer->cond));
    if (status) {
        LW_RTL_LOG_ERROR("Failed to signal timer cond variable: error %s (%d). Can NOT cancel %s timer.", 
                ErrnoToName(status), status, timer->tag);
        GOTO_ERROR_ON_STATUS(status);
    }

error:
    return status;
}

PLW_TIMER LwTimerInitialize(const char *tag, 
            void (*pfnAction)(void *), 
            void *actionData,
            unsigned short delaySeconds) {
    PLW_TIMER timer = NULL;
    int status = 0;

    assert(tag != NULL);
    assert(pfnAction != NULL);
    assert(delaySeconds > 0);

    timer = (PLW_TIMER)LwRtlMemoryAllocate(sizeof(LW_TIMER), TRUE);
    if (timer == NULL) {
        return NULL;
    }

    status = LwRtlCStringDuplicate(&timer->tag, tag); 
    GOTO_ERROR_ON_STATUS(status);

    status = pthread_mutex_init(&(timer->mutex),NULL);
    if (status) {
        LW_RTL_LOG_ERROR("Failed to initialize %s timer mutex: error %s (%d).", timer->tag, ErrnoToName(status), status);
        GOTO_ERROR_ON_STATUS(status);
    }

    status = pthread_cond_init(&(timer->cond), NULL);
    if (status) {
        LW_RTL_LOG_ERROR("Failed to initialize %s timer cond: error %s (%d).", timer->tag, ErrnoToName(status), status);
        GOTO_ERROR_ON_STATUS(status);
    }
    timer->cancelled = 0;
    timer->delaySeconds = delaySeconds;

    timer->pfnStart = &timer_start_internal;
    timer->pfnCancel = &timer_cancel_internal;
    timer->pfnAction = pfnAction;
    timer->actionData = actionData;

cleanup:
    return timer;

error:
    /* it's safe to ignore the pthread_*_destroy 
     * return value as its either EBUSY, or EINVAL */
    if (timer) {
        pthread_mutex_destroy(&timer->mutex);
        pthread_cond_destroy(&timer->cond);

        LwRtlCStringFree(&timer->tag);

        free(timer);
        timer = NULL;
    }

    goto cleanup;

}

void LwTimerFree(PLW_TIMER timer) {
    assert(timer != NULL);

    pthread_join(timer->thread, NULL);

    LwRtlCStringFree(&timer->tag);
    pthread_mutex_destroy(&(timer->mutex));
    pthread_cond_destroy(&(timer->cond));
    free(timer);
}

int LwTimerStart(PLW_TIMER timer) {
    assert(timer != NULL);
    return LwMapErrnoToLwError(timer->pfnStart(timer));
}

int LwTimerCancel(PLW_TIMER timer) {
    assert(timer != NULL);
    return LwMapErrnoToLwError(timer->pfnCancel(timer));
}
