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
 *        error.c
 *
 * Abstract:
 *
 *        Error functions.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Mar 30, 2010
 *
 */

#include "includes.h"

typedef struct AdtErrorMap {
    DWORD code;
    PCSTR msg;
} AdtErrorMapT;

static AdtErrorMapT AdtErrorMap[] = {
    {
        ADT_ERR_INVALID_ARG,
        "Invalid argument"
    },
    {
        ADT_ERR_INVALID_ACTION,
        "Unrecognized action"
    },
    {
        ADT_ERR_FAILED_ALLOC,
        "Memory allocation failed"
    },
    {
        ADT_ERR_ACTION_MISSING,
        "Action argument is missing"
    },
    {
        ADT_ERR_INVALID_USAGE,
        "Invalid usage"
    },
    {
        ADT_ERR_ACTION_NOT_SUPPORTED,
        "Action is not supported"
    },
    {
        ADT_ERR_ACTION_INVALID_STATE,
        "Invalid action state"
    },
    {
        ADT_ERR_FAILED_POPT_CTX,
        "Failed to obtain Popt context"
    },
    {
        ADT_ERR_FAILED_TO_LOCATE_AD,
        "Failed to locate AD server to connect to"
    },
    {
        ADT_ERR_FAILED_TO_LOCATE_DOMAIN,
        "Failed to locate NT domain the machine is joined to"
    },
    {
        ADT_ERR_FAILED_AD_OPEN,
        "Failed to open connection to AD server"
    },
    {
        ADT_ERR_FAILED_AD_BIND,
        "Failed to authenticate to AD server"
    },
    {
        ADT_ERR_FAILED_AD_SET_VER,
        "Failed to set LDAP version"
    },
    {
        ADT_ERR_FAILED_AD_SET_OPT,
        "Failed to set LDAP option"
    },
    {
        ADT_ERR_FAILED_AD_SET_REF,
        "Failed to set LDAP referral chasing policy"
    },
    {
        ADT_ERR_FAILED_AD_SET_GSS,
        "Failed to set GSS authentication method"
    },
    {
        ADT_ERR_FAILED_AD_SET_SIZE_LIMIT,
        "Failed to set LDAP response size limit"
    },
    {
        ADT_ERR_FAILED_AD_GET_ROOT_DSE,
        "Failed to get root DSE of AD server"
    },
    {
        ADT_ERR_FAILED_AD_GET_DEFAULT_NC,
        "Failed to get default naming context"
    },
    {
        ADT_ERR_FAILED_GTYPE_CHECK,
        "Users/groups from other domains cannot be added to a Global group. Group type must either be \"Domain Local\" or \"Universal\""
    },
    {
        ADT_ERR_ARG_MISSING_UPN_FOR_KEYTAB,
        "Must specify user principal name when using krb5 keytab file"
    },
    {
        ADT_ERR_ARG_MISSING_CREDS_FOR_BIND,
        "Must specify user/password for simple bind"
    },
    {
        ADT_ERR_FAILED_AD_GET_LDAP_GC_READY,
        "Failed to check whether is AD server is integrated with Global Catalog"
    },
    {
        ADT_ERR_FAILED_AD_GET_LDAP_SPN,
        "Failed to resolve LDAP SPN"
    },
    {
        ADT_ERR_FAILED_AD_LOGON_DN,
        "Failed to find logon-as DN"
    },
    {
        ADT_ERR_FAILED_AD_OBJ_NOT_FOUND,
        "Failed to find DN: AD object not found"
    },
    {
        ADT_ERR_FAILED_AD_MULTIPLE_FOUND,
        "Failed to resolve DN: multiple objects found"
    },
    {
        ADT_ERR_ARG_MISSING_DN,
        "\"dn\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_SOURCE_DN,
        "\"source-dn\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_TARGET_DN,
        "\"target-dn\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_FROM,
        "\"from\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_TO,
        "\"to\" argument is missing"
    },
    {
        ADT_ERR_INVALID_PARENT_DN,
        "Parent component of the new object location does not exist"
    },
    {
        ADT_ERR_INVALID_FUN_ARG,
        "Invalid function argument"
    },
    {
        ADT_ERR_FAILED_AD_GET_ATTR,
        "Failed to get object attribute"
    },
    {
        ADT_ERR_FAILED_AD_GET_GUID,
        "Failed to get object\'s GUID"
    },
    {
        ADT_ERR_FAILED_AD_GET_SID,
        "Failed to get object\'s SID"
    },
    {
        ADT_ERR_INVALID_SID,
        "Invalid SID revision"
    },
    {
        ADT_ERR_ASSERTION,
        "Critical assertion failed. Cannot continue."
    },
    {
        ADT_ERR_INVALID_OBJECTGUID,
        "Globally Unique Identifier (GUID) is invalid"
    },
    {
        ADT_ERR_ARG_MISSING_PROP,
        "Property name must be specified"
    },
    {
        ADT_ERR_ARG_MISSING_CELL_PROP,
        "The requested properties are not set in the object"
    },
    {
        ADT_ERR_INVALID_PROP_DATA,
        "Invalid LW object attributes received from AD"
    },
    {
        ADT_ERR_INVALID_SEARCH_SCOPE,
        "Invalid search scope"
    },
    {
        ADT_ERR_OBJECT_NOT_EMPTY,
        "The object has child nodes. Use \"--force option\" to delete sub-entries"
    },
    {
        ADT_ERR_ARG_MISSING_CELL,
        "\"dn\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_TO_GROUP,
        "\"to-group\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_FROM_GROUP,
        "\"from-group\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_NAME,
        "\"name\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_PASSWD,
        "\"password\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_CN,
        "You must specify at least one of these arguments: \"first-name\", \"last-name\", \"cn\""
    },
    {
        ADT_ERR_ARG_MISSING_FILTER,
        "\"filter\" argument is missing"
    },
    {
        ADT_ERR_ARG_MISSING_USER_GROUP,
        "\"user\" or \"group\" argument must be specified"
    },
    {
        ADT_ERR_ARG_MISSING_USER,
        "\"user\" argument must be specified"
    },
    {
        ADT_ERR_ARG_MISSING_GROUP,
        "\"group\" argument must be specified"
    },
    {
        ADT_ERR_INVALID_USER_GROUP,
        "Cannot accept both \"user\" and \"group\" arguments. Only one can be used."
    },
    {
        ADT_ERR_INVALID_GROUP_SCOPE,
        "Invalid group type. Acceptable values: domain-local, global, universal. Default: global"
    },
    {
        ADT_ERR_MULTIPLE_USERS,
        "Multiple users match the provided name. Try to use more specific criteria, e.g. samAccountName=<name>, or userPrincipalName=<name>, or use distinguished name"
    },
    {
        ADT_ERR_MULTIPLE_GROUPS,
        "Multiple groups match the provided name. Try to use more specific criteria, e.g. samAccountName=<name>, or cn=<name>, or use distinguished name"
    },
    {
        ADT_ERR_MULTIPLE_COMPUTERS,
        "Multiple computers match the provided name. Try to use more specific criteria, e.g. samAccountName=<name>, or cn=<name>, or use distinguished name"
    },
    {
        ADT_ERR_FAILED_AD_USER_NOT_FOUND,
        "Failed to find the AD user."
    },
    {
        ADT_ERR_FAILED_AD_GROUP_NOT_FOUND,
        "Failed to find the AD group."
    },
    {
        ADT_ERR_MEXCL_ARGS_PWD_NEVER_EXP,
        "Mutually exclusive requirements: no-must-change-password=false and no-password-expires=true"
    },
    {
        ADT_ERR_MEXCL_ARGS_PWD_CANNOT_CH,
        "Mutually exclusive requirements: no-must-change-password=false and no-can-change-password=true"
    },
    {
        ADT_ERR_AUTH_FAILED,
        "Authentication failed"
    },
    {
        ADT_ERR_INVALID_ARG_USER_COMPUTER,
        "Either \"--user\" or \"--computer\" parameter must be specified"
    },
    {
        ADT_ERR_INVALID_UID_UNAME,
        "User with the specified UID or login name already exists in the cell"
    },
    {
        0,
        NULL
    }
};

/**
 * Get adtool error message.
 *
 * @param dwError Error code.
 * @return Error message.
 */
PCSTR GetErrorMsg(IN DWORD dwError) {
    int i;

    if((dwError < ADT_ERR_BASE) || (dwError >= (ADT_ERR_BASE + ADT_ERR_RANGE))) {
        return NULL;
    }

    for(i = 0; AdtErrorMap[i].code != 0; ++i) {
        if(AdtErrorMap[i].code == dwError) {
            return AdtErrorMap[i].msg;
        }
    }

    return ADT_ERR_MSG_UNKNOWN;
}

/**
 * Save krb5 error message in the global error string.
 *
 * @param ctx Krb5 context.
 * @param err Krb5 error code.
 */
VOID Krb5SetGlobalErrorString(IN krb5_context ctx, IN krb5_error_code err)
{
    PCSTR errStr = NULL;

    if(!ctx || !err) {
        return;
    }

    errStr = krb5_get_error_message(ctx, err);

    if(errStr) {
        memset(adtLastErrMsg, 0, ADT_ERROR_MSG_SIZE_MAX);
        strncpy(adtLastErrMsg, errStr, ADT_ERROR_MSG_SIZE_MAX - 1);
        krb5_free_error_message(ctx, errStr);
    }
}
