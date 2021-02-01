/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwptimer.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) Memory Utilities
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
PLW_TIMER LwTimerInitialize(const char *tag, void (*pfnAction)(void *), void *actionData, const unsigned short delaySeconds);

/**
 * @brief Start the timer. 
 * @param [in] timer the timer 
 *
 * @return an LW status code 
 */
int LwTimerStart(const PLW_TIMER timer);

/**
 * @brief Cancel the timer. 
 * @param [in] timer the timer 
 *
 * @return an LW status code 
 */
int LwTimerCancel(const PLW_TIMER timer);

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
