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
 *        externs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Global Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "adprovider.h"

pthread_rwlock_t gADGlobalDataLock;

pthread_mutex_t gADDefaultDomainLock;

PCSTR gpszADProviderName = LSA_PROVIDER_TAG_AD;

LSA_PROVIDER_FUNCTION_TABLE gADProviderAPITable =
{
    .pfnFindObjects = AD_FindObjects,
    .pfnOpenEnumObjects = AD_OpenEnumObjects,
    .pfnEnumObjects = AD_EnumObjects,
    .pfnOpenEnumGroupMembers = AD_OpenEnumMembers,
    .pfnEnumGroupMembers = AD_EnumMembers,
    .pfnCloseEnum = AD_CloseEnum,
    .pfnQueryMemberOf = AD_QueryMemberOf,
    .pfnGetSmartCardUserObject = AD_GetSmartCardUserObject,
    .pfnGetMachineAccountInfoA = AD_GetMachineAccountInfoA,
    .pfnGetMachineAccountInfoW = AD_GetMachineAccountInfoW,
    .pfnGetMachinePasswordInfoA = AD_GetMachinePasswordInfoA,
    .pfnGetMachinePasswordInfoW = AD_GetMachinePasswordInfoW,
    .pfnShutdownProvider = AD_ShutdownProvider,
    .pfnOpenHandle = AD_OpenHandle,
    .pfnCloseHandle = AD_CloseHandle,
    .pfnServicesDomain = AD_ServicesDomain,
    .pfnAuthenticateUserPam = AD_AuthenticateUserPam,
    .pfnAuthenticateUserEx = AD_AuthenticateUserEx,
    .pfnValidateUser = AD_ValidateUser,
    .pfnCheckUserInList = AD_CheckUserInList,
    .pfnChangePassword = AD_ChangePassword,
    .pfnSetPassword = AD_SetPassword,
    .pfnAddUser = AD_AddUser,
    .pfnModifyUser = AD_ModifyUser,
    .pfnAddGroup = AD_AddGroup,
    .pfnModifyGroup = AD_ModifyGroup,
    .pfnDeleteObject = AD_DeleteObject,
    .pfnOpenSession = AD_OpenSession,
    .pfnCloseSession = AD_CloseSession,
    .pfnLookupNSSArtefactByKey = AD_FindNSSArtefactByKey,
    .pfnBeginEnumNSSArtefacts = AD_BeginEnumNSSArtefacts,
    .pfnEnumNSSArtefacts = AD_EnumNSSArtefacts,
    .pfnEndEnumNSSArtefacts = AD_EndEnumNSSArtefacts,
    .pfnGetStatus = AD_GetStatus,
    .pfnFreeStatus = AD_FreeStatus,
    .pfnRefreshConfiguration = AD_RefreshConfiguration,
    .pfnProviderIoControl = AD_ProviderIoControl
};


// please put all new globals in the LSA_AD_PROVIDER_STATE 
// structures which are stored in the following list:
LSA_LIST_LINKS gLsaAdProviderStateList;


ADCACHE_PROVIDER_FUNCTION_TABLE ADCacheTable;

PADCACHE_PROVIDER_FUNCTION_TABLE gpCacheProvider = &ADCacheTable;

BOOLEAN gbMultiTenancyEnabled = FALSE;

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
