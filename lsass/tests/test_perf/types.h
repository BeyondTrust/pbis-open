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
 *        types.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Declarations of types
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 *
 */

#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

typedef void *PVOID;
typedef unsigned int BOOL;
typedef char *PSTR;

#define IN
#define OUT

#define FALSE 0
#define TRUE 1

typedef BOOL (*TestSetupFunc)(
        IN PVOID setupArg,
        OUT PVOID *runArg
        );
typedef BOOL (*TestTimedFunc)(IN PVOID arg);
typedef BOOL (*TestCleanupFunc)(IN PVOID arg);

typedef enum
{
    TEST_TYPE_RUNS_PER_SEC,
    TEST_TYPE_SINGLE_RUN,
} TestType;

typedef struct
{
    PSTR description;
    TestType type;
    TestSetupFunc setup;
    TestTimedFunc run;
    TestCleanupFunc cleanup;
    PVOID setupArg;
} PerfTest;

#endif
