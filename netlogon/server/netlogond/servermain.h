/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        servermain.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
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

