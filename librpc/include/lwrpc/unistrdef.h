/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Abstract: UnicodeString definitions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#ifndef _UNISTRDEF_H_
#define _UNISTRDEF_H_

// TODO-Use types from <lw/types.h>
// For (u)int<N> types.
#include <lwrpc/types.h>

typedef struct unicode_string {
    UINT16 len;
    UINT16 size;
#ifdef _DCE_IDL_
    [size_is(size/2),length_is(len/2)]
#endif
    wchar16_t *string;
} UnicodeString;


typedef struct unicode_string_ex {
    UINT16 len;
    UINT16 size;   /* size = len + 1 (for terminating char) */
#ifdef _DCE_IDL_
    [size_is(size/2),length_is(len/2)]
#endif
    wchar16_t *string;
} UnicodeStringEx;


typedef struct entry {
    UINT32 idx;
    UnicodeString name;
} Entry;

typedef struct entry_array {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    Entry *entries;
} EntryArray;


typedef struct unicode_string_array {
    UINT32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    UnicodeString *names;
} UnicodeStringArray;

#endif /* _UNISTRDEF_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
