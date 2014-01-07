/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Event Log
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
