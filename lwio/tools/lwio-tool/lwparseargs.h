/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
