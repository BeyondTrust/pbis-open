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
