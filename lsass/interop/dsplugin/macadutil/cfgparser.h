/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#ifndef __CFGPARSER_H__
#define __CFGPARSER_H__

typedef struct __NVPAIR {

    PSTR pszName;
    PSTR pszValue;

    struct __NVPAIR *pNext;

} NVPAIR, *PNVPAIR;

typedef struct __CFGSECTION {

    PSTR pszName;

    PNVPAIR pNVPairList;

    struct __CFGSECTION *pNext;

} CFGSECTION, *PCFGSECTION;

typedef struct
{
    void *data;
    /**
     * The number of items in the array in terms of the type this array holds,
     * not in terms of bytes.
     */
    size_t size;
    /**
     * The number of items that can be stored without having to reallocate
     * memory. This is in items, not bytes
     */
    size_t capacity;
} DynamicArray;

typedef struct __DBLBYTE {
    BYTE b1;
    BYTE b2;
} DBLBYTE, *PDBLBYTE;


DWORD
LWParseConfigFile(
    PCSTR pszFilePath,
    PCFGSECTION* ppSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    );

void
LWFreeConfigSectionList(
    PCFGSECTION pSectionList
    );

DWORD
LWSaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PCFGSECTION pSectionList
    );

DWORD
LWSaveConfigSectionListToFile(
    FILE* fp,
    PCFGSECTION pSectionList
    );

DWORD
LWCreateConfigSection(
    PCFGSECTION* ppSectionList,
    PCFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    );

DWORD
LWGetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    );

DWORD
LWGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    );

DWORD
LWSetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
LWSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
LWDeleteConfigSection(
    PCFGSECTION* ppSectionList,
    PCSTR pszSectionName
    );

DWORD
LWDeleteNameValuePairBySection(
    PCFGSECTION pSection,
    PCSTR pszName
    );

DWORD
LWDeleteNameValuePairBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    );

#endif /* __CFGPARSER_H__ */
