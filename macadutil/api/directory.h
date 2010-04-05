
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
    char * szObjectDN,
    char ** szAttributeList,
    LDAPMessage **res
    );

DWORD
ADUAllocateGPObject(
    char * szPolicyDN,
    PGROUP_POLICY_OBJECT*  ppGPOObject
    );

VOID
ADUFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObjectList
    );

DWORD
ADUDirectorySearch(
    HANDLE hDirectory,
    char * szObjectDN,
    int scope,
    char * szQuery,
    char ** szAttributeList,
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
    PSTR pszFieldName,
    PSTR* ppszValue
    );

DWORD
ADUPutLDAPString(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PSTR   pszFieldName,
    PSTR   pszValue
    );
    
DWORD
ADUGetLDAPUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PDWORD pdwValue
    );

DWORD
ADUPutLDAPUInt32(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PSTR pszFieldName,
    DWORD dwValue
    );

