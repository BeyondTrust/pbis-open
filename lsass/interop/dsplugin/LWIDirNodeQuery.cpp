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

#include "includes.h"
#include "LWIDirNodeQuery.h"

#define SHOW_DEBUG_SPEW 0 /* GlennC, quiet down the spew! */

#define LIST_SEPARATOR ", "
#define LIST_SEPARATOR_LENGTH (sizeof(LIST_SEPARATOR)-1)

// Ideally, Apple would have definitions for these:
#define DNI_TYPE "dsAttrTypeStandard:DirectoryNodeInfo"
#define DNI_NAME "DirectoryNodeInfo"
#define LWDSPLUGIN_CONF "/opt/likewise/etc/lwdsplugin.conf"

#define GET_NODE_STR(node) \
                    SAFE_LOG_STR((node) ? (node)->fBufferData : 0)

#define GET_NODE_LEN(node) \
                    ((node) ? (node)->fBufferLength : 0)

#define GET_NODE_SIZE(node) \
                    ((node) ? (node)->fBufferSize : 0)
                    
#ifdef DEBUG_PASSWORD
#define DEBUG_USER_PASSWORD(username, oldPassword, password) \
    do { \
        if (oldPassword) { \
            LOG_PARAM("username = \"%s\", oldPassword = \"%s\", newPassword = \"%s\"", \
                      username, oldPassword, password); \
        } else { \
            LOG_PARAM("username = \"%s\", password = \"%s\"", \
                      username, password); \
        } \
    } while (0);
#else
#define DEBUG_USER_PASSWORD(username, oldPassword, password) \
    do { \
        LOG_PARAM("username = \"%s\"", username); \
    } while (0)
#endif

#ifndef kDSStdRecordTypeComputerGroups /* For when building on Tiger based build machines */
/*!
 * @defined kDSStdRecordTypeComputerGroups
 * @discussion Identifies computer group records.
 */
#define kDSStdRecordTypeComputerGroups "dsRecTypeStandard:ComputerGroups"
#endif


LWIDirNodeQuery::DirNodeRefMap * LWIDirNodeQuery::_dirNodeRefMap = NULL;
LWIDirNodeQuery::AttributeRefMap * LWIDirNodeQuery::_attributeRefMap = NULL;

static long GetCountedString(tDataBufferPtr BufferData, size_t* Offset, char** Result, size_t* Length)
{
    long macError = eDSNoErr;
    int EE = 0;
    size_t offset = *Offset;
    char* result = NULL;
    int32_t length;

    LOG_ENTER("");

    if (!BufferData->fBufferData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }

    if (offset >= BufferData->fBufferLength)
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    if ((BufferData->fBufferLength - offset) < sizeof(int32_t))
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    memcpy(&length, BufferData->fBufferData + offset, sizeof(int32_t));
    offset += sizeof(int32_t);

    if (length < 0)
    {
        macError = eDSAuthInBuffFormatError;
        GOTO_CLEANUP_EE(EE);
    }

    result = (char*) malloc(length + 1);
    if (!result)
    {
        macError = eMemoryAllocError;
        GOTO_CLEANUP_EE(EE);
    }

    memcpy(result, BufferData->fBufferData + offset, length);
    result[length] = 0;
    offset += length;

    macError = eDSNoErr;

cleanup:

    if (macError)
    {
        if (result)
        {
            free(result);
            result = NULL;
        }
        length = 0;
        offset = *Offset;
    }

    *Result = result;
    if (Length)
    {
        *Length = length;
    }
    if (!macError)
    {
        *Offset = offset;
    }

    LOG_LEAVE("--> %d (EE = %d)", macError, EE);

    return macError;
}

static bool IsPluginRootPathPrefix(const char* path)
{
    return ( path &&
             (0 == strncmp(PLUGIN_ROOT_PATH, path, sizeof(PLUGIN_ROOT_PATH)-1)) &&
             ( (0 == path[sizeof(PLUGIN_ROOT_PATH)-1]) ||
               ('/' == path[sizeof(PLUGIN_ROOT_PATH)-1]) ) );
}

static bool IsPluginRoot(const char* path)
{
    return ( path &&
             (0 == strncmp(PLUGIN_ROOT_PATH, path, sizeof(PLUGIN_ROOT_PATH)-1)) &&
             (0 == path[sizeof(PLUGIN_ROOT_PATH)-1]) );
}

static
long CrackDirNodePath(
    PCSTR pszDirNodePath,
    PSTR* ppszDomainName,
    PSTR* ppszGPOName
    )
{
    long macError = 0;
    PSTR pTemp = NULL;
    char szDomainName[256];
    char szGPOName[256];
    PSTR pGPOName = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszGPOName = NULL;

    memset( szDomainName, 
            0, 
            sizeof(szDomainName));
    
    memset( szGPOName, 
            0, 
            sizeof(szGPOName));

    if (IsPluginRoot(pszDirNodePath))
    {
        macError = eDSInvalidName;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (!IsPluginRootPathPrefix(pszDirNodePath))
    {
        macError = eDSUnknownNodeName;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    if (strlen(PLUGIN_ROOT_PATH) + 1 >= strlen(pszDirNodePath))
    {
        macError = eDSInvalidName;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pTemp = (PSTR) pszDirNodePath + sizeof(PLUGIN_ROOT_PATH);
    pGPOName = strchr(pTemp, '/');
    
    if (pGPOName == NULL)
    {
        macError = eDSInvalidName;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    strncpy( szDomainName, 
             pTemp, 
             pGPOName - pTemp);
    
    if (pGPOName[0] == '/')
    {
        pGPOName += 1; /* Skip past the '/' in the DirNodePath */
    }

    macError = LwAllocateString( szDomainName, 
                                &pszDomainName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LwAllocateString( pGPOName,
                                &pszGPOName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    *ppszDomainName = pszDomainName;
    *ppszGPOName = pszGPOName;

    return macError;

cleanup:

    if (pszDomainName) {
        LW_SAFE_FREE_STRING(pszDomainName);
    }

    if (pszGPOName) {
        LW_SAFE_FREE_STRING(pszGPOName);
    }

    *ppszDomainName = NULL;
    *ppszGPOName = NULL;

    return macError;
}

static
long
CheckAccountPolicy(
    BOOLEAN bLogAll,
    PCSTR username,
    tDataBufferPtr policyBuffer
    )
{
    long macError = eDSNoErr;
    DWORD dwDaysToPasswordExpiry = 0;
    BOOLEAN bDisabled = FALSE;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bPasswordNeverExpires = FALSE;
    BOOLEAN bPasswordExpired = FALSE;
    BOOLEAN bPromptForPasswordChange = FALSE;
    BOOLEAN bUserCanChangePassword = FALSE;
    BOOLEAN bLogonRestriction = FALSE;
    UInt32 length;

    LOG("Going to check account policy for user %s",
        username ? username : "<null>");
    macError = GetUserAccountPolicy(username,
                                    &dwDaysToPasswordExpiry,
                                    &bDisabled,
                                    &bExpired,
                                    &bLocked,
                                    &bPasswordNeverExpires,
                                    &bPasswordExpired,
                                    &bPromptForPasswordChange,
                                    &bUserCanChangePassword,
                                    &bLogonRestriction);
    if (macError)
    {
        LOG("Failed to determine account policy for user %s, error: %d",
            username ? username : "<null>",
            macError);
        goto exit;
    }

    if (bLogAll)
    {
        LOG("Account policy for user %s",
            username ? username : "<null>");
        LOG("Days till password expires: %d", dwDaysToPasswordExpiry);
        LOG("Account disabled:           %s", bDisabled ? "TRUE" : "FALSE");
        LOG("Account expired:            %s", bExpired ? "TRUE" : "FALSE");
        LOG("Account locked:             %s", bLocked ? "TRUE" : "FALSE");
        LOG("Password never expires:     %s", bPasswordNeverExpires ? "TRUE" : "FALSE");
        LOG("Password expired:           %s", bPasswordExpired ? "TRUE" : "FALSE");
        LOG("Prompt for password change: %s", bPromptForPasswordChange ? "YES" : "NO");
        LOG("User can change password:   %s", bUserCanChangePassword ? "YES" : "NO");
        LOG("Logon restriction:          %s", bLogonRestriction ? "YES" : "NO");
    }

    length = snprintf(
                policyBuffer->fBufferData + sizeof(UInt32),
                (int) policyBuffer->fBufferSize - sizeof(UInt32),
                "newPasswordRequired=%d",
                bPasswordExpired);
    if (length > policyBuffer->fBufferSize)
    {
        macError = eDSBufferTooSmall;
        goto exit;
    }

    *((UInt32 *) policyBuffer->fBufferData) = length;
    policyBuffer->fBufferLength = length + sizeof(UInt32);

    if (bLocked)
    {
        LOG("Account policy fails due to account locked for user %s",
            username ? username : "<null>");
        //dwError = LW_ERROR_ACCOUNT_LOCKED;
        macError = eDSAuthFailed;
        goto exit;
    }

    if (bDisabled)
    {
        LOG("Account policy fails due to account disabled for user %s",
            username ? username : "<null>");
        //dwError = LW_ERROR_ACCOUNT_DISABLED;
        macError = eDSAuthAccountDisabled;
        goto exit;
    }

    if (bExpired)
    {
        LOG("Account policy fails due to account expired for user %s",
            username ? username : "<null>");
        //dwError = LW_ERROR_ACCOUNT_EXPIRED;
        macError = eDSAuthAccountExpired;
        goto exit;
    }

    if (bUserCanChangePassword &&
        bPromptForPasswordChange &&
        dwDaysToPasswordExpiry > 0 &&
        dwDaysToPasswordExpiry < 5 &&
        !bPasswordExpired)
    {
        LOG("Account policy warning, password about to expire for user %s (requesting password change)",
            username ? username : "<null>");
        macError = 0; // Safer to not react, as the password change is shown at first warning.
        goto exit;
    }

    if (bPasswordExpired)
    {
        LOG("Account policy fails due to password expired for user %s",
            username ? username : "<null>");
        //dwError = LW_ERROR_PASSWORD_EXPIRED;
        macError = eDSAuthPasswordExpired;
        goto exit;
    }

    if (bLogonRestriction)
    {
        LOG("Account policy fails due to logon restriction for user %s",
            username ? username : "<null>");
        //dwError = ERROR_ACCOUNT_RESTRICTION;
        macError = eDSAuthUnknownUser;
        goto exit;
    }

    LOG("Account policy okay for user %s",
            username ? username : "<null>");

exit:

    if (macError)
    {
        LOG("Logon account policy check failed for user %s with error: %d",
            username ? username : "<null>",
            macError);
    }

    return macError;
}

long
LWIDirNodeQuery::Initialize()
{
    long macError = eDSNoErr;
    
    _dirNodeRefMap = new DirNodeRefMap();
    if (!_dirNodeRefMap)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

    _attributeRefMap = new AttributeRefMap();
    if (!_attributeRefMap)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP();
    }

cleanup:

    if (macError)
    {
        Cleanup();
    }

    return macError;
}

static
long
CopyDirNode(
    PDSDIRNODE pDirNode,
    PDSDIRNODE * ppDirNodeRef
    )
{
    long macError = eDSNoErr;
    PDSDIRNODE pNewNode = NULL;
    
    if (!pDirNode)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = LwAllocateMemory(sizeof(DSDIRNODE), (PVOID*) &pNewNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (pDirNode->pszDirNodePath)
    {
        macError = LwAllocateString(pDirNode->pszDirNodePath, &pNewNode->pszDirNodePath);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pDirNode->pDirNodeGPO)
    {
        macError = GPACopyGPOObject(pDirNode->pDirNodeGPO, &pNewNode->pDirNodeGPO);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pDirNode->pszDirNodeUserUPN)
    {
        macError = LwAllocateString(pDirNode->pszDirNodeUserUPN, &pNewNode->pszDirNodeUserUPN);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pNewNode->fPlugInRootConnection = pDirNode->fPlugInRootConnection;
    
    *ppDirNodeRef = pNewNode;
    pNewNode = NULL;
    
cleanup:

    if (pNewNode)
    {
        FreeDirNode(pNewNode);
    }

    return macError;
}

void
FreeDirNode(
    PDSDIRNODE pDirNode
    )
{
    if (pDirNode)
    {
        if (pDirNode->pszDirNodePath)
        {
            free(pDirNode->pszDirNodePath);
            pDirNode->pszDirNodePath = NULL;
        }

        GPA_SAFE_FREE_GPO_LIST(pDirNode->pDirNodeGPO);
        pDirNode->pDirNodeGPO = NULL;

        if (pDirNode->pszDirNodeUserUPN)
        {
            free(pDirNode->pszDirNodeUserUPN);
            pDirNode->pszDirNodeUserUPN = NULL;
        }

        free(pDirNode);
    }
}

void
LWIDirNodeQuery::Cleanup()
{
    if (_dirNodeRefMap)
    {
        DirNodeRefMapIter iter;

        for(iter = _dirNodeRefMap->begin(); iter != _dirNodeRefMap->end(); ++iter)
        {
            FreeDirNode(iter->second);
        }

        delete _dirNodeRefMap;
        _dirNodeRefMap = NULL;
    }

    if (_attributeRefMap)
    {
        delete _attributeRefMap;
        _attributeRefMap = NULL;
    }
}

long
LWIDirNodeQuery::Open(sOpenDirNode * pOpenDirNode)
{
    long macError = eDSNoErr;
    PDSDIRNODE pDirNode = NULL;
    DirNodeRefMapIter iter;
    PSTR dirNodeName = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszGPOName = NULL;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    unsigned long count = dsDataListGetNodeCount( pOpenDirNode->fInDirNodeName );

    dirNodeName = dsGetPathFromList(0, pOpenDirNode->fInDirNodeName, "/");

    LOG_ENTER("fType = %d, fResult = %d, fInDirRef = %u, fInDirNodeName.count = [%d], fInDirNodeName = %s, fOutDirNodeRef = %u",
              pOpenDirNode->fType,
              pOpenDirNode->fResult,
              pOpenDirNode->fInDirRef,
              count,
              SAFE_LOG_STR(dirNodeName),
              pOpenDirNode->fOutNodeRef);

    // Check to see if the path contains our registered top level path
    if ( !IsPluginRootPathPrefix(dirNodeName) )
    {
        macError = eDSOpenNodeFailed;
        GOTO_CLEANUP();
    }

    /* Remove (if any) previously saved dirNodeRefMap for given node */
    iter = _dirNodeRefMap->find(pOpenDirNode->fOutNodeRef);
    if (iter != _dirNodeRefMap->end())
    {
        LOG("Reopening the same node reference?");
        FreeDirNode(iter->second);        
        _dirNodeRefMap->erase(iter);
    }
    
    if (!IsPluginRoot(dirNodeName))
    {
        macError = CrackDirNodePath(dirNodeName, &pszDomainName, &pszGPOName);
        GOTO_CLEANUP_ON_MACERROR(macError);
    
        macError = GetSpecificGPO(pszDomainName, pszGPOName, &pGPO);
        if (macError || pGPO == NULL)
        {
            LOG("LWEDSPlugin was unable to find GPO object representing directory node (%s)", dirNodeName);
            macError = eDSOpenNodeFailed;
        }
    }
    
    /* Now create a new dirNodeRefMap entry for the new open dir node */
    macError = LwAllocateMemory(sizeof(DSDIRNODE), (PVOID*) &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    pDirNode->pszDirNodePath = dirNodeName;
    dirNodeName = NULL;
    
    pDirNode->pDirNodeGPO = pGPO;
    pGPO = NULL;

    pDirNode->fPlugInRootConnection = IsPluginRoot(pDirNode->pszDirNodePath);

    /* Save off directory node reference and path name */
    LWIDirNodeQuery::_dirNodeRefMap->insert(std::make_pair(pOpenDirNode->fOutNodeRef, pDirNode));
    
    LOG("Saved Likewise dirnode reference: %d to (%s%s) for user: %s",
        pOpenDirNode->fOutNodeRef,
        pDirNode->pszDirNodePath,
        pDirNode->fPlugInRootConnection ? " [ROOT]" : "",
        pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "<Unknown>");
    pDirNode = NULL;
    
cleanup:

    if (dirNodeName)
    {
        free(dirNodeName);
    }
    
    if (pszDomainName)
    {
        LW_SAFE_FREE_STRING(pszDomainName);
    }
    
    if (pszGPOName)
    {
        LW_SAFE_FREE_STRING(pszGPOName);
    }
    
    GPA_SAFE_FREE_GPO_LIST(pGPO);

    FreeDirNode(pDirNode);

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIDirNodeQuery::DoDirNodeAuth(
    sDoDirNodeAuth* pDoDirNodeAuth,
    bool fIsJoined,
    PVOID pAllowAdminCheckData,
    LWE_DS_FLAGS Flags) 
{
    long macError = eDSNoErr;
    int EE = 0;
    size_t offset = 0;
    char* username = NULL;
    char* auth_user = NULL;
    char* oldPassword = NULL;
    char* password = NULL;
    bool isChangePassword = false;
    bool isSetPassword = false;
    bool isAuthPassword = false;
    bool isAuthOnly = false;
    bool isGetPolicy = false;
    PDSDIRNODE pDirNode = NULL;
    PSTR pszUPN = NULL;
    PSTR pszUserSamAccount = NULL;
    PSTR pszUserDomainFQDN = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN isOnlineLogon = false;

    LOG_ENTER("fType = %d, fInNodeRef = %u, fInAuthMethod = %s, fInDirNodeAuthOnlyFlag = %d, fInAuthStepData = @%p => { length = %d }, fResult = %d",
              pDoDirNodeAuth->fType,
              pDoDirNodeAuth->fInNodeRef,
              GET_NODE_STR(pDoDirNodeAuth->fInAuthMethod),
              pDoDirNodeAuth->fInDirNodeAuthOnlyFlag,
              pDoDirNodeAuth->fInAuthStepData,
              GET_NODE_LEN(pDoDirNodeAuth->fInAuthStepData),
              pDoDirNodeAuth->fResult);
    
    macError = GetDsDirNodeRef(pDoDirNodeAuth->fInNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    if (!pDoDirNodeAuth->fInAuthMethod || !pDoDirNodeAuth->fInAuthMethod->fBufferData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }

    isAuthOnly = pDoDirNodeAuth->fInDirNodeAuthOnlyFlag ? false : true; /* Meaning is opposite what you would expect...
                                                                           an interactive logon, or ssh logon will have
                                                                           fInDirNodeAuthOnlyFlag set to false. Workgroup Manager
                                                                           admin authentication will have this set to true. Therefore,
                                                                           to make the rest of the code in this function make more
                                                                           sense, we use isAuthOnly with meaning:
                                                                           true - user is just authenticating
                                                                           false - user is logging on and creating a session connection */
                                                            

    isAuthPassword = !(strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthClearText) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthCrypt) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthNodeNativeClearTextOK) &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, "dsAuthMethodStandard:dsAuthNodeNativeRetainCredential") &&
                       strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthNodeNativeNoClearText));

    isChangePassword = !isAuthPassword && !strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthChangePasswd);
    isSetPassword = !isAuthPassword && !strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, kDSStdAuthSetPasswd);
    isGetPolicy = !isAuthPassword && !strcmp(pDoDirNodeAuth->fInAuthMethod->fBufferData, "dsAuthMethodStandard:dsAuthGetEffectivePolicy");

    if (!isAuthPassword && !isChangePassword && !isSetPassword && !isGetPolicy)
    {
        macError = eDSAuthMethodNotSupported;
        GOTO_CLEANUP_EE(EE);
    }

    if (!pDoDirNodeAuth->fInAuthStepData)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_EE(EE);
    }

    macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &username, NULL);
    GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

    if (isChangePassword)
    {
        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &oldPassword, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }

    if (isSetPassword)
    {
        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &password, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &auth_user, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

        if (strcmp(username, auth_user) != 0)
        {
            LOG("dsAuthMethodStandard:dsAuthSetPasswd operation only supported when user and auth user are the same");
            macError = eDSAuthMethodNotSupported;
            GOTO_CLEANUP_EE(EE);
        }

        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &oldPassword, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }

    if (isChangePassword || isAuthPassword)
    {
        macError = GetCountedString(pDoDirNodeAuth->fInAuthStepData, &offset, &password, NULL);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }

    if (isGetPolicy)
    {
        if (!pDoDirNodeAuth->fOutAuthStepDataResponse)
        {
            macError = eDSNullDataBuff;
            GOTO_CLEANUP_EE(EE);
        }

        macError = CheckAccountPolicy(
                        TRUE,
                        username,
                        pDoDirNodeAuth->fOutAuthStepDataResponse);
        if (macError == eDSAuthPasswordExpired)
        {
            macError = eDSNoErr;
        }
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }
    else if (isChangePassword || isSetPassword)
    {
        LOG("Going to change password for user %s", username);
        DEBUG_USER_PASSWORD(username, oldPassword, password);

        macError = ChangePassword(username, oldPassword, password);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
    }
    else
    {
        LOG("Authenticating user for %s: %s AuthOnly: %s", pDirNode->fPlugInRootConnection ? "logon" : "admin", username, isAuthOnly ? "true" : "false");   
        macError = AuthenticateUser(username, password, isAuthOnly, &isOnlineLogon, &pszMessage);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);

        if (pDirNode->fPlugInRootConnection)
        {
            LOG("%sline logon successful for user: %s. Message: %s",
                (isOnlineLogon) ? "On" : "Off", username,
                pszMessage ? pszMessage : "<none>");   
        }
        else
        {
            LOG("%sline authentication successful for administrator: %s. Message: %s",
                (isOnlineLogon) ? "On" : "Off", username,
                pszMessage ? pszMessage : "<none>");   
        }

        // Also test to see if the logon should be rejected due to account policies
        macError = CheckAccountPolicy(
                        TRUE,
                        username,
                        pDoDirNodeAuth->fOutAuthStepDataResponse);
        GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
        
        macError = GetUserPrincipalNames(username, &pszUPN, &pszUserSamAccount, &pszUserDomainFQDN);
        if (macError == eDSNoErr && pszUPN)
        {
            /* Reassign this DirNode to a new user */
            if (pDirNode->pszDirNodeUserUPN)
            {
                LW_SAFE_FREE_STRING(pDirNode->pszDirNodeUserUPN);
                pDirNode->pszDirNodeUserUPN = NULL;
            }
            
            macError = LwAllocateString(pszUPN, &pDirNode->pszDirNodeUserUPN);
            GOTO_CLEANUP_ON_MACERROR_EE(macError, EE);
            
            /* Check to see if user should be a local admin */
            if (pAllowAdminCheckData)
            {
                BOOLEAN bIsInAdminGroup = LWIsUserInLocalGroup(username, "admin");
                    
                macError = CheckUserForAccess(username, pAllowAdminCheckData);
                if (macError)
                {
                    if (Flags & LWE_DS_FLAG_DONT_REMOVE_LOCAL_ADMINS)
                    {
                        if (bIsInAdminGroup)
                        {
                            LOG("User (%s) does not appear to be configured as a local admin per policy, local admin group entry will remain due to configuration to override normal rule to remove such members from the admin group.", username);
                        }
                    }
                    else
                    {
                        if (bIsInAdminGroup)
                        {
                            LOG("User (%s) does not appear to be configured as a local admin, going to remove user from local admin group", username);
                            macError = LWRemoveUserFromLocalGroup(username, "admin");
                            if (macError)
                            {
                                LOG("Failed to remove user (%s) from admin group with error: %d", username, macError);
                                macError = eDSNoErr;
                            }
                        }
                    }

                    macError = eDSNoErr;
                }
                else
                {
                    if (!bIsInAdminGroup)
                    {
                        LOG("User %s does appear to be configured as a local admin, going add user to local admin group", username);
                        macError = LWAddUserToLocalGroup(username, "admin");
                        if (macError)
                        {
                            LOG("Failed to add user (%s) to admin group with error: %d", username, macError);
                            macError = eDSNoErr;
                        }
                    }
                }
            }

            /* Add user to the local staff group also, if not already a member */
            if (!LWIsUserInLocalGroup(username, "staff"))
            {
                LOG("Going add user %s to local staff group", username);
                macError = LWAddUserToLocalGroup(username, "staff");
                if (macError)
                {
                    LOG("Failed to add user (%s) to staff group with error: %d", username, macError);
                    macError = eDSNoErr;
                }
            }

            /* Get AD user attributes and cache them for offline access */
            LOG("Going to collect current AD user attributes for user %s", username);
            macError = CollectCurrentADAttributesForUser(pszUPN, pszUserDomainFQDN, pszMessage, isOnlineLogon);
            if (macError)
            {
                LOG("Failed to get user (%s) AD attributes with error: %d", username, macError);
                macError = eDSNoErr;
            }
			
            /* Now handle notifying Group Policy agent about user logon activity */
            if (isAuthOnly == false)
            {
                if (isOnlineLogon)
                {
                    LOG("Going to notify Group Policy about user logon for %s (UPN: %s) (user account: %s  domain: %s)", username, pszUPN, pszUserSamAccount, pszUserDomainFQDN);
                    
                    macError = NotifyUserLogon(username);
                    if (macError)
                    {
                        LOG("User (%s) logon notify to Group Policy agent failed with error: %d", username, macError);
                        LOG("User logon will continue without returning error...");
                        macError = eDSNoErr;
                    }
                }
                else
                {
                    LOG("Offline user logon, therefore not going to notify Group Policy about user logon for %s (UPN: %s) (user account: %s  domain: %s)", username, pszUPN, pszUserSamAccount, pszUserDomainFQDN);
                }
            }
        }
    }

cleanup:

    if (pszUPN)
    {
        LW_SAFE_FREE_STRING(pszUPN);
    }

    if (pszUserSamAccount)
    {
        LW_SAFE_FREE_STRING(pszUserSamAccount);
    }

    if (pszUserDomainFQDN)
    {
        LW_SAFE_FREE_STRING(pszUserDomainFQDN);
    }

    if (pszMessage)
    {
        LW_SAFE_FREE_STRING(pszMessage);
    }
        
    if (oldPassword)
    {
        free(oldPassword);
    }

    if (password)
    {
        free(password);
    }

    if (auth_user)
    {
        free(auth_user);
    }

    if (username)
    {
        free(username);
    }

    LOG_LEAVE("--> %d (EE = %d)", macError, EE);

    return macError;
}

long
LWIDirNodeQuery::Close(sCloseDirNode * pCloseDirNode)
{
    long macError = eDSNoErr;
    PDSDIRNODE pDirNode = NULL;
    DirNodeRefMapIter iter;
    
    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %u",
              pCloseDirNode->fType,
              pCloseDirNode->fResult,
              pCloseDirNode->fInNodeRef);

    iter = _dirNodeRefMap->find(pCloseDirNode->fInNodeRef);
    if (iter != _dirNodeRefMap->end())
    {
        pDirNode = iter->second;
        if (pDirNode)
        {
            LOG_PARAM("LWIDirNodeQuery::Close(\"%s\"%s) on behalf of user: %s",
                      pDirNode->pszDirNodePath ? pDirNode->pszDirNodePath : "<Unknown>",
                      pDirNode->fPlugInRootConnection ? " [ROOT]" : "",
                      pDirNode->pszDirNodeUserUPN ? pDirNode->pszDirNodeUserUPN : "<Unauthorized>");
            if (!pDirNode->fPlugInRootConnection)
            {
                LOG("We are closing child dirnode handle, here we can close our Active Directory LDAP handle");
            }
            FreeDirNode(pDirNode);
        }

        _dirNodeRefMap->erase(iter);
    }

    LOG_LEAVE("--> %d", macError);

    return macError;
}

long
LWIDirNodeQuery::GetInfo(sGetDirNodeInfo * pGetDirNodeInfo, LWE_DS_FLAGS Flags, PNETADAPTERINFO pNetAdapterList)
{
    long macError = eDSNoErr;
    unsigned long bytesWritten = 0;
    unsigned long nRecordsWritten = 0;
    unsigned long TotalRecords = 0;
    unsigned long nAttributesWritten = 0;
    unsigned long attributeCount;
    char* attributesBuffer = NULL;
    char* attributes = NULL;
    LWIQuery* pQuery = NULL;
    PDSRECORD pRecord = NULL;
    bool bSetValue;
    PLWIBITVECTOR attributeSet = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    tDataListPtr nodeNameList = NULL;
    tDataNodePtr pNameNode = NULL;
    PDSDIRNODE pDirNode = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %u, fOutAttrListRef = %d",
              pGetDirNodeInfo->fType,
              pGetDirNodeInfo->fResult,
              pGetDirNodeInfo->fInNodeRef,
              pGetDirNodeInfo->fOutAttrListRef);

    attributeCount = dsDataListGetNodeCount(pGetDirNodeInfo->fInDirNodeInfoTypeList);

    attributesBuffer = dsGetPathFromList(0, pGetDirNodeInfo->fInDirNodeInfoTypeList, LIST_SEPARATOR);
    attributes = attributesBuffer ? (attributesBuffer + LIST_SEPARATOR_LENGTH) : NULL;

    LOG_PARAM("fInDirNodeInfoTypeList => { count = %d, items = \"%s\"}",
              attributeCount, SAFE_LOG_STR(attributes));
              
    macError = GetDsDirNodeRef(pGetDirNodeInfo->fInNodeRef, &pDirNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
          
    macError = LWIQuery::Create(!pGetDirNodeInfo->fInAttrInfoOnly,
                                false, // Rely on our error eDSBufferTooSmall to cause the caller to retry
                                pGetDirNodeInfo->fInNodeRef,
                                Flags,
                                pNetAdapterList,
                                &pQuery);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIAttrLookup::GetVector(pGetDirNodeInfo->fInDirNodeInfoTypeList, &attributeSet);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::BuildRecord(DNI_TYPE, DNI_NAME, pGetDirNodeInfo->fInNodeRef, &pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    bSetValue = !pGetDirNodeInfo->fInAttrInfoOnly;

    if (LWI_BITVECTOR_ISSET(attributeSet, LWIAttrLookup::idx_kDSAttributesAll))
    {
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll);
    }

    if (LWI_BITVECTOR_ISSET(attributeSet, LWIAttrLookup::idx_kDSAttributesStandardAll))
    {
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrAuthMethod);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDS1AttrReadOnlyNode);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrNodePath);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_kDSNAttrRecordType);
        LWI_BITVECTOR_SET(attributeSet, LWIAttrLookup::idx_K_DS_ATTR_TRUST_INFORMATION);
        // kDSNAttrSubNodes --> N/A
        // kDS1AttrDataStamp --> N/A
    }

    for (int iAttr = LWIAttrLookup::idx_unknown+1; iAttr < LWIAttrLookup::idx_sentinel; iAttr++)
    {
        if (LWI_BITVECTOR_ISSET(attributeSet, iAttr))
        {
            switch (iAttr)
            {
                case LWIAttrLookup::idx_kDSNAttrAuthMethod:
                {
                    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrAuthMethod, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        // These are the normal username + password authentication methods:
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthClearText);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthNodeNativeClearTextOK);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthNodeNativeNoClearText);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthCrypt);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        // We also support change password:
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdAuthChangePasswd);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }

                    nAttributesWritten++;
                    break;
                }
                case LWIAttrLookup::idx_kDS1AttrReadOnlyNode:
                {
                    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDS1AttrReadOnlyNode, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        if (pDirNode->fPlugInRootConnection)
                        {
                            /* The root DS plugin node is used for authentication and general queries */
                            macError = LWIQuery::SetAttributeValue(pAttribute, "ReadOnly");
                            GOTO_CLEANUP_ON_MACERROR(macError);
                        }
                        else
                        {
                            /* The non-root nodes we use to represent GPOs to store setting using Workgroup Manager */
                            macError = LWIQuery::SetAttributeValue(pAttribute, "ReadWrite");
                            GOTO_CLEANUP_ON_MACERROR(macError);
                        }
                    }

                    nAttributesWritten++;
                    break;
                }
                case LWIAttrLookup::idx_kDSNAttrNodePath:
                {
                    int nNodes;

                    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrNodePath, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        nodeNameList = dsDataListAllocate(0);
                        if ( !nodeNameList )
                        {
                            macError = eDSAllocationFailed;
                            GOTO_CLEANUP_ON_MACERROR(macError);
                        }

                        macError = dsBuildListFromPathAlloc(0, nodeNameList, PLUGIN_ROOT_PATH, "/");
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        nNodes = dsDataListGetNodeCount(nodeNameList);
                        for (int iNode = 0; iNode < nNodes; iNode++)
                        {
                            macError = dsDataListGetNodeAlloc(0,
                                                              nodeNameList,
                                                              iNode+1,
                                                              &pNameNode);
                            GOTO_CLEANUP_ON_MACERROR(macError);

                            macError = LWIQuery::SetAttributeValue(pAttribute, pNameNode->fBufferData);
                            GOTO_CLEANUP_ON_MACERROR(macError);

                            dsDataNodeDeAllocate(NULL, pNameNode);
                            pNameNode = NULL;
                        }
                    }

                    nAttributesWritten++;
                    break;
                }
                case LWIAttrLookup::idx_kDSNAttrRecordType:
                {
                    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, kDSNAttrRecordType, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeUsers);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                            
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeGroups);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                            
                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputerLists);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputerGroups);
                        GOTO_CLEANUP_ON_MACERROR(macError);

                        macError = LWIQuery::SetAttributeValue(pAttribute, kDSStdRecordTypeComputers);
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }

                    nAttributesWritten++;
                    break;
                }
                case LWIAttrLookup::idx_K_DS_ATTR_TRUST_INFORMATION:
                {
                    macError = LWIQuery::AddAttributeEx(&pAttribute, pRecord, K_DS_ATTR_TRUST_INFORMATION, NULL);
                    GOTO_CLEANUP_ON_MACERROR(macError);

                    if (bSetValue)
                    {
                        // K_DS_ATTR_TRUST_INFORMATION --> Is this correct?
                        macError = LWIQuery::SetAttributeValue(pAttribute, "FullTrust");
                        GOTO_CLEANUP_ON_MACERROR(macError);
                    }

                    nAttributesWritten++;
                    break;

                }
                case LWIAttrLookup::idx_kDSAttributesAll:
                case LWIAttrLookup::idx_kDSAttributesStandardAll:
                    // We assume that the other bits are already set.
                    break;

                default:
#ifdef SHOW_DEBUG_SPEW
                    LOG("Unsupported attribute index - %d", iAttr);
#endif
                    break;
            }
        }
    }

    macError = pQuery->CommitRecord(pRecord);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecord = NULL;

    macError = pQuery->WriteGDNIResponse(pGetDirNodeInfo->fOutDataBuff->fBufferData,
                                         pGetDirNodeInfo->fOutDataBuff->fBufferSize,
                                         bytesWritten,
                                         nRecordsWritten,
                                         TotalRecords);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetDirNodeInfo->fOutDataBuff->fBufferLength = bytesWritten;
    pGetDirNodeInfo->fOutAttrInfoCount = nAttributesWritten;

    if ( bytesWritten > 0 )
    {
#ifdef SHOW_ALL_DEBUG_SPEW
        LOG_BUFFER(pGetDirNodeInfo->fOutDataBuff->fBufferData, bytesWritten);
#endif
    }

cleanup:

    if (pNameNode)
    {
        dsDataNodeDeAllocate(0, pNameNode);
    }

    if (nodeNameList)
    {
        dsDataListDeallocate(0, nodeNameList);
        free(nodeNameList);
    }

    if (attributesBuffer)
    {
        free(attributesBuffer);
    }

    if (pRecord)
    {
        LWIQuery::FreeRecord(pRecord);
    }

    if (pQuery)
    {
        delete pQuery;
    }

    LOG_LEAVE("fOutAttrInfoCount = %d, fOutDataBuff => { length = %d, size = %d } --> %d",
              pGetDirNodeInfo->fOutAttrInfoCount,
              pGetDirNodeInfo->fOutDataBuff->fBufferLength,
              pGetDirNodeInfo->fOutDataBuff->fBufferSize,
              macError);

    return macError;
}

long
LWIDirNodeQuery::FindAttribute(
    PDSMESSAGE pMessage,
    unsigned long attrIndex,
    PDSATTRIBUTE * ppAttribute
    )
{
    long macError = eDSNoErr;
    PDSATTRIBUTE currentElement = NULL;
    unsigned long currentIndex = 0;

    if (pMessage && pMessage->pHeader && pMessage->pHeader->nRecords > 0)
    {
        PDSRECORD pRecord = pMessage->pRecordList;

        // we always just use the first record

        currentElement = pRecord->pAttributeListHead;
        currentIndex = 1;
        while (currentElement && (currentIndex < attrIndex))
        {
            currentElement = currentElement->pNext;
            currentIndex++;
        }
    }

    *ppAttribute = currentElement;

    macError = currentElement ? eDSNoErr : eDSAttributeNotFound;

    return macError;
}

long
LWIDirNodeQuery::GetAttributeInfo(
    PDSMESSAGE pMessage,
    unsigned long attrIndex,
    tAttributeEntryPtr* ppAttributeEntryPtr
    )
{
    long macError = eDSNoErr;
    tAttributeEntryPtr pAttributeEntryPtr = NULL;
    PDSATTRIBUTE pAttribute = NULL;

    macError = FindAttribute(pMessage, attrIndex, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeEntry(&pAttributeEntryPtr, pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

cleanup:

    if (macError)
    {
       LwFreeMemory(pAttributeEntryPtr);
       pAttributeEntryPtr = NULL;
    }

    *ppAttributeEntryPtr = pAttributeEntryPtr;

    return macError;
}

long
LWIDirNodeQuery::GetAttributeEntry(sGetAttributeEntry * pGetAttributeEntry)
{
    long macError = eDSNoErr;
    PDSMESSAGE pMessage = NULL;
    tAttributeEntryPtr pAttrInfoPtr = NULL;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fOutAttrValueListRef = %d, fInAttrInfoIndex = %d, fOutAttrInfoPtr = @%p",
              pGetAttributeEntry->fType,
              pGetAttributeEntry->fResult,
              pGetAttributeEntry->fInNodeRef,
              pGetAttributeEntry->fOutAttrValueListRef,
              pGetAttributeEntry->fInAttrInfoIndex,
              pGetAttributeEntry->fOutAttrInfoPtr);

    if (!pGetAttributeEntry->fInOutDataBuff)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeEntry->fInOutDataBuff->fBufferData, pGetAttributeEntry->fInOutDataBuff->fBufferLength);
#endif

    macError = LWIQuery::ReadResponse(pGetAttributeEntry->fInOutDataBuff->fBufferData,
                                      pGetAttributeEntry->fInOutDataBuff->fBufferLength,
                                      &pMessage);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!LWIDirNodeQuery::_attributeRefMap)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    macError = GetAttributeInfo(pMessage, pGetAttributeEntry->fInAttrInfoIndex, &pAttrInfoPtr);
    GOTO_CLEANUP_ON_MACERROR(macError);

    LWIDirNodeQuery::_attributeRefMap->insert(std::make_pair(pGetAttributeEntry->fOutAttrValueListRef,
                                                                        pGetAttributeEntry->fInAttrInfoIndex));

    pGetAttributeEntry->fOutAttrInfoPtr = pAttrInfoPtr;
    pAttrInfoPtr = NULL;

    LOG("fOutAttrInfoPtr = @%p => { fAttributeValueCount = %d, fAttributeDataSize = %d, fAttributeValueMaxSize = %d, fAttributeSignature.fBufferSize = %d }",
        pGetAttributeEntry->fOutAttrInfoPtr,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeValueCount,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeDataSize,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeValueMaxSize,
        pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferSize);

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferData,
               pGetAttributeEntry->fOutAttrInfoPtr->fAttributeSignature.fBufferLength);
#endif

cleanup:

    if (pMessage)
    {
       LWIQuery::FreeMessage(pMessage);
    }

    if (pAttrInfoPtr)
    {
       LwFreeMemory(pAttrInfoPtr);
    }

    LOG_LEAVE("fOutAttrValueListRef = %d, fOutAttrInfoPtr = @%p --> %d",
              pGetAttributeEntry->fOutAttrValueListRef,
              pGetAttributeEntry->fOutAttrInfoPtr,
              macError);

    return macError;
}

long
LWIDirNodeQuery::GetAttributeValue(sGetAttributeValue * pGetAttributeValue)
{
    long macError = eDSNoErr;
    PDSMESSAGE pMessage = NULL;
    unsigned long attrIndex = 0;
    tAttributeValueEntryPtr pAttributeValueEntry = NULL;
    PDSATTRIBUTE pAttribute = NULL;
    PDSATTRIBUTEVALUE pAttributeValue = NULL;
    std::map<long,long>::iterator iter;

    LOG_ENTER("fType = %d, fResult = %d, fInNodeRef = %d, fInOutDataBuff = @%p, fInAttrValueIndex = %d, fInAttrValueListRef = %d",
              pGetAttributeValue->fType,
              pGetAttributeValue->fResult,
              pGetAttributeValue->fInNodeRef,
              pGetAttributeValue->fInOutDataBuff,
              pGetAttributeValue->fInAttrValueIndex,
              pGetAttributeValue->fInAttrValueListRef);

    if (!pGetAttributeValue->fInOutDataBuff)
    {
        macError = eDSNullDataBuff;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeValue->fInOutDataBuff->fBufferData, pGetAttributeValue->fInOutDataBuff->fBufferLength);
#endif

    macError = LWIQuery::ReadResponse(pGetAttributeValue->fInOutDataBuff->fBufferData,
                                      pGetAttributeValue->fInOutDataBuff->fBufferLength,
                                      &pMessage);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (!LWIDirNodeQuery::_attributeRefMap)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    iter = LWIDirNodeQuery::_attributeRefMap->find(pGetAttributeValue->fInAttrValueListRef);
    if (iter != LWIDirNodeQuery::_attributeRefMap->end())
    {
        attrIndex = iter->second;
    }
    else
    {
        LOG_ERROR("Invalid attribute value list reference: %d", pGetAttributeValue->fInAttrValueListRef);
        macError = eDSInvalidAttrListRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    // We found the index of the attribute in which the current value is to be found
    macError = FindAttribute(pMessage, attrIndex, &pAttribute);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::FindAttributeValueByIndex(pAttribute, pGetAttributeValue->fInAttrValueIndex, &pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    macError = LWIQuery::CreateAttributeValueEntry(&pAttributeValueEntry, pAttributeValue);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pGetAttributeValue->fOutAttrValue = pAttributeValueEntry;
    pAttributeValueEntry = NULL;

    LOG("fOutAttrValue = @%p => { fAttributeValueID = 0x%08x, fAttributeValueData.fBufferSize = %d }",
        pGetAttributeValue->fOutAttrValue,
        pGetAttributeValue->fOutAttrValue->fAttributeValueID,
        pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferSize);
        
#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_BUFFER(pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferData,
               pGetAttributeValue->fOutAttrValue->fAttributeValueData.fBufferLength);
#endif

cleanup:

    if (pMessage)
    {
       LWIQuery::FreeMessage(pMessage);
    }

    if (pAttributeValueEntry)
    {
        LwFreeMemory(pAttributeValueEntry);
    }

    LOG_LEAVE("fOutAttrValue = @%p --> %d",
              pGetAttributeValue->fOutAttrValue,
              macError);

    return macError;
}

long
LWIDirNodeQuery::CloseValueList(sCloseAttributeValueList * pCloseAttributeValueList)
{
    long macError = eDSNoErr;

    LOG_ENTER("fType = %d, fResult = %d, fInAttributeValueListRef = %d",
              pCloseAttributeValueList->fType,
              pCloseAttributeValueList->fResult,
              pCloseAttributeValueList->fInAttributeValueListRef
              );


    LOG_LEAVE("fInAttributeValueListRef = %d --> %d",
              pCloseAttributeValueList->fInAttributeValueListRef,
              macError);

    return macError;
}

long
LWIDirNodeQuery::CloseAttributeList(sCloseAttributeList * pCloseAttributeList)
{
    long macError = eDSNoErr;

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_ENTER("fType = %d, fResult = %d, fInAttributeListRef = %d",
              pCloseAttributeList->fType,
              pCloseAttributeList->fResult,
              pCloseAttributeList->fInAttributeListRef
              );
#endif

    if (LWIDirNodeQuery::_attributeRefMap == NULL)
    {
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    LWIDirNodeQuery::_attributeRefMap->erase(pCloseAttributeList->fInAttributeListRef);

cleanup:

#ifdef SHOW_ALL_DEBUG_SPEW
    LOG_LEAVE("fInAttributeListRef = %d --> %d",
              pCloseAttributeList->fInAttributeListRef,
              macError);
#endif
    return macError;
}

long
LWIDirNodeQuery::GetDsDirNodeRef(long dirNodeRef, PDSDIRNODE* ppDirNode)
{
    long macError = eDSNoErr;
    DirNodeRefMapIter iter;
    PDSDIRNODE pDirNode = NULL;
    BOOLEAN fCopyNode = FALSE;

#ifdef SHOW_DIRNODE_DEBUG_SPEW
    LOG_ENTER("fInNodeRef = %u", dirNodeRef);
#endif

    iter = LWIDirNodeQuery::_dirNodeRefMap->find(dirNodeRef);
    if (iter != LWIDirNodeQuery::_dirNodeRefMap->end())
    {
        pDirNode = iter->second;
    }

    if (!pDirNode)
    {
        macError = eDSInvalidNodeRef;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
#ifdef SHOW_DIRNODE_DEBUG_SPEW
    LOG_PARAM("LWIDirNodeQuery::GetDsDirNodeRef(\"%s\") on behalf of user: %s uid: %d",
              pDirNode->pszDirNodePath ? pDirNode->pszDirNodePath : "<Empty?>",
              pDirNode->pszDirNodeUserName ? pDirNode->pszDirNodeUserName : "<Unauthorized>",
              pDirNode->UserUID);
#endif
    if (fCopyNode)
    {
        macError = CopyDirNode(pDirNode, ppDirNode);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else
    {
        *ppDirNode = pDirNode;
        pDirNode = NULL;
    }
      
    pDirNode = NULL; /* Pointer to map entry, no need to free */

cleanup:

#ifdef SHOW_DIRNODE_DEBUG_SPEW
    LOG_LEAVE("%d", macError);
#endif

    return macError;
}

