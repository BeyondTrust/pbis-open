/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
