/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __DJDAEMONMGR_H__
#define __DJDAEMONMGR_H__

#include "djmodule.h"

void
DJGetDaemonStatus(
    PCSTR pszDaemonPath,
    PBOOLEAN pbStarted,
    LWException **exc
    );

DWORD
DJGetBaseDaemonPriorities(
    int *startPriority,
    int *stopPriority,
    int *stopLaterOffset
    );

void
DJManageDaemons(
    BOOLEAN bStart,
    LWException **exc
    );

void
DJStartStopDaemon(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    LWException **exc
    );

void DJRestartIfRunning(PCSTR daemon, LWException **exc);

extern const JoinModule DJDaemonStopModule;

extern const JoinModule DJDaemonStartModule;

struct _DaemonList
{
    PCSTR primaryName;
    PCSTR alternativeNames[3];
    BOOLEAN required;
    int startPriority;
    int stopPriority;
};

extern struct _DaemonList daemonList[];

#endif // __DJDAEMONMGR_H__
