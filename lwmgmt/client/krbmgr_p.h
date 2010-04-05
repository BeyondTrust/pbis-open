#ifndef __KRBMGR_P_H__
#define __KRBMGR_P_H__

DWORD
LWIOpenKeyTabServer(
    PCSTR      pszServerName,
    char **    ppszBindingString,
    handle_t * phKeyTabServer
    );

DWORD
LWIReadKeyTab(
    handle_t             hKeyTabServer,
    PCSTR                pszKeyTabPath,
    DWORD                dwLastRecordId,
    DWORD                nRecordsPerPage,
    LSA_KEYTAB_ENTRIES * KeyTabEntries
    );

DWORD
LWICountKeyTabEntries(
    handle_t hKeyTabServer,
    PCSTR    pszKeyTabPath,
    DWORD*   pdwCount
    );

DWORD
LWIWriteKeyTabEntry(
    handle_t         hKeyTabServer,
    PCSTR            pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    );

DWORD
LWIDeleteFromKeyTab(
    handle_t         hKeyTabServer,
    PCSTR            pszKeyTabPath,
    LSA_KEYTAB_ENTRY KeyTabEntry
    );

DWORD
LWIClearKeyTab(
    handle_t hKeyTabServer,
    PCSTR    pszKeyTabPath
    );

DWORD
LWICloseKeyTabServer(
    handle_t hKeyTabServer,
    char *   pszBindingString
    );

#endif

