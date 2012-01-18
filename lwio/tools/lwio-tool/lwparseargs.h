/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lwparseargs.h
 *
 * Abstract:
 *
 *        LW Parse Args Library
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __LW_PARSE_ARGS_H__
#define __LW_PARSE_ARGS_H__

#include <lw/base.h>

typedef struct _LW_PARSE_ARGS {
    PCSTR* Args;
    int Count;
    int Index;
} LW_PARSE_ARGS, *PLW_PARSE_ARGS;

VOID
LwParseArgsInit(
    OUT PLW_PARSE_ARGS pParseArgs,
    IN int argc,
    IN PCSTR argv[]
    );

int
LwParseArgsGetRemaining(
    IN PLW_PARSE_ARGS pParseArgs
    );

PCSTR
LwParseArgsGetAt(
    IN PLW_PARSE_ARGS pParseArgs,
    IN int Index
    );

int
LwParseArgsGetIndex(
    IN PLW_PARSE_ARGS pParseArgs
    );

PCSTR
LwParseArgsGetCurrent(
    IN OUT PLW_PARSE_ARGS pParseArgs
    );

PCSTR
LwParseArgsNext(
    IN OUT PLW_PARSE_ARGS pParseArgs
    );

PCSTR
LwGetProgramName(
    IN PCSTR pszProgramPath
    );

#endif /* __LW_PARSE_ARGS_H__ */
