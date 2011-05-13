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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        error.h
 *
 * Abstract:
 *
 *        Error methods.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 30, 2010
 *
 */

#ifndef _ADTOOL_ERROR_H_
#define _ADTOOL_ERROR_H_

#define ADT_ERR_MSG_UNKNOWN   "Unknown ADT error"

#define ADT_ERR_BASE                        400000
#define ADT_ERR_RANGE                       99999

#define ADT_LDAP_ERR_BASE                   500000
#define ADT_LDAP_ERR_RANGE                  99999

#define ADT_KRB5_ERR_BASE                   600000
#define ADT_KRB5_ERR_RANGE                  99999

#define ADT_WIN_ERR_BASE                    700000
#define ADT_WIN_ERR_RANGE                   99999

#define ADT_ERR_INVALID_USAGE               ADT_ERR_BASE + 0
#define ADT_ERR_INVALID_ARG                 ADT_ERR_BASE + 1
#define ADT_ERR_INVALID_ACTION              ADT_ERR_BASE + 2
#define ADT_ERR_INVALID_PROP_DATA           ADT_ERR_BASE + 3
#define ADT_ERR_INVALID_SEARCH_SCOPE        ADT_ERR_BASE + 4
#define ADT_ERR_INVALID_FUN_ARG             ADT_ERR_BASE + 5
#define ADT_ERR_INVALID_OBJECTGUID          ADT_ERR_BASE + 6
#define ADT_ERR_INVALID_GROUP_SCOPE         ADT_ERR_BASE + 7
#define ADT_ERR_INVALID_USER_GROUP          ADT_ERR_BASE + 8
#define ADT_ERR_INVALID_PARENT_DN           ADT_ERR_BASE + 9
#define ADT_ERR_INVALID_SID                 ADT_ERR_BASE + 10
#define ADT_ERR_INVALID_ARG_USER_COMPUTER   ADT_ERR_BASE + 11
#define ADT_ERR_INVALID_UID_UNAME           ADT_ERR_BASE + 12

#define ADT_ERR_ACTION_MISSING              ADT_ERR_BASE + 20
#define ADT_ERR_ACTION_NOT_SUPPORTED        ADT_ERR_BASE + 21
#define ADT_ERR_ACTION_INVALID_STATE        ADT_ERR_BASE + 22

#define ADT_ERR_FAILED_AD_OPEN              ADT_ERR_BASE + 30
#define ADT_ERR_FAILED_AD_BIND              ADT_ERR_BASE + 31
#define ADT_ERR_FAILED_AD_SET_VER           ADT_ERR_BASE + 32
#define ADT_ERR_FAILED_AD_SET_REF           ADT_ERR_BASE + 33
#define ADT_ERR_FAILED_AD_SET_GSS           ADT_ERR_BASE + 34
#define ADT_ERR_FAILED_AD_SET_OPT           ADT_ERR_BASE + 35
#define ADT_ERR_FAILED_AD_GET_ROOT_DSE      ADT_ERR_BASE + 36
#define ADT_ERR_FAILED_AD_GET_DEFAULT_NC    ADT_ERR_BASE + 37
#define ADT_ERR_FAILED_AD_LOGON_DN          ADT_ERR_BASE + 38
#define ADT_ERR_FAILED_AD_OBJ_NOT_FOUND     ADT_ERR_BASE + 39
#define ADT_ERR_FAILED_AD_MULTIPLE_FOUND    ADT_ERR_BASE + 40
#define ADT_ERR_FAILED_AD_GET_ATTR          ADT_ERR_BASE + 41
#define ADT_ERR_FAILED_AD_GET_GUID          ADT_ERR_BASE + 42
#define ADT_ERR_FAILED_AD_GET_SID           ADT_ERR_BASE + 43
#define ADT_ERR_FAILED_AD_GET_LDAP_SPN      ADT_ERR_BASE + 44
#define ADT_ERR_FAILED_AD_SET_SIZE_LIMIT    ADT_ERR_BASE + 45
#define ADT_ERR_FAILED_AD_USER_NOT_FOUND    ADT_ERR_BASE + 46
#define ADT_ERR_FAILED_AD_GROUP_NOT_FOUND   ADT_ERR_BASE + 47
#define ADT_ERR_FAILED_POPT_CTX             ADT_ERR_BASE + 48
#define ADT_ERR_FAILED_ALLOC                ADT_ERR_BASE + 49
#define ADT_ERR_FAILED_TO_LOCATE_AD         ADT_ERR_BASE + 50
#define ADT_ERR_FAILED_TO_LOCATE_DOMAIN     ADT_ERR_BASE + 51
#define ADT_ERR_FAILED_AD_GET_LDAP_GC_READY ADT_ERR_BASE + 52
#define ADT_ERR_FAILED_GTYPE_CHECK          ADT_ERR_BASE + 53

#define ADT_ERR_ARG_MISSING_PROP            ADT_ERR_BASE + 60
#define ADT_ERR_ARG_MISSING_CELL_PROP       ADT_ERR_BASE + 61
#define ADT_ERR_ARG_MISSING_DN              ADT_ERR_BASE + 62
#define ADT_ERR_ARG_MISSING_CELL            ADT_ERR_BASE + 63
#define ADT_ERR_ARG_MISSING_USER_GROUP      ADT_ERR_BASE + 64
#define ADT_ERR_ARG_MISSING_USER            ADT_ERR_BASE + 65
#define ADT_ERR_ARG_MISSING_GROUP           ADT_ERR_BASE + 66
#define ADT_ERR_ARG_MISSING_NAME            ADT_ERR_BASE + 67
#define ADT_ERR_ARG_MISSING_FILTER          ADT_ERR_BASE + 68
#define ADT_ERR_ARG_MISSING_FROM            ADT_ERR_BASE + 69
#define ADT_ERR_ARG_MISSING_TO              ADT_ERR_BASE + 70
#define ADT_ERR_ARG_MISSING_SOURCE_DN       ADT_ERR_BASE + 71
#define ADT_ERR_ARG_MISSING_TARGET_DN       ADT_ERR_BASE + 72
#define ADT_ERR_ARG_MISSING_CN              ADT_ERR_BASE + 73
#define ADT_ERR_ARG_MISSING_CREDS_FOR_BIND  ADT_ERR_BASE + 74
#define ADT_ERR_ARG_MISSING_UPN_FOR_KEYTAB  ADT_ERR_BASE + 75
#define ADT_ERR_ARG_MISSING_PASSWD          ADT_ERR_BASE + 76
#define ADT_ERR_ARG_MISSING_TO_GROUP        ADT_ERR_BASE + 77
#define ADT_ERR_ARG_MISSING_FROM_GROUP      ADT_ERR_BASE + 78

#define ADT_ERR_OBJECT_NOT_EMPTY            ADT_ERR_BASE + 80
#define ADT_ERR_MULTIPLE_USERS              ADT_ERR_BASE + 81
#define ADT_ERR_MULTIPLE_GROUPS             ADT_ERR_BASE + 82
#define ADT_ERR_ASSERTION                   ADT_ERR_BASE + 83
#define ADT_ERR_MEXCL_ARGS_PWD_NEVER_EXP    ADT_ERR_BASE + 84
#define ADT_ERR_MEXCL_ARGS_PWD_CANNOT_CH    ADT_ERR_BASE + 85
#define ADT_ERR_MULTIPLE_COMPUTERS          ADT_ERR_BASE + 86

#define ADT_ERR_AUTH_FAILED                 ADT_ERR_BASE + 90

#endif /* _ADTOOL_ERROR_H_ */
