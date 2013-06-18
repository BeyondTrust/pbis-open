/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        localcfg.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#ifndef __LOCAL_CFG_H__
#define __LOCAL_CFG_H__

DWORD
LocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    );

DWORD
LocalCfgTransferContents(
    PLOCAL_CONFIG pSrcConfig,
    PLOCAL_CONFIG pDstConfig
    );

DWORD
LocalCfgReadRegistry(
    PLOCAL_CONFIG pConfig
    );

DWORD
LocalCfgGetMinPasswordAge(
    PLONG64 pllMinPwdAge
    );

DWORD
LocalCfgGetMaxPasswordAge(
    PLONG64 pllMaxPwdAge
    );

DWORD
LocalCfgGetMinPwdLength(
    PDWORD pdwMinPwdLength
    );

DWORD
LocalCfgGetPasswordChangeWarningTime(
    PLONG64 pllPasswdChangeWarningTime
    );

DWORD
LocalCfgIsEventlogEnabled(
    PBOOLEAN pbValue
    );

DWORD
LocalCfgGetMaxGroupNestingLevel(
    PDWORD pdwNestingLevel
    );

DWORD
LocalCfgGetDefaultShell(
    PSTR* ppszLoginShell
    );

DWORD
LocalCfgGetHomedirPrefix(
    PSTR* ppszHomedirPrefix
    );

DWORD
LocalCfgGetHomedirTemplate(
    PSTR* ppszHomedirTemplate
    );

DWORD
LocalCfgGetHomedirUmask(
    mode_t* pUmask
    );

DWORD
LocalCfgMustCreateHomedir(
    PBOOLEAN pbCreateHomedir
    );

DWORD
LocalCfgAcceptNTLMv1(
    PBOOLEAN pbResult
    );

DWORD
LocalCfgGetSkeletonDirs(
    PSTR* ppszSkelDirs
    );

DWORD
LocalCfgGetEnableUnixIds(
    PBOOLEAN pbResult
    );

VOID
LocalCfgFree(
    PLOCAL_CONFIG pConfig
    );

VOID
LocalCfgFreeContents(
    PLOCAL_CONFIG pConfig
    );

#endif /* __LOCAL_CFG_H__ */
