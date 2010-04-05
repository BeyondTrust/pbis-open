/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#ifndef __CTCFGPARSER_H__
#define __CTCFGPARSER_H__

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
LWGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    );

DWORD
LWSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
LWGetConfigValueBySection(
    PCFGSECTION pSection,
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

#endif /* __CTCFGPARSER_H__ */
