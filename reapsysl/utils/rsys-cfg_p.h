/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rsys-cfg_p.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __RSYS_CFG_P_H__
#define __RSYS_CFG_P_H__

#define RSYS_CFG_TOKEN_DEFAULT_LENGTH 128

typedef struct __RSYS_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    PSTR pszConfigKey;
    PSTR pszPolicyKey;
}RSYS_CONFIG_REG,*PRSYS_CONFIG_REG;

DWORD
RSYSOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PRSYS_CONFIG_REG *ppReg
    );

VOID
RSYSCloseConfig(
    PRSYS_CONFIG_REG pReg
    );

DWORD
RSYSReadConfigDword(
    PRSYS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    );

DWORD
RSYSReadConfigString(
    PRSYS_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

DWORD
RSYSReadConfigEnum(
    PRSYS_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    );

DWORD
RSYSReadConfigBoolean(
    PRSYS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

DWORD
RSysReadConfigEnum(
    PRSYS_CONFIG_REG pReg,
    PCSTR   pszName,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    );

DWORD
RSysRegComp(
    regex_t* pRegex,
    PBOOLEAN pbCompiled,
    PCSTR pszExpr
    );


#endif /* __RSYS_CFG_P_H__ */
