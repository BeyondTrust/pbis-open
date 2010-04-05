/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *       Mac Workgroup Manager
 * 
 *       AD Utility API
 * 
 *       Global Variables
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

MACADUTIL_FUNC_TABLE gpfnTable =
    {
            &GetADDomain,
            &EnumWorkgroupManagerEnabledGPOs,
            &GetSpecificGPO,
            &IsMCXSettingEnabledForGPO,
            &ConvertDSAttributeValuesToMCXValues,
            &SaveMCXValuesForGPOSettingType,
            &ConvertMCXSettingsToMCXValues,
            &GetMCXValuesForGPOSettingType,
            &LookupComputerGroupGPO,
            &LookupComputerListGPO,
            &AuthenticateUser,
            &GetUserPrincipalNames,
            &GetLsaStatus,
            &NotifyUserLogon,
            &NotifyUserLogoff,
            &GetHomeDirectoryDockMCXValue,
            &GetADUserInfo,
            &GetConfigurationSettings,
            &GetAccessCheckData,
            &CheckUserForAccess,
            &FreeAccessCheckData
    };

pthread_mutex_t g_ADULock = PTHREAD_MUTEX_INITIALIZER;

time_t gdwKrbTicketExpiryTime = 0;
const double gdwExpiryGraceSeconds = (60 * 60);
