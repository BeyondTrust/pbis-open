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
 *        sqldb_create_schema.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Sqlite registry backend
 *        Registry schema Related SQL statement
 *
 * Authors:
 *          Wei Fu (wfu@likewise.com)
 */
#ifndef __SQLCACHE_CREATE_SCHEMA_H__
#define __SQLCACHE_CREATE_SCHEMA_H__


#define REG_DB_INSERT_REG_VALUE_ATTRIBUTES "INSERT INTO " REG_DB_TABLE_NAME_SCHEMA_VALUES " (" \
                                "ParentId," \
                                "ValueName," \
                                "Type," \
                                "DefaultValue," \
                                "Document," \
                                "RangeType," \
                                "Hint," \
                                "Range," \
                                "LastUpdated) " \
                                "VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9)" \
                                ""


#define REG_DB_QUERY_VALUE_ATTRIBUTES  "select " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".DefaultValue, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Document, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".RangeType, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Hint, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Range, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
            "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?2 " \


#define REG_DB_QUERY_VALUE_ATTRIBUTES_WITHTYPE  "select " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".DefaultValue, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Document, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".RangeType, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Hint, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Range, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
            "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?2 " \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type = ?3" \


#define REG_DB_QUERY_VALUE_ATTRIBUTES_WITHWRONGTYPE "select " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".DefaultValue, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Document, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".RangeType, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Hint, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Range, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
            "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?2 " \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type != ?3" \


#define REG_DB_UPDATE_VALUE_ATTRIBUTES "update " REG_DB_TABLE_NAME_SCHEMA_VALUES " set " \
                        "LastUpdated = ?1, " \
                        "DefaultValue = ?2, " \
                        "Document = ?3, " \
                        "RangeType = ?4, " \
                        "Hint = ?5, " \
                        "Range = ?6 " \
                        "where ParentId = ?7 and ValueName = ?8 " \

#define REG_DB_DELETE_VALUE_ATTRIBUTES "delete from "  REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
                    "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?1" \
                    "AND " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName = ?2" \

#define REG_DB_DELETE_ALL_VALUE_ATTRIBUTES "delete from "  REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
            "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?1" \


#define RED_DB_QUERY_DEFAULT_VALUE_COUNT "select COUNT (*) as DefaultvalueCount " \
                "from " REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
                "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?1 " \
                        "AND ValueName NOT IN (select " REG_DB_TABLE_NAME_VALUES ".ValueName " \
                                               "from " REG_DB_TABLE_NAME_VALUES \
                                               " where ParentId = ?2) " \


#define REG_DB_QUERY_DEFAULT_VALUES "select " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Type, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".DefaultValue, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Document, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".RangeType, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Hint, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".Range, " \
            REG_DB_TABLE_NAME_SCHEMA_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_SCHEMA_VALUES " " \
            "where " REG_DB_TABLE_NAME_SCHEMA_VALUES ".ParentId = ?1 " \
                        "AND ValueName NOT IN (select " REG_DB_TABLE_NAME_VALUES ".ValueName " \
                                               "from " REG_DB_TABLE_NAME_VALUES \
                                               " where ParentId = ?2) " \
                    "LIMIT ?3 OFFSET ?4" \


#endif /* __SQLCACHE_CREATE_SCHEMA_H__ */

