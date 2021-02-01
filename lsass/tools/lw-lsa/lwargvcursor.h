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
 *        lwargvcursor.h
 *
 * Abstract:
 *
 *        Argv cursor for argv parsing.
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#ifndef __LW_ARGVCURSOR_H__
#define __LW_ARGVCURSOR_H__

#include <lw/types.h>
#include <lw/attrs.h>

typedef struct _LW_ARGV_CURSOR {
    PCSTR* Argv;
    ULONG Count;
    ULONG Index;
} LW_ARGV_CURSOR, *PLW_ARGV_CURSOR;

static
inline
VOID
LwArgvCursorInit(
    OUT PLW_ARGV_CURSOR Cursor,
    IN int argc,
    IN PCSTR argv[]
    )
{
    Cursor->Argv = argv;
    Cursor->Count = (argc >= 0) ? (ULONG) argc : 0;
    Cursor->Index = 0;
}

static
inline
PCSTR
_LwArgvCursorGetAt(
    IN PLW_ARGV_CURSOR Cursor,
    IN ULONG Index
    )
{
    PCSTR arg = NULL;

    if (Index < Cursor->Count)
    {
        arg = Cursor->Argv[Index];
    }

    return arg;
}

static
inline
PCSTR
LwArgvCursorProgramName(
    IN PLW_ARGV_CURSOR Cursor
    )
{
    return _LwArgvCursorGetAt(Cursor, 0);
}

static
inline
PCSTR
LwArgvCursorPeek(
    IN PLW_ARGV_CURSOR Cursor
    )
{
    return _LwArgvCursorGetAt(Cursor, Cursor->Index);
}

static
inline
PCSTR
LwArgvCursorPop(
    IN OUT PLW_ARGV_CURSOR Cursor
    )
{
    PCSTR arg = LwArgvCursorPeek(Cursor);

    if (arg)
    {
        Cursor->Index++;
    }

    return arg;
}

static
inline
ULONG
LwArgvCursorRemaining(
    IN PLW_ARGV_CURSOR Cursor
    )
{
    return Cursor->Count - Cursor->Index;
}

#endif // __LW_ARGV_CURSOR_H__
