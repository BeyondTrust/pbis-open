#ifndef __GPADIRECTORY_H__
#define __GPADIRECTORY_H__

CENTERROR
GPOReadObject(
    HANDLE hDirectory,
    char * szObjectDN,
    char ** szAttributeList,
    LDAPMessage **res
    );

CENTERROR
GPOAllocateGPObject(
    char * szPolicyDN,
    PGROUP_POLICY_OBJECT*  ppGPOObject
    );

VOID
GPOFreeGPOObject(
    PGROUP_POLICY_OBJECT pGPOObject
    );
VOID
GPOFreeGPOList(
    PGROUP_POLICY_OBJECT pGPOObjectList
    );

LDAPMessage*
GPOFirstLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    );

LDAPMessage*
GPONextLDAPEntry(
    HANDLE hDirectory,
    LDAPMessage *res
    );

LDAP *
GPOGetLDAPSession(
    HANDLE hDirectory
    );

CENTERROR
GPAGetLDAPStrings(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR pszFieldName,
    PSTR** pppszValues,
    PDWORD pdwNumValues
    );

CENTERROR
GPAGetLDAPGUID(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR   pszFieldName,
    PSTR*  ppszGUID
    );

#endif /* __GPADIRECTORY_H__ */
