/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        machinepwdinfo-impl.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
