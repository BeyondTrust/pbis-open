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
 *        ad_groups_p.h
 *
 * Abstract:
 *
 *        User monitor service for users and groups
 * 
 *        Functions internal to this package for enumerating and tracking
 *        AD groups.
 *
 * Authors: Kyle Stemen <kstemen@beyondtrust.com>
 * 
 */
#ifndef __AD_GROUPS_P_H__
#define __AD_GROUPS_P_H__

DWORD
UmnSrvWriteADGroupEvent(
    PLW_EVENTLOG_CONNECTION pEventlog,
    long long PreviousRun,
    PUSER_MONITOR_GROUP pOld,
    long long Now,
    PLSA_SECURITY_OBJECT pNew
    );

DWORD
UmnSrvUpdateADGroup(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroups,
    long long PreviousRun,
    long long Now,
    PLSA_SECURITY_OBJECT pGroup
    );

DWORD
UmnSrvUpdateADGroupMember(
    PLW_EVENTLOG_CONNECTION pEventlog,
    HANDLE hReg,
    HKEY hGroups,
    long long PreviousRun,
    long long Now,
    PLSA_SECURITY_OBJECT pGroup,
    PCSTR pMember
    );

#endif /* __AD_GROUPS_P_H__ */
