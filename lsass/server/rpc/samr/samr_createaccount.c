#include "includes.h"


static
DWORD
SamrSrvBuildHomedirPath(
    PCWSTR  pwszSamAccountName,
    PCWSTR  pwszDomainName,
    PWSTR  *ppHomedirPath
    );

static
DWORD
SamrSrvAddToDefaultAliases(
    handle_t       hBinding,
    DOMAIN_HANDLE  hDomain,
    PSID           pDomainSid,
    DWORD          dwRid
    );


NTSTATUS
SamrSrvCreateAccount(
    IN  handle_t          hBinding,
    IN  DOMAIN_HANDLE     hDomain,
    IN  UNICODE_STRING   *pAccountName,
    IN  DWORD             dwObjectClass,
    IN  DWORD             dwAccountFlags,
    IN  DWORD             dwAccessMask,
    OUT ACCOUNT_HANDLE   *phAccount,
    OUT PDWORD            pdwAccessGranted,
    OUT PDWORD            pdwRid
    )
{
    const wchar_t wszAccountDnFmt[] = L"CN=%ws,%ws";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    HANDLE hDirectory = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNetBIOSName[] = DS_ATTR_NETBIOS_NAME;
    WCHAR wszAttrHomedir[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrShell[] = DS_ATTR_SHELL;

    enum AttrValueIndex {
        ATTR_VAL_IDX_OBJECT_CLASS = 0,
        ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
        ATTR_VAL_IDX_COMMON_NAME,
        ATTR_VAL_IDX_ACCOUNT_FLAGS,
        ATTR_VAL_IDX_NETBIOS_NAME,
        ATTR_VAL_IDX_HOME_DIR,
        ATTR_VAL_IDX_SHELL,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_OBJECT_CLASS */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_SAM_ACCOUNT_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_COMMON_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_ACCOUNT_FLAGS */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_NETBIOS_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_HOME_DIR */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_SHELL */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        }
    };

    DIRECTORY_MOD ModObjectClass = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectClass,
        1,
        &AttrValues[ATTR_VAL_IDX_OBJECT_CLASS]
    };

    DIRECTORY_MOD ModSamAccountName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrSamAccountName,
        1,
        &AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD ModCommonName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrCommonName,
        1,
        &AttrValues[ATTR_VAL_IDX_COMMON_NAME]
    };

    DIRECTORY_MOD ModAccountFlags = {
        DIR_MOD_FLAGS_ADD,
        wszAttrAccountFlags,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS]
    };

    DIRECTORY_MOD ModNetBIOSName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrNetBIOSName,
        1,
        &AttrValues[ATTR_VAL_IDX_NETBIOS_NAME]
    };

    DIRECTORY_MOD ModHomeDir = {
        DIR_MOD_FLAGS_ADD,
        wszAttrHomedir,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DIR]
    };

    DIRECTORY_MOD ModShell = {
        DIR_MOD_FLAGS_ADD,
        wszAttrShell,
        1,
        &AttrValues[ATTR_VAL_IDX_SHELL]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];

    PWSTR pwszAccountName = NULL;
    PWSTR pwszParentDn = NULL;
    PWSTR pwszAccountDn = NULL;
    PSTR pszShell = NULL;
    PWSTR pwszHomedirPath = NULL;
    DWORD dwAccountDnLen = 0;
    size_t sCommonNameLen = 0;
    size_t sParentDnLen = 0;
    DWORD i = 0;
    UNICODE_STRING AccountName = {0};
    IDS Rids = {0};
    IDS Types = {0};
    DWORD dwRid = 0;
    DWORD dwAccountType = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessGranted = 0;

    memset(&Mods, 0, sizeof(Mods));

    pDomCtx  = (PDOMAIN_CONTEXT)hDomain;
    pConnCtx = pDomCtx->pConnCtx;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = LwAllocateMemory(sizeof(*pAccCtx),
                               OUT_PPVOID(&pAccCtx));
    BAIL_ON_LSA_ERROR(dwError);

    hDirectory   = pConnCtx->hDirectory;
    pwszParentDn = pDomCtx->pwszDn;

    dwError = LwAllocateWc16StringFromUnicodeString(
                                     &pwszAccountName,
                                     pAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&AccountName,
                                        pwszAccountName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Check if such account name already exists.
     */

    ntStatus = SamrSrvLookupNames(hBinding,
                                  hDomain,
                                  1,
                                  &AccountName,
                                  &Rids,
                                  &Types);
    if (ntStatus == STATUS_SUCCESS)
    {
        /* Account already exists - return error code */
        ntStatus = STATUS_USER_EXISTS;
    }
    else if (ntStatus == STATUS_NONE_MAPPED)
    {
        /* Account doesn't exists - proceed with creating it */
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (Rids.pIds)
    {
        SamrSrvFreeMemory(Rids.pIds);
    }

    if (Types.pIds)
    {
        SamrSrvFreeMemory(Types.pIds);
    }

    /*
     * Prepare account attributes and create the object
     */
    dwError = LwWc16sLen(pwszAccountName, &sCommonNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszParentDn, &sParentDnLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwAccountDnLen = sCommonNameLen +
                     sParentDnLen +
                     (sizeof(wszAccountDnFmt)/sizeof(wszAccountDnFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwAccountDnLen,
                               OUT_PPVOID(&pwszAccountDn));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszAccountDn, dwAccountDnLen, wszAccountDnFmt,
                    pwszAccountName,
                    pwszParentDn) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = SamrSrvConfigGetDefaultLoginShell(&pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvBuildHomedirPath(pwszAccountName,
                                      pDomCtx->pwszDomainName,
                                      &pwszHomedirPath);
    BAIL_ON_LSA_ERROR(dwError);

    AttrValues[ATTR_VAL_IDX_OBJECT_CLASS].data.ulValue
        = dwObjectClass;
    AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszAccountName;
    AttrValues[ATTR_VAL_IDX_COMMON_NAME].data.pwszStringValue
        = pwszAccountName;
    AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS].data.ulValue
        = (dwAccountFlags | ACB_DISABLED);
    AttrValues[ATTR_VAL_IDX_NETBIOS_NAME].data.pwszStringValue
        = pDomCtx->pwszDomainName;
    AttrValues[ATTR_VAL_IDX_HOME_DIR].data.pwszStringValue
        = pwszHomedirPath;
    AttrValues[ATTR_VAL_IDX_SHELL].data.pszStringValue
        = pszShell;

    Mods[i++] = ModObjectClass;
    Mods[i++] = ModSamAccountName;
    Mods[i++] = ModCommonName;
    Mods[i++] = ModNetBIOSName;

    if (dwObjectClass == DS_OBJECT_CLASS_USER)
    {
        Mods[i++] = ModAccountFlags;
        Mods[i++] = ModHomeDir;
        Mods[i++] = ModShell;
    }

    dwError = DirectoryAddObject(hDirectory,
                                 pwszAccountDn,
                                 Mods);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvLookupNames(hBinding,
                                  hDomain,
                                  1,
                                  &AccountName,
                                  &Rids,
                                  &Types);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwRid         = Rids.pIds[0];
    dwAccountType = Types.pIds[0];

    /*
     * Create default security descriptor
     */
    ntStatus = SamrSrvCreateNewAccountSecurityDescriptor(
                                  pDomCtx->pDomainSid,
                                  dwRid,
                                  dwObjectClass,
                                  &pSecDesc);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectorySetEntrySecurityDescriptor(
                                  hDirectory,
                                  pwszAccountDn,
                                  pSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    if (!RtlAccessCheck(pSecDesc,
                        pConnCtx->pUserToken,
                        dwAccessMask,
                        0,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (dwObjectClass == DS_OBJECT_CLASS_USER)
    {
        dwError = SamrSrvAddToDefaultAliases(
                                     hBinding,
                                     hDomain,
                                     pDomCtx->pDomainSid,
                                     dwRid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pAccCtx->Type            = SamrContextAccount;
    pAccCtx->refcount        = 1;
    pAccCtx->dwAccessGranted = dwAccessGranted;
    pAccCtx->pwszDn          = pwszAccountDn;
    pAccCtx->pwszName        = pwszAccountName;
    pAccCtx->dwRid           = dwRid;
    pAccCtx->dwAccountType   = dwAccountType;

    pAccCtx->pDomCtx         = pDomCtx;
    InterlockedIncrement(&pDomCtx->refcount);

    *phAccount        = (ACCOUNT_HANDLE)pAccCtx;
    *pdwRid           = dwRid;
    *pdwAccessGranted = dwAccessGranted;

cleanup:
    LW_SAFE_FREE_MEMORY(pszShell);
    LW_SAFE_FREE_MEMORY(pwszHomedirPath);

    SamrSrvFreeUnicodeString(&AccountName);

    if (Rids.pIds)
    {
        SamrSrvFreeMemory(Rids.pIds);
    }

    if (Types.pIds)
    {
        SamrSrvFreeMemory(Types.pIds);
    }

    SamrSrvFreeSecurityDescriptor(&pSecDesc);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pAccCtx)
    {
        SamrSrvAccountContextFree(pAccCtx);
    }

    *phAccount        = NULL;
    *pdwAccessGranted = 0;
    *pdwRid           = 0;
    goto cleanup;
}


static
DWORD
SamrSrvBuildHomedirPath(
    PCWSTR pwszSamAccountName,
    PCWSTR pwszDomainName,
    PWSTR *ppwszHomedirPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszSamAccount = NULL;
    PWSTR pwszDomain = NULL;
    PSTR pszHomedirTemplate = NULL;
    PWSTR pwszHomedirTemplate = NULL;
    size_t sHomedirTemplateLen = 0;
    PWSTR pwszTemplateCursor = NULL;
    PSTR pszHomedirPrefix = NULL;
    PWSTR pwszHomedirPrefix = NULL;
    size_t sHomedirPrefixLen = 0;
    PSTR pszHostName = NULL;
    PWSTR pwszHostName = NULL;
    size_t sHostNameLen = 0;
    size_t sSamAccountNameLen = 0;
    size_t sDomainNameLen = 0;
    PWSTR pwszHomedirPath = NULL;
    DWORD dwHomedirPathLenAllowed = 0;
    DWORD dwOffset = 0;
    DWORD dwLenRemaining = 0;
    PWSTR pwszInsert = NULL;
    size_t sInsertLen = 0;

    dwError = LwAllocateWc16String(&pwszSamAccount,
                                   pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomain,
                                   pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvConfigGetHomedirTemplate(&pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszHomedirTemplate,
                           &pwszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszHomedirTemplate,
                         &sHomedirTemplateLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (strstr(pszHomedirTemplate, "%H"))
    {
        dwError = SamrSrvConfigGetHomedirPrefix(&pszHomedirPrefix);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszHomedirPrefix)
        {
            dwError = LwMbsToWc16s(pszHomedirPrefix,
                                   &pwszHomedirPrefix);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sLen(pwszHomedirPrefix,
                                 &sHomedirPrefixLen);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (strstr(pszHomedirTemplate, "%L"))
    {
        dwError = LsaDnsGetHostInfo(&pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszHostName)
        {
            dwError = LwMbsToWc16s(pszHostName,
                                   &pwszHostName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sLen(pwszHostName,
                                 &sHostNameLen);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LwWc16sLen(pwszSamAccount,
                         &sSamAccountNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszDomain,
                         &sDomainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwHomedirPathLenAllowed = (sHomedirTemplateLen +
                               sHomedirPrefixLen +
                               sHostNameLen +
                               sSamAccountNameLen +
                               sDomainNameLen +
                               1);

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwHomedirPathLenAllowed,
                               OUT_PPVOID(&pwszHomedirPath));
    BAIL_ON_LSA_ERROR(dwError);

    pwszTemplateCursor = pwszHomedirTemplate;
    dwLenRemaining = dwHomedirPathLenAllowed - dwOffset - 1;

    while (pwszTemplateCursor[0] &&
           dwLenRemaining > 0)
    {
        if (pwszTemplateCursor[0] == (WCHAR)('%'))
        {
            switch (pwszTemplateCursor[1])
            {
            case (WCHAR)('D'):
                pwszInsert = pwszDomain;
                sInsertLen = sDomainNameLen;

                dwError = LwWc16sToUpper(pwszInsert);
                BAIL_ON_LSA_ERROR(dwError);
                break;

            case (WCHAR)('U'):
                pwszInsert = pwszSamAccount;
                sInsertLen = sSamAccountNameLen;

                dwError = LwWc16sToLower(pwszInsert);
                BAIL_ON_LSA_ERROR(dwError);
                break;

            case (WCHAR)('H'):
                pwszInsert = pwszHomedirPrefix;
                sInsertLen = sHomedirPrefixLen;
                break;

            case (WCHAR)('L'):
                pwszInsert = pwszHostName;
                sInsertLen = sHostNameLen;

            default:
                dwError = LW_ERROR_INVALID_HOMEDIR_TEMPLATE;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pwszTemplateCursor += 2;
        }
        else
        {
            PCWSTR pwszEnd = pwszTemplateCursor;
            while (pwszEnd[0] &&
                   pwszEnd[0] != (WCHAR)('%'))
            {
                pwszEnd++;
            }

            if (!pwszEnd)
            {
                dwError = LwWc16sLen(pwszTemplateCursor,
                                     &sInsertLen);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                sInsertLen = pwszEnd - pwszTemplateCursor;
            }

            pwszInsert = pwszTemplateCursor;
            pwszTemplateCursor += sInsertLen;
        }

        memcpy(pwszHomedirPath + dwOffset,
               pwszInsert,
               sizeof(WCHAR) * sInsertLen);

        dwOffset += sInsertLen;
        dwLenRemaining = dwHomedirPathLenAllowed - dwOffset - 1;
    }

    LSA_ASSERT(dwOffset < dwHomedirPathLenAllowed);

    pwszHomedirPath[dwOffset] = 0;
    dwOffset++;

    *ppwszHomedirPath = pwszHomedirPath;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSamAccount);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pszHomedirTemplate);
    LW_SAFE_FREE_MEMORY(pwszHomedirTemplate);
    LW_SAFE_FREE_MEMORY(pszHomedirPrefix);
    LW_SAFE_FREE_MEMORY(pwszHomedirPrefix);
    LW_SAFE_FREE_MEMORY(pszHostName);
    LW_SAFE_FREE_MEMORY(pwszHostName);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszHomedirPath);

    *ppwszHomedirPath = NULL;
    goto cleanup;
}


static
DWORD
SamrSrvAddToDefaultAliases(
    handle_t       hBinding,
    DOMAIN_HANDLE  hDomain,
    PSID           pDomainSid,
    DWORD          dwRid
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwUserSidLen = 0;
    PSID pUserSid = NULL;
    ACCOUNT_HANDLE hAlias = NULL;

    dwUserSidLen = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);
    dwError = LwAllocateMemory(dwUserSidLen,
                               OUT_PPVOID(&pUserSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCopySid(dwUserSidLen,
                          pUserSid,
                          pDomainSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlAppendRidSid(dwUserSidLen,
                               pUserSid,
                               dwRid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* Add user account to "Likewise Users" alias */
    ntStatus = SamrSrvOpenAlias(hBinding,
                                hDomain,
                                ALIAS_ACCESS_ADD_MEMBER |
                                ALIAS_ACCESS_REMOVE_MEMBER,
                                DOMAIN_ALIAS_RID_LW_USERS,
                                &hAlias);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvAddAliasMember(hBinding,
                                     hAlias,
                                     pUserSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    if (hAlias)
    {
        SamrSrvClose(hBinding, &hAlias);
    }

    LW_SAFE_FREE_MEMORY(pUserSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
