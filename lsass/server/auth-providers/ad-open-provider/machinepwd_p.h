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
 *        machinepwd_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Machine Password Sync API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __MACHINEPWD_P_H__
#define __MACHINEPWD_P_H__

DWORD
ADInitMachinePasswordSync(
    IN PLSA_AD_PROVIDER_STATE pState
    );

DWORD
ADStartMachinePasswordSync(
    IN PLSA_AD_PROVIDER_STATE pState
    );

VOID
ADSyncTimeToDC(
    PLSA_AD_PROVIDER_STATE pState,
    PCSTR pszDomainFQDN
    );

#define AD_LOCK_MACHINE_PASSWORD(handle, locked) \
            do \
            { \
                if (!(locked)) \
                { \
                    ADLockMachinePassword(handle); \
                    (locked) = TRUE; \
                } \
            } while(0)

VOID
ADLockMachinePassword(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    );

#define AD_UNLOCK_MACHINE_PASSWORD(handle, locked) \
            do \
            { \
                if (locked) \
                { \
                    ADUnlockMachinePassword(handle); \
                    (locked) = FALSE; \
                } \
            } while(0)

VOID
ADUnlockMachinePassword(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    );

VOID
ADShutdownMachinePasswordSync(
    IN OUT LSA_MACHINEPWD_STATE_HANDLE* phMachinePwdState
    );

DWORD
ADRefreshMachineTGT(
    IN PLSA_AD_PROVIDER_STATE pState,
    OUT OPTIONAL PDWORD pdwGoodUntilTime
    );

VOID
ADSetMachineTGTExpiry(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState,
    IN DWORD dwGoodUntil
    );

VOID
ADSetMachineTGTExpiryError(
    IN LSA_MACHINEPWD_STATE_HANDLE hMachinePwdState
    );

#endif /* __MACHINEPWD_P_H__ */
