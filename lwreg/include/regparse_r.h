/*
 * Copyright Likewise Software    2004-2009
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
 *        regparse.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#ifndef REGPARSE_R_H
#define REGPARSE_R_H


typedef struct _REG_PARSE_ITEM
{
    /* 
     * Use these values below when "type" != REG_ATTRIBUTES.
     * When type == REG_ATTRIBUTES, these values are not valid,
     * and regAttributeEntry should be used.
     */
    REG_DATA_TYPE type;       /* Type of value name */
    REG_DATA_TYPE valueType;  /* Type of data value */
    PSTR keyName;
    PSTR valueName;
    DWORD lineNumber;
    void *value;
    DWORD valueLen;
    DWORD status;             /* status of data consistency check */

    /* valid when type = REG_ATTRIBUTES. */
    LWREG_VALUE_ATTRIBUTES regAttr;
} REG_PARSE_ITEM, *PREG_PARSE_ITEM;

#endif /* __REGPARSE_R_H__ */
