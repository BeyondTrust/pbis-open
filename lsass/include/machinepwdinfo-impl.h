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
 *        machinepwdinfo-impl.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Machine Account/Password Info Memory Managent Implementation Helper
 *
 *        These functions should not be exposed across any API boundaries.
 *        Rather, they are intended to help implement memory management
 *        related to machine account/password within a single
 *        component/module.
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#ifndef _LSA_MACHINE_INFO_MEM_IMPL_
#define _LSA_MACHINE_INFO_MEM_IMPL_

#include <lsa/lsapstore-types.h>
#include <lw/attrs.h>

VOID
LsaImplFreeMachinePasswordInfoContentsA(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

VOID
LsaImplFreeMachinePasswordInfoContentsW(
    IN OUT PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

DWORD
LsaImplFillMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A pTargetPasswordInfo
    );

DWORD
LsaImplFillMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pSourcePasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W pTargetPasswordInfo
    );

VOID
LsaImplFreeMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo
    );

VOID
LsaImplFreeMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo
    );

VOID
LsaImplFreeMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    );

VOID
LsaImplFreeMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo
    );

DWORD
LsaImplDuplicateMachineAccountInfoA(
    IN PLSA_MACHINE_ACCOUNT_INFO_A pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppNewAccountInfo
    );

DWORD
LsaImplDuplicateMachineAccountInfoW(
    IN PLSA_MACHINE_ACCOUNT_INFO_W pAccountInfo,
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppNewAccountInfo
    );

DWORD
LsaImplDuplicateMachinePasswordInfoA(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppNewPasswordInfo
    );

DWORD
LsaImplDuplicateMachinePasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppNewPasswordInfo
    );

DWORD
LsaImplConvertMachinePasswordInfoMultiByteToWide(
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppNewPasswordInfo
    );

DWORD
LsaImplConvertMachinePasswordInfoWideToMultiByte(
    IN PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo,
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppNewPasswordInfo
    );

#endif
