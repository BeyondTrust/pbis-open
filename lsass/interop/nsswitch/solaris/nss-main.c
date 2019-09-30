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
 *        nss-main.c
 *
 * Abstract:
 * 
 *        Name Server Switch (BeyondTrust LSASS)
 * 
 *        Main Entry Points
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "nss-user.h"
#include "nss-shadow.h"
#include "nss-group.h"
#include "nss-netgrp.h"

/* The addition of this symbol indicates to NSCD that we are NSS2 compatible 
 * To support this the various user and group routines have been modified to return
 * the etc files format for the passwd/group entities when being called by NSCD
 */
void *_nss_lsass_version = 0;

nss_backend_t*
_nss_lsass_passwd_constr(
    const char* pszDbName,
    const char* pszSrcName,
    const char* pszCfgStr
    )
{
    return LsaNssSolarisPasswdCreateBackend();
}

nss_backend_t*
_nss_lsass_shadow_constr(
    const char* pszDbName,
    const char* pszSrcName,
    const char* pszCfgStr
    )
{
    return LsaNssSolarisShadowCreateBackend();
}

nss_backend_t*
_nss_lsass_group_constr(
    const char* pszDbName,
    const char* pszSrcName,
    const char* pszCfgStr
    )
{
    return LsaNssSolarisGroupCreateBackend();
}

nss_backend_t*
_nss_lsass_netgroup_constr(
    const char* pszDbName,
    const char* pszSrcName,
    const char* pszCfgStr
    )
{
    return LsaNssSolarisNetgroupCreateBackend();
}
