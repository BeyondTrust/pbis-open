/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samdbglobals.c
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
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
