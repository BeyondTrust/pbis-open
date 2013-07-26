/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
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

