/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     rtllog.c
 *
 * Abstract:
 *
 *     RTL Logging
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lw/rtllog.h>

LW_RTL_LOG_CONTROL _LwRtlLogControl;

LW_VOID
LwRtlLogSetCallback(
    LW_IN LW_OPTIONAL LW_RTL_LOG_CALLBACK Callback,
    LW_IN LW_OPTIONAL LW_PVOID Context
    )
{
    _LwRtlLogControl.Callback = Callback;
    _LwRtlLogControl.Context = Context;
}

LW_VOID
LwRtlLogGetCallback(
    LW_OUT LW_OPTIONAL LW_RTL_LOG_CALLBACK *Callback,
    LW_OUT LW_OPTIONAL LW_PVOID *Context
    )
{
    if (Callback)
    {
        *Callback = _LwRtlLogControl.Callback;
    }

    if (Context)
    {
        *Context = _LwRtlLogControl.Context;
    }
}

LW_VOID
LwRtlLogSetLevel(
    LW_IN LW_RTL_LOG_LEVEL Level
    )
{
    _LwRtlLogControl.Level = Level;
}
