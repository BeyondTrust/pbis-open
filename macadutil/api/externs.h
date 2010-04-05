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
 *       Mac Workgroup Manager
 * 
 *       AD Utility API
 * 
 *       External Variables
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __EXTERNS_H__
#define __EXTERNS_H__

#define MACADUTIL_KRB5_CACHEPATH LWDS_ADMIN_CACHE_DIR "/krb5_cc_lwedsplugin"

extern MACADUTIL_FUNC_TABLE gpfnTable;
extern time_t gdwKrbTicketExpiryTime;
extern const double gdwExpiryGraceSeconds;

extern pthread_mutex_t g_ADULock;

#define ENTER_KRB5_LOCK(bInLock)              \
        if (!bInLock) {                       \
           pthread_mutex_lock(&g_ADULock);    \
           bInLock = TRUE;                    \
        }

#define LEAVE_KRB5_LOCK(bInLock)              \
        if (bInLock) {                        \
           pthread_mutex_unlock(&g_ADULock);  \
           bInLock = FALSE;                   \
        }

#endif /* __EXTERNS_H__ */
