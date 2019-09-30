/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        externs.h
 *
 * Abstract:
 *
 *        BeyondTrust Event Log
 * 
 *        Server External Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __SERVER_EXTERNS_H__
#define __SERVER_EXTERNS_H__

extern pthread_rwlock_t g_dbLock;
extern DWORD gdwNewEventCount;

#define ENTER_RW_READER_LOCK(inLock) \
    do \
    { \
        if (!inLock) \
        { \
            int thr_err = pthread_rwlock_rdlock(&g_dbLock); \
            if (thr_err)\
            { \
                EVT_LOG_ERROR("Failed to acquire shared lock on global rwmutex.  Aborting program....\n"); \
                abort(); \
            } \
            inLock = TRUE; \
        } \
    } \
    while (FALSE)

#define LEAVE_RW_READER_LOCK(inLock) \
    do \
    { \
        if (inLock) \
        { \
            int thr_err = pthread_rwlock_unlock(&g_dbLock);       \
            if (thr_err)\
            { \
                EVT_LOG_ERROR("Failed to release shared lock on global rwmutex.  Aborting program....\n"); \
                abort(); \
            } \
            inLock = FALSE; \
        } \
    } \
    while (FALSE)

#define ENTER_RW_WRITER_LOCK(inLock) \
    do \
    { \
        if (!inLock) \
        { \
            int thr_err = pthread_rwlock_wrlock(&g_dbLock);       \
            if (thr_err)\
            { \
                EVT_LOG_ERROR("Failed to acquire exclusive lock on global rwmutex.  Aborting program....\n"); \
                abort(); \
            } \
            inLock = TRUE; \
        } \
    } \
    while (FALSE)

#define LEAVE_RW_WRITER_LOCK(inLock) \
    do \
    { \
        if (inLock) \
        { \
            int thr_err = pthread_rwlock_unlock(&g_dbLock);       \
            if (thr_err)\
            { \
                EVT_LOG_ERROR("Failed to release exclusive lock on global rwmutex.  Aborting program....\n"); \
                abort(); \
            } \
            inLock = FALSE; \
        } \
    } \
    while (FALSE)

#define EVENTLOG_DB_DIR CACHEDIR "/db"
#define EVENTLOG_DB EVENTLOG_DB_DIR "/lwi_events.db"

#define DEFAULT_CONFIG_FILE_PATH CONFIGDIR "/eventlogd.conf"

#define EVT_DEFAULT_MAX_LOG_SIZE    104857600 //100M,converting it in to bytes
#define EVT_DEFAULT_MAX_RECORDS     100000	//100k,converting it 100 * 1000
#define EVT_DEFAULT_MAX_AGE         90 //days
#define EVT_DEFAULT_PURGE_INTERVAL  1 //days
#define EVT_MAINTAIN_EVENT_COUNT  50
#define EVT_DEFAULT_BOOL_REMOVE_RECORDS_AS_NEEDED TRUE
#define EVT_DEFAULT_BOOL_REGISTER_TCP_IP FALSE

#endif /* __SERVER_EXTERNS_H__ */
