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
 *        lwparseargs.c
 *
 * Abstract:
 *
 *        LW Parse Args Library
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "lwparseargs.h"

VOID
LwParseArgsInit(
    OUT PLW_PARSE_ARGS pParseArgs,
    IN int argc,
    IN PCSTR argv[]
    )
{
    pParseArgs->Args = argv;
    pParseArgs->Count = argc;
    pParseArgs->Index = 0;
}

int
LwParseArgsGetRemaining(
    IN PLW_PARSE_ARGS pParseArgs
    )
{
    return pParseArgs->Count - pParseArgs->Index;
}

PCSTR
LwParseArgsGetAt(
    IN PLW_PARSE_ARGS pParseArgs,
    IN int Index
    )
{
    return (Index < pParseArgs->Count) ? pParseArgs->Args[Index] : NULL;
}

int
LwParseArgsGetIndex(
    IN PLW_PARSE_ARGS pParseArgs
    )
{
    return pParseArgs->Index;
}

PCSTR
LwParseArgsGetCurrent(
    IN OUT PLW_PARSE_ARGS pParseArgs
    )
{
    return LwParseArgsGetAt(pParseArgs, pParseArgs->Index);
}

PCSTR
LwParseArgsNext(
    IN OUT PLW_PARSE_ARGS pParseArgs
    )
{
    PCSTR pszNext = LwParseArgsGetAt(pParseArgs, pParseArgs->Index + 1);
    if (pszNext)
    {
        pParseArgs->Index++;
    }
    return pszNext;
}

PCSTR
LwGetProgramName(
    IN PCSTR pszProgramPath
    )
{
    PCSTR pszProgramName = pszProgramPath;
    PCSTR pszCurrent = pszProgramName;

    while (pszCurrent[0])
    {
        if ('/' == pszCurrent[0])
        {
            pszProgramName = pszCurrent + 1;
        }
        pszCurrent++;
    }

    return pszProgramName;
}
