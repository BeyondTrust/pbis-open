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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rsys-ipc.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __RSYS_IPC_H__
#define __RSYS_IPC_H__

#include <lwmsg/lwmsg.h>

#include "rsys-utils.h"

typedef struct _RSYS_IPC_ERROR
{
    DWORD dwError;
    PCSTR pszErrorMessage;
} RSYS_IPC_ERROR, *PRSYS_IPC_ERROR;

typedef enum _RSYS_IPC_TAG
{
    RSYS_Q_SET_LOGINFO,
    RSYS_R_SET_LOGINFO_SUCCESS,
    RSYS_R_SET_LOGINFO_FAILURE,
    RSYS_Q_GET_LOGINFO,
    RSYS_R_GET_LOGINFO_SUCCESS,
    RSYS_R_GET_LOGINFO_FAILURE,
} RSYS_IPC_TAG;

LWMsgProtocolSpec*
RSysIPCGetProtocolSpec(
    void
    );

#define MAP_LWMSG_ERROR(_e_) ((_e_) ? -1 : 0)
#define MAP_RSYS_ERROR_IPC(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

#endif /*__RSYS_IPC_H__*/
