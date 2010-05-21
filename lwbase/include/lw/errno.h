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
 * Module Name:
 *
 *        errno.h
 *
 * Abstract:
 *
 *        UNIX errno codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LW_ERRNO_H__
#define __LW_ERRNO_H__

#include <errno.h>
#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

LW_NTSTATUS
LwErrnoToNtStatus(
    LW_IN int Code
    );

LW_WINERROR
LwErrnoToWin32Error(
    LW_IN int Code
    );

LW_PCSTR
LwErrnoToName(
    LW_IN int Code
    );

LW_PCSTR
LwErrnoToDescription(
    LW_IN int Code
    );

#ifndef LW_STRICT_NAMESPACE
#define ErrnoToNtStatus(Code)       LwErrnoToNtStatus(Code)
#define ErrnoToWin32Error(Code)     LwErrnoToWin32Error(Code)
#define ErrnoToName(Code)           LwErrnoToName(Code)
#define ErrnoToDescription(Code)    LwErrnoToDescription(Code)
#endif

LW_END_EXTERN_C

#endif
