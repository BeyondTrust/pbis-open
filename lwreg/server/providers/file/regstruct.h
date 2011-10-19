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
 *        regstruct.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Private header for Likewise registry
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

typedef struct _REG_KEY_CELL {
    DWORD dwSubKeyListCellIndex;
    DWORD dwKeyNameIndex;
} REG_KEY_CELL, *PREG_KEY_CELL;

typedef struct STRING_CELL {
    DWORD dwNumBytes;
} REG_STRING_CELL, *PREG_STRING_CELL;


typedef struct _REG_CELL {
    DWORD dwCellFlags;
    DWORD dwCellType;
    DWORD dwNextCellIndex;
    DWORD dwCellTotalSize;
    DWORD dwCellUsedSize;
    union {
        KEYCELL  KeyCellData;
        VALUECELL ValueCellData;
        SUBKEYLISTCELL SubKeyCellData;
        VALUELISTCELL  ValueListCellData;
        STRINGCELL StringCellData;
    }
} REG_CELL, *PREG_CELL;
