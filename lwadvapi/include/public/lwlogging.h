/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 *        lwlogging.h
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi) logging definitions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __LWLOGGING_H__
#define __LWLOGGING_H__

typedef enum
{
    LW_LOG_LEVEL_ALWAYS = 0,
    LW_LOG_LEVEL_ERROR,
    LW_LOG_LEVEL_WARNING,
    LW_LOG_LEVEL_INFO,
    LW_LOG_LEVEL_VERBOSE,
    LW_LOG_LEVEL_DEBUG,
    LW_LOG_LEVEL_TRACE
} LwLogLevel;

typedef VOID (*PLWLOG_CALLBACK)(LwLogLevel level, PVOID pUserData, PCSTR pszMessage);


LW_BEGIN_EXTERN_C

DWORD
LwSetLogFunction(
    IN LwLogLevel maxLevel,
    IN PLWLOG_CALLBACK pCallback,
    IN PVOID pUserData
    );

DWORD
LwLogMessage(
    IN LwLogLevel level,
    IN PCSTR pszFormat,
    ...
    );

LW_END_EXTERN_C


#endif /* __LWLOGGING_H__ */
