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
 *        main.c
 *
 * Abstract:
 *
 *        BeyondTrust SMB Subsystem
 *
 *        External Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_P_H__
#define __EXTERNS_P_H__

extern SMBSERVERINFO  gLwioServerInfo;
extern PSMBSERVERINFO gpLwioServerInfo;

extern pthread_t gSignalHandlerThread;
extern pthread_t* gpSignalHandlerThread;

#define DAEMON_NAME "lwiod"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

#define LWIO_LOCK_SERVERINFO(bInLock)                  \
        if (!bInLock) {                               \
           pthread_mutex_lock(gpLwioServerInfo->pLock);   \
           bInLock = TRUE;                            \
        }

#define LWIO_UNLOCK_SERVERINFO(bInLock)                \
        if (bInLock) {                                \
           pthread_mutex_unlock(gpLwioServerInfo->pLock); \
           bInLock = FALSE;                           \
        }

extern LWIO_CONFIG gLwioServerConfig;


#endif /* __EXTERNS_P_H__ */

