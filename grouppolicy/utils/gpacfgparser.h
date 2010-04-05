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

#ifndef __GPACFGPARSER_H__
#define __GPACFGPARSER_H__

#include "includes.h"

typedef struct __GPANVPAIR {

    PSTR pszName;
    PSTR pszValue;

    struct __GPANVPAIR *pNext;

} GPANVPAIR, *PGPANVPAIR;

typedef struct __GPACFGSECTION {

    PSTR pszName;

    PGPANVPAIR pNVPairList;

    struct __GPACFGSECTION *pNext;

} GPACFGSECTION, *PGPACFGSECTION;

CENTERROR
GPAParseConfigFile(
    PCSTR pszFilePath,
    PGPACFGSECTION* ppSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    );

void
GPAFreeConfigSectionList(
    PGPACFGSECTION pSectionList
    );

CENTERROR
GPASaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PGPACFGSECTION pSectionList
    );

CENTERROR
GPASaveConfigSectionListToFile(
    FILE* fp,
    PGPACFGSECTION pSectionList
    );

CENTERROR
GPACreateConfigSection(
    PGPACFGSECTION* ppSectionList,
    PGPACFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    );

CENTERROR
GPAGetConfigValueBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    );

CENTERROR
GPASetConfigValueBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    );

CENTERROR
GPAGetConfigValueBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    );

CENTERROR
GPASetConfigValueBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    );

CENTERROR
GPADeleteConfigSection(
    PGPACFGSECTION* ppSectionList,
    PCSTR pszSectionName
    );

CENTERROR
GPADeleteNameValuePairBySection(
    PGPACFGSECTION pSection,
    PCSTR pszName
    );

CENTERROR
GPADeleteNameValuePairBySectionName(
    PGPACFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    );
#endif /* __GPACFGPARSER_H__ */
