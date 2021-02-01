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

typedef struct __LSA_SRV_API_CONFIG
{
    BOOLEAN bSawProviderAD;    // Not part of original code
    BOOLEAN bSawProviderLocal; // Not part of original code

    BOOLEAN bEnableEventLog;
    BOOLEAN bLogNetworkConnectionEvents;
    PSTR    pszLogLevel; /* Added; Original code uses a function directly. */
} LSA_SRV_API_CONFIG, *PLSA_SRV_API_CONFIG;

typedef struct _LSA_PAM_CONFIG
{
    PSTR pszLogLevel; // DWORD dwLogLevel;
    BOOLEAN bLsaPamDisplayMOTD;
    PSTR pszAccessDeniedMessage;
    PSTR pszActiveDirectoryPasswordPrompt;
    PSTR pszLocalPasswordPrompt;
    PSTR pszOtherPasswordPrompt;
} LSA_PAM_CONFIG, *PLSA_PAM_CONFIG;

typedef struct _LSA_AD_CONFIG {

    DWORD               dwCacheReaperTimeoutSecs;
    DWORD               dwCacheEntryExpirySecs;
    DWORD               dwCacheSizeCap;
    CHAR                chSpaceReplacement;
    CHAR                chDomainSeparator;
    BOOLEAN             bEnableEventLog;
    BOOLEAN             bShouldLogNetworkConnectionEvents;
    BOOLEAN             bCreateK5Login;
    BOOLEAN             bCreateHomeDir;
    BOOLEAN             bLDAPSignAndSeal;
    BOOLEAN             bAssumeDefaultDomain;
    BOOLEAN             bSyncSystemTime;
    BOOLEAN             bRefreshUserCreds;
    DWORD               dwMachinePasswordSyncLifetime;
    PSTR                pszShell;
    PSTR                pszHomedirPrefix;
    PSTR                pszHomedirTemplate;
    PSTR                pszUmask;
    PSTR                pszSkelDirs;
    PSTR                pszUnresolvedMemberList;
    PSTR                pszCellSupport;
    PSTR                pszCacheBackend;
    BOOLEAN             bTrimUserMembershipEnabled;
    BOOLEAN             bNssGroupMembersCacheOnlyEnabled;
    BOOLEAN             bNssUserMembershipCacheOnlyEnabled;
    BOOLEAN             bNssEnumerationEnabled;
    struct {
        DWORD           dwCheckDomainOnlineSeconds;
        DWORD           dwUnknownDomainCacheTimeoutSeconds;
    } DomainManager;
} LSA_AD_CONFIG, *PLSA_AD_CONFIG;


typedef struct __LOCAL_CONFIG
{
    BOOLEAN   bEnableEventLog;
    DWORD     dwMaxGroupNestingLevel;
    PSTR      pszLoginShell;
    PSTR      pszHomedirPrefix;
    PSTR      pszHomedirTemplate;
    BOOLEAN   bCreateHomedir;
    PSTR      pszUmask;
    PSTR      pszSkelDirs;

} LOCAL_CONFIG, *PLOCAL_CONFIG;

typedef struct __LSASS_CONFIG
{
    LSA_SRV_API_CONFIG LsaConfig;
    LSA_PAM_CONFIG     PamConfig;
    LSA_AD_CONFIG      ADConfig;
    LOCAL_CONFIG       LocalConfig;

    BOOLEAN bSawProviderAD;
    BOOLEAN bSawProviderLocal;
} LSASS_CONFIG, *PLSASS_CONFIG;

