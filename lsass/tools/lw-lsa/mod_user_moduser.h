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
 *        moduser.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Driver for program to modify an existing user
 *
 * Authors:
 *
 *        Krishna Ganugapati (krishnag@likewisesoftware.com)
 *        Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __MODUSER_H__
#define __MODUSER_H__

typedef enum
{
    UserModTask_EnableUser,
    UserModTask_DisableUser,
    UserModTask_UnlockUser,
    UserModTask_ChangePasswordAtNextLogon,
    UserModTask_SetPasswordNeverExpires,
    UserModTask_SetPasswordMustExpire,
    UserModTask_AddToGroups,
    UserModTask_RemoveFromGroups,
    UserModTask_SetNtPasswordHash,
    UserModTask_SetLmPasswordHash,
    UserModTask_SetExpiryDate,
    UserModTask_SetPrimaryGroup,
    UserModTask_SetHomedir,
    UserModTask_SetShell,
    UserModTask_SetGecos,
    UserModTask_SetPassword
} UserModificationTaskType;

typedef struct __USER_MOD_TASK
{
    UserModificationTaskType taskType;
    PSTR pszData;
} USER_MOD_TASK, *PUSER_MOD_TASK;

#endif /* __MODUSER_H__ */
