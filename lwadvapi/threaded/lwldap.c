/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwldap.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        LDAP API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "includes.h"
#include <gssapi/gssapi_krb5.h>

/* used by inet_addr, not defined on Solaris anywhere!? */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif

#ifndef SASL_CB_SERVERFQDN
#define SASL_CB_SERVERFQDN 0xc0004001
#endif

DWORD
LwCLdapOpenDirectory(
    IN PCSTR pszServerName,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LDAP * ld = NULL;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    int rc = LDAP_VERSION3;
    PSTR pszURL = NULL;

    LW_BAIL_ON_INVALID_STRING(pszServerName);
    
    dwError = LwAllocateStringPrintf(&pszURL, "cldap://%s",
                                        pszServerName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = ldap_initialize(&ld, pszURL);
    BAIL_ON_LDAP_ERROR(dwError);

    dwError = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &rc);
    BAIL_ON_LDAP_ERROR(dwError);

    dwError = ldap_set_option(ld, LDAP_OPT_REFERRALS, (void *)LDAP_OPT_OFF);
    BAIL_ON_LDAP_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pDirectory), OUT_PPVOID(&pDirectory));
    BAIL_ON_LW_ERROR(dwError);

    pDirectory->ld = ld;

error:
    LW_SAFE_FREE_STRING(pszURL);
    if (dwError)
    {
        if (pDirectory)
        {
            LwLdapCloseDirectory(pDirectory);
            pDirectory = NULL;
        }
    }

    *phDirectory = (HANDLE)pDirectory;

    return dwError;
}


DWORD
LwLdapPingTcp(
    PCSTR pszHostAddress,
    DWORD dwTimeoutSeconds
    )
{
    DWORD dwError = 0;
    int sysRet = 0;
    int fd = -1;
    struct in_addr addr;
    struct sockaddr_in socketAddress;
    struct timeval timeout;
    fd_set fds;
    int socketError;
#ifdef GETSOCKNAME_TAKES_SOCKLEN_T
    socklen_t socketErrorLength = 0;
#else
    int socketErrorLength = 0;
#endif

    addr.s_addr = inet_addr(pszHostAddress);
    if (addr.s_addr == INADDR_NONE)
    {
        LW_RTL_LOG_ERROR("Could not convert address'%s' to in_addr", pszHostAddress);
        dwError = LW_ERROR_DNS_RESOLUTION_FAILED;
        BAIL_ON_LW_ERROR(dwError);
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(389);
    socketAddress.sin_addr = addr;

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    sysRet = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (sysRet < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    sysRet = connect(fd, (struct sockaddr *)&socketAddress, sizeof(socketAddress));
    {
        dwError = LwMapErrnoToLwError(errno);
        // We typically expect EINPROGRESS
        dwError = (LW_ERROR_ERRNO_EINPROGRESS == dwError) ? 0 : dwError;
        BAIL_ON_LW_ERROR(dwError);
    }

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    timeout.tv_sec = dwTimeoutSeconds;
    timeout.tv_usec = 0;

    sysRet = select(fd + 1, NULL, &fds, NULL, &timeout);
    if (sysRet < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    switch (sysRet)
    {
        case 0:
            // We timed out
            LW_RTL_LOG_DEBUG("Timed out connecting to '%s'", pszHostAddress);
            // ISSUE-2008/09/16-dalmeida -- Technically, not a "domain"...
            dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
            BAIL_ON_LW_ERROR(dwError);
            break;
        case 1:
            // Normal case
            break;
        default:
            // This should never happen.
            LW_RTL_LOG_DEBUG("Unexpected number of file descriptors returned (%d)", sysRet);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
            break;
    }

    if (!FD_ISSET(fd, &fds))
    {
        // ISSUE-2008/07/15-dalmeida -- Suitable error code?
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    socketError = 0;
    socketErrorLength = sizeof(socketError);
    sysRet = getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError,
                        &socketErrorLength);
    if (sysRet < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    if (socketErrorLength != sizeof(socketError))
    {
        dwError = LW_ERROR_ERRNO_EMSGSIZE;
        BAIL_ON_LW_ERROR(dwError);
    }

    if (socketError)
    {
        dwError = LwMapErrnoToLwError(socketError);
        BAIL_ON_LW_ERROR(dwError);
    }

error:
    if (fd != -1)
    {
        close(fd);
    }

    return dwError;
}

DWORD
LwLdapOpenDirectoryServerSingleAttempt(
    IN PCSTR pszServerAddress,
    IN PCSTR pszServerName,
    IN DWORD dwTimeoutSec,
    IN DWORD dwFlags,
    OUT PLW_LDAP_DIRECTORY_CONTEXT* ppDirectory
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LDAP * ld = NULL;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    int rc = LDAP_VERSION3;
    DWORD dwPort = 389;
    struct timeval timeout = {0};
    BOOLEAN bLdapSeal = FALSE;

    timeout.tv_sec = dwTimeoutSec;

    LW_BAIL_ON_INVALID_STRING(pszServerName);
    LW_BAIL_ON_INVALID_STRING(pszServerAddress);

    if (dwFlags & LW_LDAP_OPT_GLOBAL_CATALOG)
    {
       dwPort = 3268;
    }

    // This creates the ld without immediately connecting to the server.
    // That way a connection timeout can be set first.
    ld = (LDAP *)ldap_init(pszServerAddress, dwPort);
    if (!ld) {
        dwError = LwMapErrnoToLwError(errno);
        LW_RTL_LOG_ERROR("Failed to open LDAP connection to domain controller");
        BAIL_ON_LW_ERROR(dwError);
        LW_RTL_LOG_ERROR("Failed to get errno for failed open LDAP connection");
        dwError = LW_ERROR_LDAP_ERROR;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
    BAIL_ON_LDAP_ERROR(dwError);

    dwError = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &rc);
    if (dwError) {
        LW_RTL_LOG_ERROR("Failed to set LDAP option protocol version");
        BAIL_ON_LDAP_ERROR(dwError);
    }

    dwError = ldap_set_option( ld, LDAP_OPT_REFERRALS, (void *)LDAP_OPT_OFF);
    if (dwError) {
        LW_RTL_LOG_ERROR("Failed to set LDAP option to not follow referrals");
        BAIL_ON_LDAP_ERROR(dwError);
    }

    /* This tells ldap to retry when select returns with EINTR */
    dwError = ldap_set_option( ld, LDAP_OPT_RESTART, (void *)LDAP_OPT_ON);
    if (dwError) {
        LW_RTL_LOG_ERROR("Failed to set LDAP option to auto retry ");
        BAIL_ON_LDAP_ERROR(dwError);
    }

    if (dwFlags & LW_LDAP_OPT_SIGN_AND_SEAL)
    {
        bLdapSeal = TRUE;
    }

    dwError = LwAllocateMemory(sizeof(*pDirectory), OUT_PPVOID(&pDirectory));
    BAIL_ON_LW_ERROR(dwError);

    pDirectory->ld = ld;
    ld = NULL;

    if (dwFlags & LW_LDAP_OPT_ANNONYMOUS)
    {
        dwError = LwLdapBindDirectoryAnonymous((HANDLE)pDirectory);
    }
    else
    {
        dwError = LwLdapBindDirectory(
                      (HANDLE)pDirectory,
                      pszServerName,
                      bLdapSeal);
    }
    // The above functions return -1 when a connection times out.
    if (dwError == (DWORD)-1)
    {
        dwError = ETIMEDOUT;
    }
    BAIL_ON_LW_ERROR(dwError);

    *ppDirectory = pDirectory;

cleanup:

    return(dwError);

error:

    if (pDirectory)
    {
        LwLdapCloseDirectory(pDirectory);
    }
    if (ld)
    {
        ldap_unbind_s(ld);
    }

    *ppDirectory = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LwLdapOpenDirectoryServer(
    IN PCSTR pszServerAddress,
    IN PCSTR pszServerName,
    IN DWORD dwFlags,
    OUT PHANDLE phDirectory
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwAttempt = 0;
    struct timespec sleepTime;
    DWORD dwTimeoutSec = 15;

    LW_BAIL_ON_INVALID_STRING(pszServerName);
    LW_BAIL_ON_INVALID_STRING(pszServerAddress);

    for (dwAttempt = 1; dwAttempt <= 3; dwAttempt++)
    {
        // dwTimeoutSec controls how long openldap will wait for the connection
        // to be established. For the first attempt, this is set to 15 seconds
        // (which is the same amount of time netlogon will wait to get the dc
        // name). The second attempt halves the value to 7 seconds. The third
        // attempt halves it again to 3 seconds.
        dwError = LwLdapOpenDirectoryServerSingleAttempt(
                        pszServerAddress,
                        pszServerName,
                        dwTimeoutSec,
                        dwFlags,
                        &pDirectory);
        if (dwError == ETIMEDOUT)
        {
            LW_ASSERT(pDirectory == NULL);
            LW_RTL_LOG_ERROR("The ldap connection to %s was disconnected. This was attempt #%d",
                    pszServerAddress,
                    dwAttempt);
            dwTimeoutSec /= 2;

            // This is the amount of time to sleep before trying to reconnect
            // again. It is: .1 seconds * dwAttempt
            sleepTime.tv_sec = 0;
            sleepTime.tv_nsec = dwAttempt * 100000000;
            while (nanosleep(&sleepTime, &sleepTime) == -1)
            {
                if (errno != EINTR)
                {
                    dwError = LwMapErrnoToLwError(errno);
                    BAIL_ON_LW_ERROR(dwError);
                }
            }
            continue;
        }
        BAIL_ON_LW_ERROR(dwError);
        break;
    }

    *phDirectory = (HANDLE)pDirectory;

cleanup:

    return(dwError);

error:

    if (pDirectory)
    {
        LwLdapCloseDirectory(pDirectory);
    }

    *phDirectory = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LwLdapBindDirectoryAnonymous(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    LW_BAIL_ON_INVALID_HANDLE(hDirectory);

    dwError = ldap_bind_s(
                    pDirectory->ld,
                    NULL,
                    NULL,
                    LDAP_AUTH_SIMPLE);
    BAIL_ON_LDAP_ERROR(dwError);

cleanup:

    return dwError;

error:

    LW_RTL_LOG_ERROR("Failed on LDAP simple bind (Error code: %u)", dwError);

    if(pDirectory->ld != NULL)
    {
        ldap_unbind_s(pDirectory->ld);
        pDirectory->ld = NULL;
    }

    goto cleanup;
}

DWORD
LwLdapBindDirectory(
    HANDLE hDirectory,
    PCSTR pszServerName,
    BOOLEAN bSeal 
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    PSTR pszTargetPrinciaplName = NULL;
    BOOLEAN bNeedCredentials = FALSE;

    // Leave the realm empty so that kerberos referrals are turned on.
    dwError = LwAllocateStringPrintf(&pszTargetPrinciaplName, "ldap/%s@", pszServerName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwKrb5CheckInitiatorCreds(pszTargetPrinciaplName, &bNeedCredentials);
    BAIL_ON_LW_ERROR(dwError);

    if (bNeedCredentials)
    {
        dwError = SEC_E_NO_CREDENTIALS;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwLdapBindDirectorySasl(pDirectory->ld, pszServerName, bSeal);
    BAIL_ON_LW_ERROR(dwError);

error:
    LW_SAFE_FREE_STRING(pszTargetPrinciaplName);

    return dwError;
}

static int
LwLdapGssSpnegoInteract(
    LDAP *ld,
    unsigned flags,
    void *defaults,
    void *in
){
    sasl_interact_t *interact = in;

    while (interact->id != SASL_CB_LIST_END)
    {
        switch (interact->id)
        {
            case SASL_CB_SERVERFQDN:
                // Supply the server's FQDN which is
                // required for Kerberos authentication

                interact->result = defaults;
                interact->len = strlen(interact->result);
                interact++;
                break;

            default:
                interact->result = "";
                interact->len = strlen(interact->result);
                interact++;
        }
    }

    return LDAP_SUCCESS;
}

DWORD
LwLdapBindDirectorySasl(
    LDAP *ld,
    PCSTR pszServerName,
    BOOLEAN bSeal
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    // Do not attempt to canonicalize the server
    // name which isn't necessary since we used
    // the server's FQDN or address.

    dwError = ldap_set_option(
                  ld,
                  LDAP_OPT_X_SASL_NOCANON,
                  LDAP_OPT_ON);
    BAIL_ON_LDAP_ERROR(dwError);

    // ssf=1 is sign, ssf>1 is seal.  By default
    // it will use the maximum available ssf level
    // so setting minssf isn't strictly necessary.
    // Setting minssf guarantees an error if it
    // cannot provide the minimum level.

    if (bSeal)
    {
        dwError = ldap_set_option(
                      ld,
                      LDAP_OPT_X_SASL_SECPROPS,
                      (void *)"minssf=2");
        BAIL_ON_LDAP_ERROR(dwError);
    }
    else
    {
        dwError = ldap_set_option(
                      ld,
                      LDAP_OPT_X_SASL_SECPROPS,
                      (void *)"minssf=1,maxssf=1");
        BAIL_ON_LDAP_ERROR(dwError);
    }

    dwError = ldap_sasl_interactive_bind_s(
                  ld,
                  NULL,
                  "GSS-SPNEGO",
                  NULL,
                  NULL,
                  LDAP_SASL_QUIET,
                  LwLdapGssSpnegoInteract,
                  (void *)pszServerName);
    if (dwError != 0) {
        LW_RTL_LOG_ERROR("ldap_sasl_interactive_bind_s failed with error code %d", dwError);
        BAIL_ON_LDAP_ERROR(dwError);
    }

error:

    return dwError;
}

void
LwLdapCloseDirectory(
    HANDLE hDirectory
    )
{
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    if (pDirectory) {
        if(pDirectory->ld)
        {
            ldap_unbind_s(pDirectory->ld);
        }
        LwFreeMemory(pDirectory);
    }
    return;
}

DWORD
LwLdapGetParentDN(
    PCSTR pszObjectDN,
    PSTR* ppszParentDN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszParentDN = NULL;
    PSTR pComma = NULL;

    if (!pszObjectDN || !*pszObjectDN) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pComma = strchr(pszObjectDN,',');
    if (!pComma) {
        dwError = LW_ERROR_LDAP_NO_PARENT_DN;
        BAIL_ON_LW_ERROR(dwError);
    }

    pComma++;

    dwError= LwAllocateString(pComma, &pszParentDN);
    BAIL_ON_LW_ERROR(dwError);

    *ppszParentDN = pszParentDN;

    return(dwError);

error:

    *ppszParentDN = NULL;

    return(dwError);
}

DWORD
LwLdapDirectorySearch(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR*  ppszAttributeList,
    LDAPMessage** ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    struct timeval timeout = {0};
    LDAPMessage* pMessage = NULL;

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    dwError = ldap_search_st(pDirectory->ld,
                             pszObjectDN,
                             scope,
                             pszQuery,
                             ppszAttributeList,
                             0,
                             &timeout,
                             &pMessage);
    if (dwError) {
        if (dwError == LDAP_NO_SUCH_OBJECT) {
            LW_RTL_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
            BAIL_ON_LDAP_ERROR(dwError);
        }
        if (dwError == LDAP_REFERRAL) {
            LW_RTL_LOG_ERROR("Caught LDAP_REFERRAL Error on ldap search");
            LW_RTL_LOG_ERROR("LDAP Search Info: DN: [%s]", LW_IS_NULL_OR_EMPTY_STR(pszObjectDN) ? "<null>" : pszObjectDN);
            LW_RTL_LOG_ERROR("LDAP Search Info: scope: [%d]", scope);
            LW_RTL_LOG_ERROR("LDAP Search Info: query: [%s]", LW_IS_NULL_OR_EMPTY_STR(pszQuery) ? "<null>" : pszQuery);
            if (ppszAttributeList) {
                size_t i;
                for (i = 0; ppszAttributeList[i] != NULL; i++) {
                    LW_RTL_LOG_ERROR("LDAP Search Info: attribute: [%s]", ppszAttributeList[i]);
                }
            }
            else {
                LW_RTL_LOG_ERROR("Error: LDAP Search Info: no attributes were specified");
            }
        }
        BAIL_ON_LDAP_ERROR(dwError);
    }

    *ppMessage = pMessage;

cleanup:

    return(dwError);

error:

    *ppMessage = NULL;

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    goto cleanup;
}

DWORD
LwLdapDirectorySearchEx(
    HANDLE hDirectory,
    PCSTR  pszObjectDN,
    int    scope,
    PCSTR  pszQuery,
    PSTR*  ppszAttributeList,
    LDAPControl** ppServerControls,
    DWORD  dwNumMaxEntries,
    LDAPMessage** ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    struct timeval timeout = {0};
    LDAPMessage* pMessage = NULL;

    // Set timeout to 60 seconds to be able to deal with large group
    // Instead of bailing on errors
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    dwError = ldap_search_ext_s(
                    pDirectory->ld,
                    pszObjectDN,
                    scope,
                    pszQuery,
                    ppszAttributeList,
                    0,
                    ppServerControls,
                    NULL,
                    &timeout,
                    dwNumMaxEntries,
                    &pMessage);
    if (dwError) {
        if (dwError == LDAP_NO_SUCH_OBJECT) {
            LW_RTL_LOG_VERBOSE("Caught LDAP_NO_SUCH_OBJECT Error on ldap search");
            BAIL_ON_LDAP_ERROR(dwError);
        }
        if (dwError == LDAP_REFERRAL) {
            LW_RTL_LOG_ERROR("Caught LDAP_REFERRAL Error on ldap search");
            LW_RTL_LOG_ERROR("LDAP Search Info: DN: [%s]", LW_IS_NULL_OR_EMPTY_STR(pszObjectDN) ? "<null>" : pszObjectDN);
            LW_RTL_LOG_ERROR("LDAP Search Info: scope: [%d]", scope);
            LW_RTL_LOG_ERROR("LDAP Search Info: query: [%s]", LW_IS_NULL_OR_EMPTY_STR(pszQuery) ? "<null>" : pszQuery);
            if (ppszAttributeList) {
                size_t i;
                for (i = 0; ppszAttributeList[i] != NULL; i++) {
                    LW_RTL_LOG_ERROR("LDAP Search Info: attribute: [%s]", ppszAttributeList[i]);
                }
            }
            else {
                LW_RTL_LOG_ERROR("Error: LDAP Search Info: no attributes were specified");
            }
        }
        BAIL_ON_LDAP_ERROR(dwError);
    }

    *ppMessage = pMessage;

cleanup:

    return(dwError);

error:

    *ppMessage = NULL;

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    goto cleanup;
}

DWORD
LwLdapEnablePageControlOption(
    HANDLE hDirectory
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    LDAPControl serverControl = {0};
    LDAPControl *ppServerPageCtrls[2] = {NULL, NULL};

    serverControl.ldctl_value.bv_val = NULL;
    serverControl.ldctl_value.bv_len = 0;
    serverControl.ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
    serverControl.ldctl_iscritical = 'T';

    ppServerPageCtrls[0] = &serverControl;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    dwError = ldap_set_option(pDirectory->ld,
                              LDAP_OPT_SERVER_CONTROLS,
                              OUT_PPVOID(&ppServerPageCtrls));
    BAIL_ON_LDAP_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwLdapDisablePageControlOption(
    HANDLE hDirectory
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    LDAPControl *ppServerPageCtrls[1] = {NULL};

    dwError = ldap_set_option(pDirectory->ld,
                              LDAP_OPT_SERVER_CONTROLS,
                              OUT_PPVOID(&ppServerPageCtrls));
    BAIL_ON_LDAP_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwLdapDirectoryOnePagedSearch(
    HANDLE         hDirectory,
    PCSTR          pszObjectDN,
    PCSTR          pszQuery,
    PSTR*          ppszAttributeList,
    DWORD          dwPageSize,
    PLW_SEARCH_COOKIE pCookie,
    int            scope,
    LDAPMessage**  ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    ber_int_t pageCount = 0;
    CHAR pagingCriticality = 'T';
    LDAPControl *pPageControl = NULL;
    LDAPControl *ppInputControls[2] = { NULL, NULL };
    LDAPControl **ppReturnedControls = NULL;
    int errorcodep = 0;
    LDAPMessage* pMessage = NULL;
    BOOLEAN bSearchFinished = FALSE;
    struct berval * pBerCookie = (struct berval *)pCookie->pvData;

    LW_ASSERT(pCookie->pfnFree == NULL || pCookie->pfnFree == LwLdapFreeCookie);
    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

   // dwError = ADEnablePageControlOption(hDirectory);
   // BAIL_ON_LW_ERROR(dwError);

    dwError = ldap_create_page_control(pDirectory->ld,
                                       dwPageSize,
                                       pBerCookie,
                                       pagingCriticality,
                                       &pPageControl);
    BAIL_ON_LDAP_ERROR(dwError);

    ppInputControls[0] = pPageControl;

    dwError = LwLdapDirectorySearchEx(
               hDirectory,
               pszObjectDN,
               scope,
               pszQuery,
               ppszAttributeList,
               ppInputControls,
               0,
               &pMessage);
    BAIL_ON_LW_ERROR(dwError);

    dwError = ldap_parse_result(pDirectory->ld,
                                pMessage,
                                &errorcodep,
                                NULL,
                                NULL,
                                NULL,
                                &ppReturnedControls,
                                0);
    BAIL_ON_LDAP_ERROR(dwError);

    if (pBerCookie != NULL)
    {
        ber_bvfree(pBerCookie);
        pBerCookie = NULL;
    }

    dwError = ldap_parse_page_control(pDirectory->ld,
                                      ppReturnedControls,
                                      &pageCount,
                                      &pBerCookie);
    BAIL_ON_LDAP_ERROR(dwError);

    if (pBerCookie == NULL || pBerCookie->bv_len < 1)
    {
        bSearchFinished = TRUE;
    }

    if (ppReturnedControls)
    {
       ldap_controls_free(ppReturnedControls);
       ppReturnedControls = NULL;
    }

    ppInputControls[0] = NULL;
    ldap_control_free(pPageControl);
    pPageControl = NULL;

    pCookie->bSearchFinished = bSearchFinished;
    *ppMessage = pMessage;
    pCookie->pvData = pBerCookie;
    pCookie->pfnFree = LwLdapFreeCookie;

cleanup:
  /*  dwError_disable = ADDisablePageControlOption(hDirectory);
    if (dwError_disable)
        LW_RTL_LOG_ERROR("Error: LDAP Disable PageControl Info: failed");*/

    if (ppReturnedControls) {
        ldap_controls_free(ppReturnedControls);
    }

    ppInputControls[0] = NULL;

    if (pPageControl) {
        ldap_control_free(pPageControl);
    }

    return (dwError);

error:

    *ppMessage = NULL;
    pCookie->pvData = NULL;
    pCookie->pfnFree = NULL;
    pCookie->bSearchFinished = TRUE;

    if (pBerCookie != NULL)
    {
        ber_bvfree(pBerCookie);
        pBerCookie = NULL;
    }

    goto cleanup;
}

LDAPMessage*
LwLdapFirstEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    )
{
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    return ldap_first_entry(pDirectory->ld, pMessage);
}

DWORD
LwLdapCountEntries(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PDWORD pdwCount
    )
{
    int iCount = 0;
    int err = 0;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwError = 0;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    iCount = ldap_count_entries(pDirectory->ld, pMessage);

    if (iCount < 0)
    {
        dwError = ldap_get_option(
                        pDirectory->ld,
                        LDAP_OPT_ERROR_NUMBER,
                        &err);
        BAIL_ON_LDAP_ERROR(dwError);
        dwError = err;
        BAIL_ON_LDAP_ERROR(dwError);
    }

    *pdwCount = iCount;

cleanup:
    return dwError;

error:
    *pdwCount = 0;
    goto cleanup;
}

DWORD
LwLdapModify(
    HANDLE hDirectory,
    PCSTR pszDN,
    LDAPMod** ppMods
    )
{
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    DWORD dwError = 0;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    dwError = ldap_modify_s(
                  pDirectory->ld,
                  pszDN,
                  ppMods);
    BAIL_ON_LDAP_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

LDAPMessage*
LwLdapNextEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage
    )
{
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    return ldap_next_entry(pDirectory->ld, pMessage);
}

LDAP *
LwLdapGetSession(
    HANDLE hDirectory
    )
{
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    return(pDirectory->ld);
}

DWORD
LwLdapGetBytes(
        HANDLE hDirectory,
        LDAPMessage* pMessage,
        PSTR pszFieldName,
        PBYTE* ppszByteValue,
        PDWORD pszByteLen
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    struct berval **ppszValues = NULL;
    PBYTE pszByteValue = NULL;
    DWORD szByteLen = 0;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = ldap_get_values_len(pDirectory->ld, pMessage, pszFieldName);

    if (ppszValues && ppszValues[0]){
        if (ppszValues[0]->bv_len != 0){
            dwError = LwAllocateMemory(
                        sizeof(BYTE) * ppszValues[0]->bv_len,
                        OUT_PPVOID(&pszByteValue));
            BAIL_ON_LW_ERROR(dwError);
            memcpy (pszByteValue, ppszValues[0]->bv_val, ppszValues[0]->bv_len * sizeof (BYTE));
            szByteLen = ppszValues[0]->bv_len;
        }
    }

    *ppszByteValue = pszByteValue;
    *pszByteLen = szByteLen;

cleanup:

    if (ppszValues) {
        ldap_value_free_len(ppszValues);
    }

    return dwError;

error:
    *ppszByteValue = NULL;
    *pszByteLen = 0;

    LW_SAFE_FREE_MEMORY(pszByteValue);

    goto cleanup;
}


DWORD
LwLdapGetString(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR pszFieldName,
    PSTR* ppszValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszValue = NULL;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    ppszValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszValues && ppszValues[0]) {
        dwError = LwAllocateString(ppszValues[0], &pszValue);
        BAIL_ON_LW_ERROR(dwError);
    }
    *ppszValue = pszValue;

cleanup:
    if (ppszValues) {
        ldap_value_free(ppszValues);
    }
    return dwError;

error:
    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LwLdapGetGUID(
    IN HANDLE       hDirectory,
    IN LDAPMessage* pMessage,
    IN PSTR         pszFieldName,
    OUT PSTR*       ppszGUID
    )
{
    DWORD                  dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    struct berval **                 ppValues = NULL;
    uint8_t                rawGUIDValue[16];
    PSTR                   pszValue = NULL;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    ppValues = ldap_get_values_len(
                            pDirectory->ld,
                            pMessage,
                            pszFieldName);
    if (!ppValues || ppValues[0]->bv_len != sizeof(rawGUIDValue))
    {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        BAIL_ON_LW_ERROR(dwError);
    }

    memcpy(
        rawGUIDValue,
        ppValues[0]->bv_val,
        sizeof(rawGUIDValue));

    dwError = LwAllocateStringPrintf(
                  &pszValue,
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  rawGUIDValue[3],
                  rawGUIDValue[2],
                  rawGUIDValue[1],
                  rawGUIDValue[0],
                  rawGUIDValue[5],
                  rawGUIDValue[4],
                  rawGUIDValue[7],
                  rawGUIDValue[6],
                  rawGUIDValue[8],
                  rawGUIDValue[9],
                  rawGUIDValue[10],
                  rawGUIDValue[11],
                  rawGUIDValue[12],
                  rawGUIDValue[13],
                  rawGUIDValue[14],
                  rawGUIDValue[15]);
    BAIL_ON_LW_ERROR(dwError);

    *ppszGUID = pszValue;
    pszValue = NULL;

cleanup:

    if (ppValues)
    {
        ldap_value_free_len(ppValues);
    }

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    goto cleanup;

}


DWORD
LwLdapGetDN(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR pszLdapValue = NULL;
    PSTR pszValue = NULL;

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    pszLdapValue = ldap_get_dn(pDirectory->ld, pMessage);
    if (LW_IS_NULL_OR_EMPTY_STR(pszLdapValue))
    {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwAllocateString(pszLdapValue, &pszValue);
    BAIL_ON_LW_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:
    if (pszLdapValue) {
        ldap_memfree(pszLdapValue);
    }
    return dwError;

error:
    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LwLdapIsValidADEntry(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PBOOLEAN pbValidADEntry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszValue = NULL;

    dwError = LwLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszValue);
    BAIL_ON_LW_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        BAIL_ON_LW_ERROR(dwError);
    }

    *pbValidADEntry = TRUE;

cleanup:

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    *pbValidADEntry = FALSE;

    goto cleanup;
}


DWORD
LwLdapGetUInt32(
    HANDLE       hDirectory,
    LDAPMessage* pMessage,
    PCSTR        pszFieldName,
    PDWORD       pdwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    dwError = LwLdapGetString(hDirectory, pMessage, pszFieldName, &pszValue);
    BAIL_ON_LW_ERROR(dwError);

    if (pszValue) {
        *pdwValue = atoi(pszValue);
    } else {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        // This error occurs very frequently (every time an unenabled user
        // or group is queried in default schema mode). So in order to avoid
        // log noise, BAIL_ON_LW_ERROR is not used here.
        goto error;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszValue);

    return dwError;

error:

    *pdwValue = 0;

    goto cleanup;
}

DWORD
LwLdapGetUInt64(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT UINT64* pqwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;
    PSTR pszEndPtr = NULL;

    dwError = LwLdapGetString(hDirectory, pMessage, pszFieldName, &pszValue);
    BAIL_ON_LW_ERROR(dwError);

    if (pszValue)
    {
#if SIZEOF_UNSIGNED_LONG == 8
        *pqwValue = strtoul(pszValue, &pszEndPtr, 10);
#else
        *pqwValue = strtoull(pszValue, &pszEndPtr, 10);
#endif
        if (pszEndPtr == NULL || pszEndPtr == pszValue || *pszEndPtr != '\0')
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_ERROR(dwError);
        }
    }
    else
    {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        // This error occurs very frequently (every time an unenabled user
        // or group is queried in default schema mode). So in order to avoid
        // log noise, BAIL_ON_LW_ERROR is not used here.
        goto error;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    *pqwValue = 0;
    goto cleanup;
}

DWORD
LwLdapGetInt64( 
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT int64_t * pqwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;
    PSTR pszEndPtr = NULL;
 
    dwError = LwLdapGetString(hDirectory, pMessage, pszFieldName, &pszValue);
    BAIL_ON_LW_ERROR(dwError);
 
    if (pszValue)
    {     
#if SIZEOF_LONG == 8
        *pqwValue = strtol(pszValue, &pszEndPtr, 10);
#else
        *pqwValue = strtoll(pszValue, &pszEndPtr, 10);
#endif
        if (pszEndPtr == NULL || pszEndPtr == pszValue || *pszEndPtr != '\0')
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_LW_ERROR(dwError);
        }   
    }       
    else    
    {
        dwError = LW_ERROR_INVALID_LDAP_ATTR_VALUE;
        // This error occurs very frequently (every time an unenabled user
        // or group is queried in default schema mode). So in order to avoid
        // log noise, BAIL_ON_LW_ERROR is not used here.
        goto error;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    *pqwValue = 0;
    goto cleanup;
}

// This utility function parse a ldap result in the format of
// <GUID=xxxxxxxx>;<SID=yyyyyyyyy>;distinguishedName (hexadecimal)
// It also handles the case when AD object does not have a SID,
// Hence, <GUID=xxxxxxxx>;distinguishedName
DWORD
LwLdapParseExtendedDNResult(
    IN PCSTR pszExtDnResult,
    OUT PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    PCSTR pszSidHex = NULL;
    PCSTR pszCurrExtDnResult = pszExtDnResult;
    DWORD dwSidLength = 0;
    PSTR pszSid = NULL;
    UCHAR* pucSIDByteArr = NULL;
    DWORD dwSIDByteCount = 0;
    PLW_SECURITY_IDENTIFIER pSID = NULL;

    LW_BAIL_ON_INVALID_STRING(pszCurrExtDnResult);

    if (strncasecmp(pszCurrExtDnResult, "<GUID=", sizeof("<GUID=")-1))
    {
        dwError = LW_ERROR_LDAP_ERROR;
        LW_RTL_LOG_ERROR("Failed to find extended DN entry '%s' GUID part. [error code:%d]",
                       pszExtDnResult, dwError);
        BAIL_ON_LW_ERROR(dwError);
    }

    while (*pszCurrExtDnResult != ';')
    {
        if (*pszCurrExtDnResult == '\0')
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
        }
        pszCurrExtDnResult++;
    }
    pszCurrExtDnResult++;

    if (strncasecmp(pszCurrExtDnResult, "<SID=", sizeof("<SID=")-1))
    {
        LW_RTL_LOG_DEBUG("The extended DN entry '%s' has no SID part.", pszExtDnResult);
        goto cleanup;
    }

    pszSidHex = pszCurrExtDnResult + sizeof("<SID=") - 1;

    while (*(pszSidHex+dwSidLength) != '>')
    {
        if (*(pszSidHex+dwSidLength) == '\0')
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
        }
        ++dwSidLength;
    }

    if (*(pszSidHex+dwSidLength+1) != ';')
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LwHexStrToByteArray(
                 pszSidHex,
                 &dwSidLength,
                 &pucSIDByteArr,
                 &dwSIDByteCount);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocSecurityIdentifierFromBinary(
                 pucSIDByteArr,
                 dwSIDByteCount,
                 &pSID);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwGetSecurityIdentifierString(
                 pSID,
                 &pszSid);
    BAIL_ON_LW_ERROR(dwError);

cleanup:
    if (dwError)
    {
        LW_SAFE_FREE_STRING(pszSid);
    }
    *ppszSid = pszSid;

    LW_SAFE_FREE_MEMORY(pucSIDByteArr);
    if (pSID)
    {
        LwFreeSecurityIdentifier(pSID);
    }

    return dwError;

error:
    // Do not actually handle any error here,
    // Do it in the cleanup, since there is a 'goto cleanup'

    goto cleanup;
}

DWORD
LwLdapGetStrings(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    )
{
    return LwLdapGetStringsWithExtDnResult(
            hDirectory,
            pMessage,
            pszFieldName,
            FALSE,
            pppszValues,
            pdwNumValues);
}

DWORD
LwLdapGetStringsWithExtDnResult(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    IN PCSTR pszFieldName,
    IN BOOLEAN bDoSidParsing,
    OUT PSTR** pppszValues,
    OUT PDWORD pdwNumValues
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR *ppszLDAPValues = NULL;
    PSTR *ppszValues = NULL;
    INT iNum = 0;
    DWORD dwNumValues = 0;
    int iValue = 0;

    LW_BAIL_ON_INVALID_HANDLE(hDirectory);
    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;
    LW_BAIL_ON_INVALID_POINTER(pMessage);

    ppszLDAPValues = (PSTR*)ldap_get_values(pDirectory->ld, pMessage, pszFieldName);
    if (ppszLDAPValues)
    {
        iNum = ldap_count_values(ppszLDAPValues);
        if (iNum < 0)
        {
            dwError = LW_ERROR_LDAP_ERROR;
            BAIL_ON_LW_ERROR(dwError);
        }
        else if (iNum > 0)
        {
            dwError = LwAllocateMemory((iNum+1)*sizeof(PSTR), OUT_PPVOID(&ppszValues));
            BAIL_ON_LW_ERROR(dwError);

            dwNumValues = 0;
            for (iValue = 0; iValue < iNum; iValue++)
            {
                if (bDoSidParsing)
                {
                    dwError = LwLdapParseExtendedDNResult(ppszLDAPValues[iValue], &ppszValues[dwNumValues]);
                    BAIL_ON_LW_ERROR(dwError);
                }
                else
                {
                    dwError = LwAllocateString(ppszLDAPValues[iValue], &ppszValues[dwNumValues]);
                    BAIL_ON_LW_ERROR(dwError);
                }
                if (ppszValues[dwNumValues])
                {
                    dwNumValues++;
                }
            }
        }
    }

    *pppszValues = ppszValues;
    *pdwNumValues = dwNumValues;

cleanup:
    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
    }

    return dwError;

error:
    LwFreeNullTerminatedStringArray(ppszValues);
    *pppszValues = NULL;
    *pdwNumValues = 0;

    goto cleanup;
}

DWORD
LwLdapPutString(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    PSTR   pszValue
    )
{
    DWORD dwError = 0;
    PLW_LDAP_DIRECTORY_CONTEXT pDirectory = NULL;
    PSTR modvals[2];
    LDAPMod  mod;
    LDAPMod* mods[2];

    pDirectory = (PLW_LDAP_DIRECTORY_CONTEXT)hDirectory;

    modvals[0] = pszValue;
    modvals[1] = NULL;
    
    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = (PSTR)pszFieldName;
    mod.mod_values = modvals;
    
    mods[0] = &mod;
    mods[1] = NULL;
    
    dwError = ldap_modify_s(pDirectory->ld, pszDN, mods);
    if (dwError)
    {
        LW_RTL_LOG_ERROR("Failed to update LDAP object attribute string with error: %d, errno: %d", dwError, errno);
    }
    BAIL_ON_LW_ERROR(dwError);
    
error:

    return dwError;
}

DWORD
LwLdapPutUInt32(
    HANDLE hDirectory,
    PCSTR  pszDN,
    PCSTR  pszFieldName,
    DWORD  dwValue
    )
{
    DWORD dwError = 0;
    char szValue[256];

    memset(szValue, 0, sizeof(szValue));
    sprintf(szValue, "%u", dwValue);
    
    dwError = LwLdapPutString(hDirectory, pszDN, pszFieldName, szValue);
    if (dwError)
    {
        LW_RTL_LOG_ERROR("Failed to update LDAP object attribute integer with error: %d, errno: %d", dwError, errno);
    }
    BAIL_ON_LW_ERROR(dwError);

error:

    return dwError;
}

/* Escapes a string according to the directions given in RFC 2254. */
DWORD
LwLdapEscapeString(
    PSTR *ppszResult,
    PCSTR pszInput
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    size_t iOutputPos = 0;
    size_t iInputPos = 0;
    PSTR pszResult = NULL;

    //Empty strings are allowed, but not null strings
    LW_BAIL_ON_INVALID_POINTER(pszInput);

    // Calculate the length of the escaped output string
    for(; pszInput[iInputPos]; iInputPos++)
    {
        switch(pszInput[iInputPos])
        {
            case '*':
            case '(':
            case ')':
            case '\\':
                iOutputPos += 3;
                break;
            default:
                iOutputPos ++;
                break;
        }
    }

    dwError = LwAllocateMemory(iOutputPos + 1, OUT_PPVOID(&pszResult));
    iOutputPos = 0;
    for(iInputPos = 0; pszInput[iInputPos]; iInputPos++)
    {
        switch(pszInput[iInputPos])
        {
            case '*':
                memcpy(pszResult + iOutputPos, "\\2a", 3);
                iOutputPos += 3;
                break;
            case '(':
                memcpy(pszResult + iOutputPos, "\\28", 3);
                iOutputPos += 3;
                break;
            case ')':
                memcpy(pszResult + iOutputPos, "\\29", 3);
                iOutputPos += 3;
                break;
            case '\\':
                memcpy(pszResult + iOutputPos, "\\5c", 3);
                iOutputPos += 3;
                break;
            default:
                pszResult[iOutputPos++] = pszInput[iInputPos];
                break;
        }
    }
    pszResult[iOutputPos++] = '\0';

    *ppszResult = pszResult;
    pszResult = NULL;

error:
    LW_SAFE_FREE_STRING(pszResult);
    return dwError;
}

#define DC_PREFIX "dc="

DWORD
LwLdapConvertDomainToDN(
    PCSTR pszDomainName,
    PSTR* ppszDomainDN
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszDomainDN = NULL;
    PCSTR pszIter = NULL;
    PSTR  pszWriteMark = NULL;
    DWORD dwRequiredDomainDNLen = 0;
    DWORD nDomainParts = 0;
    size_t stLength = 0;

    LW_BAIL_ON_INVALID_STRING(pszDomainName);

    // Figure out the length required to write the Domain DN
    pszIter = pszDomainName;
    while ((stLength = strcspn(pszIter, ".")) != 0) {
        dwRequiredDomainDNLen += sizeof(DC_PREFIX) - 1;
        dwRequiredDomainDNLen += stLength;
        nDomainParts++;

        pszIter += stLength;

        stLength = strspn(pszIter, ".");
        pszIter += stLength;
    }

    dwError = LwAllocateMemory(
                    sizeof(CHAR) * (dwRequiredDomainDNLen +
                                    nDomainParts),
                    OUT_PPVOID(&pszDomainDN));
    BAIL_ON_LW_ERROR(dwError);

    // Write out the Domain DN
    pszWriteMark = pszDomainDN;
    pszIter = pszDomainName;
    while ((stLength = strcspn(pszIter, ".")) != 0) {
        if (*pszDomainDN){
            *pszWriteMark++ = ',';
        }

        memcpy(pszWriteMark, DC_PREFIX, sizeof(DC_PREFIX) - 1);
        pszWriteMark += sizeof(DC_PREFIX) - 1;

        memcpy(pszWriteMark, pszIter, stLength);
        pszWriteMark += stLength;

        pszIter += stLength;

        stLength = strspn(pszIter, ".");
        pszIter += stLength;
    }

    *ppszDomainDN = pszDomainDN;

cleanup:

    return dwError;

error:

    *ppszDomainDN = NULL;

    LW_SAFE_FREE_STRING(pszDomainDN);

    goto cleanup;
}

DWORD
LwLdapConvertDNToDomain(
    PCSTR pszDN,
    PSTR* ppszDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pszDomainName = NULL;
    PSTR pszCurrent = NULL;
    PWSTR pwszDNCopy = NULL;
    // Do not free
    PWSTR pwszDcLocation = NULL;
    PSTR pszDcLocation = NULL;
    PCSTR pszDelim = ",";
    PSTR pszDomainPart = NULL;
    PSTR pszStrTokSav = NULL;
    const wchar16_t pwszDcPrefix[] = { 'd', 'c', '=', 0 };

    LW_BAIL_ON_INVALID_STRING(pszDN);

    dwError = LwMbsToWc16s(pszDN, &pwszDNCopy);
    BAIL_ON_LW_ERROR(dwError);

    LwWc16sToLower(pwszDNCopy);

    pwszDcLocation = pwszDNCopy;

    while (wc16sncmp(pwszDcLocation, pwszDcPrefix,
                sizeof(pwszDcPrefix)/2 - 1))
    {
        if (pwszDcLocation[0] == '\\')
        {
            // Skip one extra character and make sure the next one is not null
            pwszDcLocation++;
        }
        if (!pwszDcLocation[0])
        {
            dwError = LW_ERROR_INVALID_LDAP_DN;
            BAIL_ON_LW_ERROR(dwError);
        }
        pwszDcLocation++;
    }

    dwError = LwWc16sToMbs(pwszDcLocation, &pszDcLocation);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateMemory(strlen(pszDcLocation)*sizeof(CHAR),
                                OUT_PPVOID(&pszDomainName));
    BAIL_ON_LW_ERROR(dwError);

    pszCurrent = pszDomainName;

    pszDomainPart = strtok_r (pszDcLocation, pszDelim, &pszStrTokSav);
    while (pszDomainPart != NULL){
        DWORD dwLen = 0;

        if (strncmp(pszDomainPart, DC_PREFIX, sizeof(DC_PREFIX)-1)) {
            dwError = LW_ERROR_INVALID_LDAP_DN;
            BAIL_ON_LW_ERROR(dwError);
        }

        pszDomainPart += sizeof(DC_PREFIX) -1;

        dwLen = strlen(pszDomainPart);

        if (*pszDomainName) {
            *pszCurrent++ = '.';
        }

        memcpy(pszCurrent, pszDomainPart, dwLen);
        pszCurrent += dwLen;

        pszDomainPart = strtok_r (NULL, pszDelim, &pszStrTokSav);
    }

    *ppszDomainName = pszDomainName;

cleanup:

    LW_SAFE_FREE_MEMORY(pwszDNCopy);
    LW_SAFE_FREE_STRING(pszDcLocation);

    return dwError;

error:

    *ppszDomainName = NULL;

    LW_SAFE_FREE_STRING(pszDomainName);

    goto cleanup;
}

VOID
LwLdapFreeCookie(
    PVOID pCookie
    )
{
    if (pCookie != NULL)
    {
        ber_bvfree((struct berval*)pCookie);
    }
}

VOID
LwFreeCookieContents(
    IN OUT PLW_SEARCH_COOKIE pCookie
    )
{
    if (pCookie->pfnFree)
    {
        pCookie->pfnFree(pCookie->pvData);
        pCookie->pfnFree = NULL;
    }
    pCookie->pvData = NULL;
    pCookie->bSearchFinished = FALSE;
}

VOID
LwInitCookie(
    OUT PLW_SEARCH_COOKIE pCookie
    )
{
    memset(pCookie, 0, sizeof(*pCookie));
}

DWORD
LwLdapDirectoryExtendedDNSearch(
    IN HANDLE hDirectory,
    IN PCSTR pszObjectDN,
    IN PCSTR pszQuery,
    IN PSTR* ppszAttributeList,
    IN int scope,
    OUT LDAPMessage** ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR ExtDNCriticality = 'T';
    LDAPControl *pExtDNControl = NULL;
    LDAPControl *ppInputControls[2] = { NULL, NULL };
    LDAPMessage* pMessage = NULL;
    struct berval value = {0};

    // Setup the extended DN control, in order to be windows 2000 compatible,
    // Do not specify control value, hence, the return result will always be in hexadecimal string format.
    value.bv_len = 0;
    value.bv_val = NULL;
    dwError = ldap_control_create(LDAP_CONTROL_X_EXTENDED_DN,
                                  ExtDNCriticality,
                                  &value,
                                  0,
                                  &pExtDNControl);
    BAIL_ON_LDAP_ERROR(dwError);

    ppInputControls[0] = pExtDNControl;

    dwError = LwLdapDirectorySearchEx(
               hDirectory,
               pszObjectDN,
               scope,
               pszQuery,
               ppszAttributeList,
               ppInputControls,
               0,
               &pMessage);
    BAIL_ON_LW_ERROR(dwError);

    LW_ASSERT(pMessage != NULL);
    *ppMessage = pMessage;

cleanup:
    ppInputControls[0] = NULL;

    if (pExtDNControl)
    {
        ldap_control_free(pExtDNControl);
    }

    return (dwError);

error:
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    *ppMessage = NULL;

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
