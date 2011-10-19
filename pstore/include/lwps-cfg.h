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
 *        lwps-cfg.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        Config (INF Style) parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWPS_CFG_H__
#define __LWPS_CFG_H__

//defined flags in dwOptions
#define LWPS_CFG_OPTION_STRIP_SECTION         0x00000001
#define LWPS_CFG_OPTION_STRIP_NAME_VALUE_PAIR 0x00000002
#define LWPS_CFG_OPTION_STRIP_ALL             (LWPS_CFG_OPTION_STRIP_SECTION | \
                                         LWPS_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNLWPS_CONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNLWPS_CONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNLWPS_CONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNLWPS_CONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PVOID    pData,
                        PBOOLEAN pbContinue
                        );

DWORD
LwpsParseConfigFile(
    PCSTR                          pszFilePath,
    DWORD                          dwOptions,
    PFNLWPS_CONFIG_START_SECTION   pfnStartSectionHandler,
    PFNLWPS_CONFIG_COMMENT         pfnCommentHandler,
    PFNLWPS_CONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNLWPS_CONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                          pData
    );

#endif /* __LWPS_CFG_H__ */
