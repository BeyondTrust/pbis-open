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
 *        servermain.h
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
 * 
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 * 
 */
#ifndef __SERVERMAIN_H__
#define __SERVERMAIN_H__

/* Define max number of tries LWNetStartupPreCheck() will perform */
#define STARTUP_PRE_CHECK_WAIT 12

typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
} LWNETSERVERINFO, *PLWNETSERVERINFO;

int
main(
    int argc,
    const char* argv[]
    );

DWORD
LWNetStartupPreCheck(
    VOID
    );

DWORD
LWNetSrvSetDefaults(
    VOID
    );

DWORD
LWNetSrvInitialize(
    VOID
    );

DWORD
LWNetInitCacheFolders(
    VOID
    );

DWORD
LWNetSrvGetCachePath(
    PSTR* ppszPath
    );

DWORD
LWNetSrvGetPrefixPath(
    PSTR* ppszPath
    );

VOID
LWNetSrvLogProcessStartedEvent(
    VOID
    );

VOID
LWNetSrvLogProcessStoppedEvent(
    DWORD dwExitCode
    );

VOID
LWNetSrvLogProcessFailureEvent(
    DWORD dwErrCode
    );

DWORD
LWNetGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    );

#endif /* __SERVERMAIN_H__ */

