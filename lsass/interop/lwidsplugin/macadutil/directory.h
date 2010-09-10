
DWORD
ADUOpenDirectory(
    PCSTR   szDomain,
    PHANDLE phDirectory
    );

VOID
ADUCloseDirectory(
    HANDLE hDirectory
    );

DWORD
ADUReadObject(
    HANDLE hDirectory,
    PSTR szObjectDN,
    PCSTR* szAttributeList,
    LDAPMessage **res
    );

DWORD
ADUDirectorySearch(
    HANDLE hDirectory,
    PCSTR szObjectDN,
    int scope,
    PSTR szQuery,
    PCSTR* szAttributeList,
    LDAPMessage **res
    );

LDAPMessage*
ADUFirstLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    );

LDAPMessage*
ADUNextLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    );

LDAP *
ADUGetLDAPSession(
    HANDLE hDirectory
    );

DWORD
ADUGetLDAPString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    );

DWORD
ADUPutLDAPString(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    PSTR   pszValue
    );
    
DWORD
ADUGetLDAPUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR  pszFieldName,
    PDWORD pdwValue
    );

DWORD
ADUPutLDAPUInt32(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    DWORD  dwValue
    );

