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
