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
 *        evtfwd-logging.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *        Logging interface
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __UMN_LOGGING_H__
#define __UMN_LOGGING_H__

#include <lw/rtllog.h>

#define UMN_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define SERVICE_NAME "usermonitor"

#define UMN_LOG_ALWAYS(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_ALWAYS, SERVICE_NAME, ## __VA_ARGS__)

#define UMN_LOG_ERROR(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_ERROR, SERVICE_NAME, ## __VA_ARGS__)

#define UMN_LOG_WARNING(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_WARNING, SERVICE_NAME, ## __VA_ARGS__)

#define UMN_LOG_INFO(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_ALWAYS, SERVICE_NAME, ## __VA_ARGS__)

#define UMN_LOG_VERBOSE(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_VERBOSE, SERVICE_NAME, ## __VA_ARGS__)

#define UMN_LOG_DEBUG(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_DEBUG, SERVICE_NAME, ## __VA_ARGS__)


#endif /* __UMN_LOGGING_H__ */
