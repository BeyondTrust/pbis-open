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


#define REG_DB_INSERT_REG_SCHEMA_KEY "INSERT INTO " REG_DB_TABLE_NAME_SCHEMA_KEYS " (" \
                                     "ParentId," \
                                     "KeyName," \
                                     "AclIndex," \
                                     "LastUpdated) " \
                                     "VALUES (?1,?2,?3,?4)" \


#if 0
                                ""

#define REG_DB_INSERT_REG_VALUE "INSERT INTO " REG_DB_TABLE_NAME_VALUES " (" \
                                "ParentId," \
                                "ValueName," \
                                "Type," \
                                "Value," \
                                "LastUpdated) " \
                                "VALUES (?1,?2,?3,?4,?5)" \
                                ""

#define REG_DB_INSERT_REG_ACL "INSERT INTO " REG_DB_TABLE_NAME_ACLS " (" \
                                "Acl) " \
                                "VALUES (?1)" \
                                ""

#define REG_DB_UPDATE_REG_ACL "update " REG_DB_TABLE_NAME_ACLS " set " \
                        "Acl = ?1 " \
                        "where CacheId = ?2 " \

#define REG_DB_UPDATE_REG_VALUE "update " REG_DB_TABLE_NAME_VALUES " set " \
	                    "LastUpdated = ?1, " \
                        "Value = ?2 " \
                        "where ParentId = ?3 and ValueName = ?4 " \

#define REG_DB_UPDATE_KEY_ACL_INDEX_BY_KEYID  "update " REG_DB_TABLE_NAME_KEYS " set " \
	                    "AclIndex = ?1 " \
                        "where CacheId = ?2 " \

#define REG_DB_OPEN_KEY_EX "select " \
            REG_DB_TABLE_NAME_KEYS ".CacheId, " \
            REG_DB_TABLE_NAME_KEYS ".LastUpdated, " \
            REG_DB_TABLE_NAME_KEYS ".ParentId, " \
            REG_DB_TABLE_NAME_KEYS ".KeyName, " \
            REG_DB_TABLE_NAME_KEYS ".AclIndex " \
            "from " REG_DB_TABLE_NAME_KEYS " " \
            "where " REG_DB_TABLE_NAME_KEYS ".KeyName = ?1 " \
            		"AND " REG_DB_TABLE_NAME_KEYS ".ParentId = ?2 " \

#define REG_DB_QUERY_KEY_ACL_INDEX_BY_KEYID  "select " \
	        REG_DB_TABLE_NAME_KEYS ".AclIndex " \
			"from " REG_DB_TABLE_NAME_KEYS " " \
			"where " REG_DB_TABLE_NAME_KEYS ".CacheId = ?1 " \

#define REG_DB_QUERY_KEYVALUE  "select " \
	        REG_DB_TABLE_NAME_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_VALUES ".Type, " \
            REG_DB_TABLE_NAME_VALUES ".Value, " \
            REG_DB_TABLE_NAME_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_VALUES ".ParentId = ?2 " \

#define REG_DB_QUERY_KEYVALUE_WITHTYPE  "select " \
	        REG_DB_TABLE_NAME_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_VALUES ".Type, " \
            REG_DB_TABLE_NAME_VALUES ".Value, " \
            REG_DB_TABLE_NAME_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_VALUES ".ParentId = ?2 " \
                    "AND " REG_DB_TABLE_NAME_VALUES ".Type = ?3" \


#define REG_DB_QUERY_KEYVALUE_WITHWRONGTYPE "select " \
	        REG_DB_TABLE_NAME_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_VALUES ".Type, " \
            REG_DB_TABLE_NAME_VALUES ".Value, " \
            REG_DB_TABLE_NAME_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ValueName = ?1 " \
                    "AND " REG_DB_TABLE_NAME_VALUES ".ParentId = ?2 " \
                    "AND " REG_DB_TABLE_NAME_VALUES ".Type != ?3" \


#define REG_DB_QUERY_SUBKEY_COUNT  "select COUNT (*) as subkeyCount " \
			"from " REG_DB_TABLE_NAME_KEYS " " \
			"where " REG_DB_TABLE_NAME_KEYS ".ParentId = ?1 " \

#define REG_DB_QUERY_ACL_REFCOUNT  "select COUNT (*) as aclrefCount " \
			"from " REG_DB_TABLE_NAME_KEYS " " \
			"where " REG_DB_TABLE_NAME_KEYS ".AclIndex = ?1 " \
              "AND " REG_DB_TABLE_NAME_KEYS ".CacheId != ?2 " \

#define REG_DB_QUERY_SUBKEYS       "select " \
            REG_DB_TABLE_NAME_KEYS ".CacheId, " \
            REG_DB_TABLE_NAME_KEYS ".LastUpdated, " \
            REG_DB_TABLE_NAME_KEYS ".ParentId, " \
            REG_DB_TABLE_NAME_KEYS ".KeyName, " \
            REG_DB_TABLE_NAME_KEYS ".AclIndex " \
            "from " REG_DB_TABLE_NAME_KEYS " " \
            "where " REG_DB_TABLE_NAME_KEYS ".ParentId = ?1 " \
                    "LIMIT ?2 OFFSET ?3" \

#define REG_DB_QUERY_VALUE_COUNT   "select COUNT (*) as valueCount " \
			"from " REG_DB_TABLE_NAME_VALUES " " \
			"where " REG_DB_TABLE_NAME_VALUES ".ParentId = ?1 " \

#define REG_DB_DELETE_KEY "delete from "  REG_DB_TABLE_NAME_KEYS " " \
            "where " REG_DB_TABLE_NAME_KEYS ".CacheId = ?1" \
              "AND " REG_DB_TABLE_NAME_KEYS ".KeyName = ?2 " \

#define REG_DB_DELETE_KEY_VALUES "delete from "  REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ParentId = ?1" \

#define REG_DB_QUERY_VALUES         "select " \
            REG_DB_TABLE_NAME_VALUES ".ParentId, " \
            REG_DB_TABLE_NAME_VALUES ".ValueName, " \
            REG_DB_TABLE_NAME_VALUES ".Type, " \
            REG_DB_TABLE_NAME_VALUES ".Value, " \
            REG_DB_TABLE_NAME_VALUES ".LastUpdated " \
            "from " REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ParentId = ?1 " \
                    "LIMIT ?2 OFFSET ?3" \

#define REG_DB_DELETE_KEYVALUE  "delete from "  REG_DB_TABLE_NAME_VALUES " " \
            "where " REG_DB_TABLE_NAME_VALUES ".ParentId = ?1" \
            		"AND " REG_DB_TABLE_NAME_VALUES ".ValueName = ?2" \


#define REG_DB_DELETE_ACL  "delete from "  REG_DB_TABLE_NAME_ACLS " " \
            "where " REG_DB_TABLE_NAME_ACLS ".CacheId = ?1" \

#define REG_DB_QUERY_KEY_ACL_INDEX  "select " \
	        REG_DB_TABLE_NAME_ACLS ".CacheId " \
			"from " REG_DB_TABLE_NAME_ACLS " " \
			"where " REG_DB_TABLE_NAME_ACLS ".Acl = ?1 " \

#define REG_DB_QUERY_KEY_ACL  "select " \
	        REG_DB_TABLE_NAME_ACLS ".Acl " \
			"from " REG_DB_TABLE_NAME_ACLS " " \
			"where " REG_DB_TABLE_NAME_ACLS ".CacheId = ?1 " \

#define REG_DB_QUERY_TOTAL_ACL_COUNT   "select COUNT (*) as totalAclCount " \
            "from " REG_DB_TABLE_NAME_ACLS " " \

#define REG_DB_QUERY_KEY_ACL_BY_OFFSET  "select " \
            REG_DB_TABLE_NAME_ACLS ".CacheId, " \
            REG_DB_TABLE_NAME_ACLS ".Acl " \
            "from " REG_DB_TABLE_NAME_ACLS " " \
            "LIMIT 1 OFFSET ?1" \

#endif

#endif /* __SQLCACHE_CREATE_SCHEMA_H__ */

