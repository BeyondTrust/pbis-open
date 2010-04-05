/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
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

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

extern pthread_t gSignalHandlerThread;
extern PVOID     pgSignalHandlerThread;
extern LWMGMTSERVERINFO gServerInfo;

#define LWMGMT_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define LWMGMT_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)

#define DEFAULT_CONFIG_FILE_PATH CONFIGDIR "/lwmgmtd.conf"

#define DAEMON_NAME "lwmgmtd"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

#endif /* __SERVER_EXTERNS_H__ */
