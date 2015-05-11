/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        ntlogmacros.h
 *
 * Abstract:
 *
 *        Log Macros -- TEMPORARY (see TODO below)
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __NT_LOG_MACROS_H__
#define __NT_LOG_MACROS_H__

// TODO -- Fold IO_LOG stuff and these together somewhere...

#define LOG_LEAVE_IF_STATUS_EE(status, EE) \
    do { \
        if (EE || status) \
        { \
            LWIO_LOG_DEBUG("LEAVE_IF: -> 0x%08x (%s) (EE = %d)", status, LwNtStatusToName(status), EE); \
        } \
    } while (0)

#define LOG_LEAVE_IF_STATUS_EE_EX(status, EE, Format, ...) \
    do { \
        if (EE || status) \
        { \
            LWIO_LOG_DEBUG("LEAVE_IF: " Format " -> 0x%08x (%s) (EE = %d)", ## __VA_ARGS__, status, LwNtStatusToName(status), EE); \
        } \
    } while (0)

#endif /* __NT_LOG_MACROS_H__ */
