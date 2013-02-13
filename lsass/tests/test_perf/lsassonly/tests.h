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
 *        tests.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test helper function declarations
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 *
 */

#ifndef LSASS_TESTS_H
#define LSASS_TESTS_H

#include "config.h"
#include <stdarg.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <lsa/lsa.h>
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include <lsautils.h>

BOOL
RunConnectDisconnect(
    IN PVOID unused
    );

typedef struct _FIND_STATE
{
    uid_t Uid;
    HANDLE Connection;
} FIND_STATE;

BOOL
SetupFindUserById(
    IN PVOID username,
    OUT PVOID *ppvFindState
    );

BOOL
RunFindUserById(
    IN PVOID pvState
    );

void
CleanupFindUserById(
    IN PVOID pvState
    );

BOOL
SetupConnectLsass(
    IN PVOID username,
    OUT PVOID *pHandle
    );

BOOL
RunGetLogLevel(
    IN PVOID handle
    );

void
CleanupConnectLsass(
    IN PVOID handle
    );

#endif
