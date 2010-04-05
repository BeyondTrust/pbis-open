#ifndef __GPLDAP_H__
#define __GPLDAP_H__

#define GPA_DISPLAY_NAME_ATTR            "displayName"
#define GPA_FLAGS_ATTR                   "flags"
#define GPA_FILESYS_PATH_ATTR            "gPCFileSysPath"
#define GPA_FUNCTIONALITY_VERSION_ATTR   "gPCFunctionalityVersion"
#define GPA_MACHINE_EXTENSION_NAMES_ATTR "gPCMachineExtensionNames"
#define GPA_USER_EXTENSION_NAMES_ATTR    "gPCUserExtensionNames"
#define GPA_WQL_FILTER_ATTR              "gPCWQLFilter"
#define GPA_VERSION_NUMBER_ATTR          "versionNumber"
#define GPA_GPLINK_ATTR                  "gPLink"
#define GPA_GPOPTIONS_ATTR               "gPOptions"
#define GPA_OBJECTGUID_ATTR              "objectGUID"

typedef struct _GPO_DIRECTORY_CONTEXT {
    LDAP *ld;
} GPO_DIRECTORY_CONTEXT, *PGPO_DIRECTORY_CONTEXT;

CENTERROR
GPOisJoinedToAD(
    BOOLEAN *pbIsJoinedToAD
    );

CENTERROR
GPOGetADDomain(
    char ** ppszDomain
    );

CENTERROR
GPOOpenDirectory(
    char *szDomain,
    char *szServerName,
    HANDLE * phDirectory
    );

CENTERROR
GPOOpenDirectory2(
    HANDLE* phDirectory,
    PSTR*   ppszMachineName,
    PSTR*   ppszADDomain
    );

CENTERROR
GPODirectorySearch(
    HANDLE hDirectory,
    char * szObjectDN,
    int scope,
    char * szQuery,
    char ** szAttributeList,
    LDAPMessage **res
    );

CENTERROR
GPOGetParentDN(
    char * pszObjectDN,
    char ** ppszParentDN
    );

CENTERROR
GPAGetLDAPString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PSTR* ppszValue
    );

CENTERROR
GPAGetLDAPUInt32(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PDWORD pdwValue
    );

CENTERROR
GPAFindComputerDN(
    HANDLE hDirectory,
    PSTR pszHostName,
    PSTR pszDomainName,
    PSTR *ppszComputerDN
    );

CENTERROR
GPAFindUserDN(
    HANDLE hDirectory,
    PSTR pszUserName,
    PSTR pszDomainName,
    PSTR *ppszUserDN
    );

VOID
GPAFreeUserAttributes(
    PGPUSER_AD_ATTRS pUserADAttrs
    );

CENTERROR
GPAGetUserAttributes(
    HANDLE hDirectory,
    PSTR pszUserSID,
    PSTR pszDomainName,
    PGPUSER_AD_ATTRS * ppUserADAttrs
    );

VOID
GPOCloseDirectory(
    HANDLE hDirectory
    );

#endif /* __GPLDAP_H */
