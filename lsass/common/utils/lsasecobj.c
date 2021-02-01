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

#include "includes.h"

VOID
LsaUtilFreeSecurityObject(
    PLSA_SECURITY_OBJECT pObject
    )
{
    if (pObject)
    {
        LW_SAFE_FREE_STRING(pObject->pszDN);
        LW_SAFE_FREE_STRING(pObject->pszObjectSid);
        LW_SAFE_FREE_STRING(pObject->pszNetbiosDomainName);
        LW_SAFE_FREE_STRING(pObject->pszSamAccountName);
        
        switch(pObject->type)
        {
        case LSA_OBJECT_TYPE_GROUP:
            LW_SAFE_FREE_STRING(pObject->groupInfo.pszAliasName);
            LW_SAFE_FREE_STRING(pObject->groupInfo.pszUnixName);
            LW_SAFE_FREE_STRING(pObject->groupInfo.pszPasswd);
            break;
        case LSA_OBJECT_TYPE_USER:
            LW_SAFE_FREE_STRING(pObject->userInfo.pszPrimaryGroupSid);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszUPN);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszAliasName);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszUnixName);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszPasswd);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszGecos);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszShell);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszHomedir);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszDisplayName);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszWindowsHomeFolder);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszLocalWindowsHomeFolder);
            LW_SECURE_FREE_MEMORY(pObject->userInfo.pLmHash, pObject->userInfo.dwLmHashLen);
            LW_SECURE_FREE_MEMORY(pObject->userInfo.pNtHash, pObject->userInfo.dwNtHashLen);
            break;
        }

        LwFreeMemory(pObject);
    }
}

void
LsaUtilFreeSecurityObjectList(
    DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjectList
    )
{
    DWORD dwIndex = 0;

    if (ppObjectList)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LsaUtilFreeSecurityObject(ppObjectList[dwIndex]);
        }

        LwFreeMemory(ppObjectList);
    }
}
