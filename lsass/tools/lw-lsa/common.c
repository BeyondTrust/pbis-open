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
 * Module Name:
 *
 *        common.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Common functions for tools
 *
 * Authors: Brian Koropoff(bkoropoff@likewise.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsaclient.h"
#include <lsa/lsa.h>
#include <lsa/lsa2.h>
#include <lwmem.h>
#include <lwerror.h>

#include "common.h"

#define SAFE_STRING(x) ((x) == NULL ? "<null>" : (x))

VOID
PrintSecurityObject(
    PLSA_SECURITY_OBJECT pObject,
    DWORD dwObjectNumber,
    DWORD dwObjectTotal
    )
{
    switch (pObject->type)
    {
    case LSA_OBJECT_TYPE_GROUP:
        if (dwObjectTotal)
        {
            printf("Group object [%d of %d] (%s)\n", dwObjectNumber+1, dwObjectTotal, SAFE_STRING(pObject->pszObjectSid));
        }
        else
        {
            printf("Group object [%d] (%s)\n", dwObjectNumber+1, SAFE_STRING(pObject->pszObjectSid));   
        }
        printf("============\n");
        printf("Enabled: %s\n", pObject->enabled ? "yes" : "no");
        printf("Distinguished name: %s\n", SAFE_STRING(pObject->pszDN));
        printf("SAM account name: %s\n", SAFE_STRING(pObject->pszSamAccountName));
        printf("NetBIOS domain name: %s\n", SAFE_STRING(pObject->pszNetbiosDomainName));
        printf("Alias: %s\n", SAFE_STRING(pObject->groupInfo.pszAliasName));
        printf("UNIX name: %s\n", SAFE_STRING(pObject->groupInfo.pszUnixName));
        printf("GID: %lu\n", (unsigned long) pObject->groupInfo.gid);
        break;
    case LSA_OBJECT_TYPE_USER:
        if (dwObjectTotal)
        {
            printf("User object [%d of %d] (%s)\n", dwObjectNumber+1, dwObjectTotal, SAFE_STRING(pObject->pszObjectSid));
        }
        else
        {
            printf("User object [%d] (%s)\n", dwObjectNumber+1, SAFE_STRING(pObject->pszObjectSid));
        }
        printf("============\n");
        printf("Enabled: %s\n", pObject->enabled ? "yes" : "no");
        printf("Distinguished name: %s\n", SAFE_STRING(pObject->pszDN));
        printf("SAM account name: %s\n", SAFE_STRING(pObject->pszSamAccountName));
        printf("NetBIOS domain name: %s\n", SAFE_STRING(pObject->pszNetbiosDomainName));
        if (pObject->userInfo.bIsGeneratedUPN)
        {
            printf("UPN (generated): %s\n", SAFE_STRING(pObject->userInfo.pszUPN));
        }
        else
        {
            printf("UPN: %s\n", SAFE_STRING(pObject->userInfo.pszUPN));
        }
        printf("Alias: %s\n", SAFE_STRING(pObject->userInfo.pszAliasName));
        printf("UNIX name: %s\n", SAFE_STRING(pObject->userInfo.pszUnixName));
        printf("GECOS: %s\n", SAFE_STRING(pObject->userInfo.pszAliasName));        
        printf("Shell: %s\n", SAFE_STRING(pObject->userInfo.pszShell));
        printf("Home directory: %s\n", SAFE_STRING(pObject->userInfo.pszHomedir));
        printf("UID: %lu\n", (unsigned long) pObject->userInfo.uid);
        printf("Primary group SID: %s\n", SAFE_STRING(pObject->userInfo.pszPrimaryGroupSid));
        printf("Primary GID: %lu\n", (unsigned long) pObject->userInfo.gid);
        if (pObject->userInfo.bIsAccountInfoKnown)
        {
            printf("Password expired: %s\n", pObject->userInfo.bPasswordExpired ? "yes" : "no");
            printf("Password nevers expires: %s\n", pObject->userInfo.bPasswordNeverExpires ? "yes" : "no");
            printf("Change password on next logon: %s\n", pObject->userInfo.bPromptPasswordChange ? "yes" : "no");
            printf("User can change password: %s\n", pObject->userInfo.bUserCanChangePassword ? "yes" : "no");
            printf("Account disabled: %s\n", pObject->userInfo.bAccountDisabled ? "yes" : "no");
            printf("Account expired: %s\n", pObject->userInfo.bAccountExpired ? "yes" : "no");
            printf("Account locked: %s\n", pObject->userInfo.bAccountLocked ? "yes" : "no");
        }
        break;
    default:
        printf("Unknown object (%s)\n", SAFE_STRING(pObject->pszObjectSid));
        break;
    }
}

PCSTR
Basename(
    PCSTR pszPath
    )
{
    PSTR pszSlash = strrchr(pszPath, '/');

    if (pszSlash)
    {
        return pszSlash + 1;
    }
    else
    {
        return pszPath;
    }
}
