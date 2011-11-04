/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        samdbglobals.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Global variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassDomainAttrMaps[] =
    { SAMDB_DOMAIN_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassBuiltInDomainAttrMaps[] =
    { SAMDB_CONTAINER_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassContainerAttrMaps[] =
    { SAMDB_CONTAINER_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassLocalGroupAttrMaps[] =
    { SAMDB_LOCAL_GROUP_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassUserAttrMaps[] =
    { SAMDB_USER_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassLocalGroupMemberAttrMaps[] =
    { SAMDB_LOCALGRP_MEMBER_ATTRIBUTE_MAP };

static
SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO gObjectClassMaps[] =
{
    {
        SAMDB_OBJECT_CLASS_DOMAIN,
        &gObjectClassDomainAttrMaps[0],
        sizeof(gObjectClassDomainAttrMaps)/sizeof(gObjectClassDomainAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN,
        &gObjectClassBuiltInDomainAttrMaps[0],
        sizeof(gObjectClassBuiltInDomainAttrMaps)/sizeof(gObjectClassBuiltInDomainAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_CONTAINER,
        &gObjectClassContainerAttrMaps[0],
        sizeof(gObjectClassContainerAttrMaps)/sizeof(gObjectClassContainerAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_LOCAL_GROUP,
        &gObjectClassLocalGroupAttrMaps[0],
        sizeof(gObjectClassLocalGroupAttrMaps)/sizeof(gObjectClassLocalGroupAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_USER,
        &gObjectClassUserAttrMaps[0],
        sizeof(gObjectClassUserAttrMaps)/sizeof(gObjectClassUserAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER,
        &gObjectClassLocalGroupMemberAttrMaps[0],
        sizeof(gObjectClassLocalGroupMemberAttrMaps)/sizeof(gObjectClassLocalGroupMemberAttrMaps[0])
    }
};

static
SAM_DB_ATTRIBUTE_MAP gAttrMaps[] =
{
    SAMDB_OBJECT_ATTRIBUTE_MAP
};

SAM_GLOBALS gSamGlobals =
    {
        .mutex                     = PTHREAD_MUTEX_INITIALIZER,

        .pObjectClassAttrMaps      = &gObjectClassMaps[0],
        .dwNumObjectClassAttrMaps  = (sizeof(gObjectClassMaps)/
                                     sizeof(gObjectClassMaps[0])),

        .pAttrMaps                 = &gAttrMaps[0],
        .dwNumMaps                 = (sizeof(gAttrMaps)/sizeof(gAttrMaps[0])),
        .attrLookup                = {NULL},

        .pszProviderName           = NULL,

        .providerFunctionTable     = {NULL},

        .pDbContextList            = NULL,
        .dwNumDbContexts           = 0,
        .dwNumMaxDbContexts        = 0
    };


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
