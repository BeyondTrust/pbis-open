/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (NFSSVC)
 *
 * Server Main
 *
 * Global Variables
 *
 */

#include "includes.h"

NFSSVC_RUNTIME_GLOBALS gServerInfo =
{
    .mutex                 = PTHREAD_MUTEX_INITIALIZER,
    .pRpcListenerThread    = NULL,
    .pServerBinding        = NULL,
    .pWkstaBinding         = NULL,
    .pRegistryBinding      = NULL,
    .dwStartAsDaemon       = 0,
    .logTarget             = NFSSVC_LOG_TARGET_DISABLED,
    .maxAllowedLogLevel    = NFSSVC_LOG_LEVEL_ERROR,
    .szLogFilePath         = "",
    .bProcessShouldExit    = 0,
    .dwExitCode            = 0,
    .pServerSecDesc        = NULL,
    .config                =    {
                                .mutex = PTHREAD_MUTEX_INITIALIZER,
                                .szLsaLpcSocketPath = DEFAULT_LSALPC_SOCKET_PATH
                                },
    .genericMapping        =    {
                                .GenericRead = FILE_GENERIC_READ,
                                .GenericWrite = FILE_GENERIC_WRITE,
                                .GenericExecute = FILE_GENERIC_EXECUTE,
                                .GenericAll = FILE_ALL_ACCESS
                                }
};

PNFSSVC_RUNTIME_GLOBALS gpServerInfo = &gServerInfo;
