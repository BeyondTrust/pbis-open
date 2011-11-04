/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

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
