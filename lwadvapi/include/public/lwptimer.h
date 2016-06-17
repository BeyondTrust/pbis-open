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
 *        lwptimer.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) Memory Utilities
 *
 *        pthread timer implementation.
 */

#ifndef __LW_PTIMER_H__
#define __LW_PTIMER_H__

LW_BEGIN_EXTERN_C 

/**
 * @file lwptimer.h
 * @brief A simple pthread based timer API
 */

/**
 * @brief Defines a timer that unless cancelled, executes 
 * a specified action when the timer expires. 
 */
typedef struct _LW_TIMER {
  char *tag;

  pthread_t thread;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned short cancelled : 1;

	unsigned short delaySeconds; 
	void *actionData;
	
	int  (*pfnStart)(struct _LW_TIMER *self);
	void (*pfnAction)(void *);
	int  (*pfnCancel)(struct _LW_TIMER *self);
} LW_TIMER, *PLW_TIMER;


/**
 * @brief Initialize the timer. 
 *
 * Timers should be freed via a call to LwTimerFree().
 *
 * @param [in] tag a descriptive label displayed in log messages
 * @param [in] pfnAction the action to execute when the timer expires
 * @param [in] actionData data passed to pfnAction
 * @param [in] delaySeconds the timer delay in seconds
 * @return a pointer to the timer, or NULL
 */
PLW_TIMER LwTimerInitialize(const char *tag, void (*pfnAction)(void *), void *actionData, unsigned short delaySeconds);

/**
 * @brief Start the timer. 
 * @param [in] timer the timer 
 *
 * @return an LW status code 
 */
int LwTimerStart(PLW_TIMER timer);

/**
 * @brief Cancel the timer. 
 * @param [in] timer the timer 
 *
 * @return an LW status code 
 */
int LwTimerCancel(PLW_TIMER timer);

/**
 * @brief Free the timer. 
 *
 * n.b. Calling code must ensure the timer has either expired or been
 * cancelled.
 *
 * @param [in] timer the timer 
 */
void LwTimerFree(PLW_TIMER timer);

LW_END_EXTERN_C

#endif
