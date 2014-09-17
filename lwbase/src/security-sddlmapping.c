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
 *        security-sddlmapping.c
 *
 * Abstract:
 *
 *        Sid-string and Rid mapping
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "includes.h"
#include "security-sddl-internal.h"

typedef struct _TABLE_ENTRY
{
    ULONG rid;
    PCSTR pszSddl;
    PCSTR pszSidString;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
RtlpSddlLookupRid(
    IN ULONG rid
    );

static
PTABLE_ENTRY
RtlpSddlLookupSddl(
    IN PCSTR pszSddl
    );

static
TABLE_ENTRY LwSddlSidStringTable[] =
{
    { SECURITY_ANONYMOUS_LOGON_RID, SDDL_ANONYMOUS, SECURITY_ANONYMOUS_LOGON_PREFIX },
    { SECURITY_CREATOR_GROUP_RID, SDDL_CREATOR_GROUP, SECURITY_CREATOR_GROUP_PREFIX },
    { SECURITY_CREATOR_OWNER_RID, SDDL_CREATOR_OWNER, SECURITY_CREATOR_OWNER_PREFIX },
    { SECURITY_AUTHENTICATED_USER_RID, SDDL_AUTHENTICATED_USERS, SECURITY_AUTHENTICATED_USER_PREFIX },
    { SECURITY_NETWORK_RID, SDDL_NETWORK, SECURITY_NETWORK_PREFIX },
    { SECURITY_PRINCIPAL_SELF_RID, SDDL_PERSONAL_SELF, SECURITY_PRINCIPAL_SELF_PREFIX },
    { SECURITY_RESTRICTED_CODE_RID, SDDL_RESTRICTED_CODE, SECURITY_RESTRICTED_CODE_PREFIX },
    { SECURITY_SERVICE_RID, SDDL_SERVICE, SECURITY_SERVICE_PREFIX },
    { SECURITY_LOCAL_SYSTEM_RID, SDDL_LOCAL_SYSTEM, SECURITY_LOCAL_SYSTEM_PREFIX },
    { SECURITY_INTERACTIVE_RID, SDDL_INTERACTIVE, SECURITY_INTERACTIVE_PREFIX },
    { DOMAIN_ALIAS_RID_ADMINS, SDDL_BUILTIN_ADMINISTRATORS, SECURITY_BUILTIN_DOMAIN_ADMIN },
    { DOMAIN_ALIAS_RID_GUESTS, SDDL_BUILTIN_GUESTS, SECURITY_BUILTIN_DOMAIN_GUESTS },
    { DOMAIN_ALIAS_RID_USERS, SDDL_BUILTIN_USERS, SECURITY_BUILTIN_DOMAIN_USERS},
#if 0
    { DOMAIN_ALIAS_RID_ACCOUNT_OPS, SDDL_ACCOUNT_OPERATORS },
    { DOMAIN_ALIAS_RID_BACKUP_OPS, SDDL_BACKUP_OPERATORS },
    { DOMAIN_GROUP_RID_CERT_ADMINS, SDDL_CERT_SERV_ADMINISTRATORS },
    { DOMAIN_GROUP_RID_ADMINS, SDDL_DOMAIN_ADMINISTRATORS },
    { DOMAIN_GROUP_RID_COMPUTERS, SDDL_DOMAIN_COMPUTERS },
    { DOMAIN_GROUP_RID_CONTROLLERS, SDDL_DOMAIN_DOMAIN_CONTROLLERS },
    { DOMAIN_GROUP_RID_GUESTS, SDDL_DOMAIN_GUESTS },
    { DOMAIN_GROUP_RID_USERS, SDDL_DOMAIN_USERS },
    { DOMAIN_GROUP_RID_ENTERPRISE_ADMINS, SDDL_ENTERPRISE_ADMINS },
    { DOMAIN_USER_RID_ADMIN, SDDL_LOCAL_ADMIN },
    { DOMAIN_USER_RID_GUEST, SDDL_LOCAL_GUEST },
    { DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS, SDDL_NETWORK_CONFIGURATION_OPS },
    { DOMAIN_GROUP_RID_POLICY_ADMINS, SDDL_GROUP_POLICY_ADMINS },
    { DOMAIN_ALIAS_RID_PRINT_OPS, SDDL_PRINTER_OPERATORS },
    { DOMAIN_ALIAS_RID_POWER_USERS, SDDL_POWER_USERS },
    { DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, SDDL_REMOTE_DESKTOP },
    { DOMAIN_ALIAS_RID_REPLICATOR, SDDL_REPLICATOR },
    { DOMAIN_ALIAS_RID_RAS_SERVERS, SDDL_RAS_SERVERS },
    { DOMAIN_ALIAS_RID_PREW2KCOMPACCESS, SDDL_ALIAS_PREW2KCOMPACC },
    { DOMAIN_GROUP_RID_SCHEMA_ADMINS, SDDL_SCHEMA_ADMINISTRATORS },
    { DOMAIN_ALIAS_RID_SYSTEM_OPS, SDDL_SERVER_OPERATORS },
    { SECURITY_SERVER_LOGON_RID, SDDL_ENTERPRISE_DOMAIN_CONTROLLERS },
    { SECURITY_MANDATORY_HIGH_RID, SDDL_ML_HIGH },
    { SECURITY_MANDATORY_MEDIUM_RID, SDDL_ML_MEDIUM },
    { SECURITY_MANDATORY_LOW_RID, SDDL_ML_LOW },
    { SECURITY_MANDATORY_SYSTEM_RID, SDDL_ML_SYSTEM },
    { SECURITY_LOCAL_SERVICE_RID, SDDL_LOCAL_SERVICE },
    { SECURITY_NETWORK_SERVICE_RID, SDDL_NETWORK_SERVICE }
#endif
    {-1,  "", ""}
};
#undef SIDRID_MAPPING

LW_PCSTR
RtlpRidToSddl(
    LW_IN ULONG rid
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupRid(rid);

    if (pEntry && pEntry->pszSddl)
    {
        return pEntry->pszSddl;
    }
    else
    {
        return NULL;
    }
}

ULONG
RtlpSddlToRid(
    LW_IN PCSTR pszSddl
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupSddl(pszSddl);

    if (pEntry)
    {
        return pEntry->rid;
    }
    else
    {
        return -1;
    }
}

LW_PCSTR
RtlpSddlToSidString(
    LW_IN PCSTR pszSddl
    )
{
    PTABLE_ENTRY pEntry = RtlpSddlLookupSddl(pszSddl);

    if (pEntry)
    {
        return pEntry->pszSidString;
    }
    else
    {
        return NULL;
    }
}

static
PTABLE_ENTRY
RtlpSddlLookupRid(
    IN ULONG rid
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwSddlSidStringTable) / sizeof(*LwSddlSidStringTable); index++)
    {
        if (LwSddlSidStringTable[index].rid == rid)
        {
            return &LwSddlSidStringTable[index];
        }
    }

    return NULL;
}

static
PTABLE_ENTRY
RtlpSddlLookupSddl(
    IN PCSTR pszSddl
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwSddlSidStringTable) / sizeof(*LwSddlSidStringTable); index++)
    {
        if (!strcasecmp(LwSddlSidStringTable[index].pszSddl, pszSddl))
        {
            return &LwSddlSidStringTable[index];
        }
    }

    return NULL;
}
