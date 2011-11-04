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
 *        rtlgoto.h
 *
 * Abstract:
 *
 *        RTL goto macros
 *
 *        GOTO_CLEANUP<XXX> macros used to bail.  Logging is done separately
 *        by using a local EE (early exit) variable (of type int),
 *        which contains the line nubmer of the GOTO_CLEANUP<XXX>_EE
 *        operation.
 *
 *        The reason for GOTO_CLEANUP() is so that you can search
 *        the code for GOTO_CLEANUP and find all instances of going
 *        to the cleanup label (regardless of which GOTO_CLEANUP<XXX>
 *        macro was used.
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __RTL_GOTO_H__
#define __RTL_GOTO_H__

#define GOTO_CLEANUP() \
  do { goto cleanup; } while (0)

#define GOTO_CLEANUP_EE(EE) \
  do { (EE) = __LINE__; goto cleanup; } while (0)

#define _GOTO_CLEANUP_ON_NONZERO(value) \
    do { if (value) goto cleanup; } while (0)

#define _GOTO_CLEANUP_ON_NONZERO_EE(value, EE) \
      do { if (value) { (EE) = __LINE__; goto cleanup; } } while (0)

#define GOTO_CLEANUP_ON_STATUS(status) \
    _GOTO_CLEANUP_ON_NONZERO(status)

#define GOTO_CLEANUP_ON_STATUS_EE(status, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(status, EE)

#define GOTO_CLEANUP_ON_ERRNO(error) \
    _GOTO_CLEANUP_ON_NONZERO(error)

#define GOTO_CLEANUP_ON_ERRNO_EE(error, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(error, EE)

#define GOTO_CLEANUP_ON_WINERROR(winError) \
    _GOTO_CLEANUP_ON_NONZERO(winError)

#define GOTO_CLEANUP_ON_WINERROR_EE(winError, EE) \
    _GOTO_CLEANUP_ON_NONZERO_EE(winError, EE)

#define GOTO_ERROR_ON_STATUS(status) \
    do { if ((status)) goto error; } while (0)

#endif /* __RTL_GOTO_H__ */
