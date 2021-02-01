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
 *        sqldb_create_schema.h
 *
 * Abstract:
 *
 *        BeyondTrust Registry
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

