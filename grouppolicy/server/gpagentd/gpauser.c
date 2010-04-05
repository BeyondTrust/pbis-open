#include "includes.h"

void
GPAFreeUserContextList(
    PGPUSERCONTEXT pUserCtxList
    )
{
    PGPUSERCONTEXT pUserCtx = NULL;
    while (pUserCtxList) {
        pUserCtx = pUserCtxList;
        pUserCtxList = pUserCtxList->pNext;
        if (pUserCtx->pUserInfo) {
           GPAFreeUser(pUserCtx->pUserInfo);
        }
        GPA_SAFE_FREE_GPO_LIST(pUserCtx->pGPOList);
        LwFreeMemory(pUserCtx);
    }
}

/**
 * This routine does not duplicate the GPO List right now
 */
CENTERROR
GPACloneUserContext(
        PGPUSERCONTEXT pContext,
        PGPUSERCONTEXT* ppCopyContext
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pNewContext = NULL;

    ceError = LwAllocateMemory(sizeof(GPUSERCONTEXT), (PVOID*)&pNewContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pthread_mutex_init(&pNewContext->lock, NULL);
    pthread_cond_init(&pNewContext->cond, NULL);

    pNewContext->policyStatus = pContext->policyStatus;

    ceError = LwAllocateMemory(sizeof(GPUSER), (PVOID*)&pNewContext->pUserInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(pContext->pUserInfo->pszName, &pNewContext->pUserInfo->pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(pContext->pUserInfo->pszSID, &pNewContext->pUserInfo->pszSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = LwAllocateString(pContext->pUserInfo->pszHomeDir, &pNewContext->pUserInfo->pszHomeDir);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pNewContext->pUserInfo->uid = pContext->pUserInfo->uid;

    *ppCopyContext = pNewContext;
    pNewContext = NULL;

error:

    GPA_SAFE_FREE_USER_CONTEXT_LIST(pNewContext);

    return ceError;
}

static
CENTERROR
GPACreateGPUserContext(
    PGPUSER pUserInfo,
    PGPUSERCONTEXT* ppUserContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pUserContext = NULL;

    ceError = LwAllocateMemory(sizeof(GPUSERCONTEXT), (PVOID*)&pUserContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pthread_mutex_init(&pUserContext->lock, NULL);
    pthread_cond_init(&pUserContext->cond, NULL);
    pUserContext->policyStatus = GP_USER_POLICY_NEW;
    pUserContext->pUserInfo = pUserInfo;

    *ppUserContext = pUserContext;

cleanup:
    return ceError;

error:
    *ppUserContext = NULL;
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);
    goto cleanup;
}

CENTERROR
GPAGetADUserInfoForLoginId(
    PCSTR pszLoginId,
    PGPUSERCONTEXT* ppUserContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSER pUserInfo = NULL;
    PGPUSERCONTEXT pUserContext = NULL;

    if (IsNullOrEmptyString(pszLoginId) || !strcmp(pszLoginId, "root")) {
        ceError = CENTERROR_GP_NOT_AD_USER;
        goto error;
    }

    ceError = GPAFindUserByName(pszLoginId, &pUserInfo);
    if (CENTERROR_EQUAL(ceError, CENTERROR_GP_NOT_AD_USER)) {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACreateGPUserContext(pUserInfo, &pUserContext);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pUserInfo = NULL;

    *ppUserContext = pUserContext;

cleanup:
    if (pUserInfo)
    {
        GPAFreeUser(pUserInfo);
    }
    return ceError;

error:
    *ppUserContext = NULL;
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);
    goto cleanup;
}

CENTERROR
GPAGetADUserInfoForUID(
    PCSTR    pszLoginId,
    uid_t    uid,
    PGPUSERCONTEXT* ppUserContext
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSER pUserInfo = NULL;
    PGPUSERCONTEXT pUserContext = NULL;

    // The login name is optional, so only check it if present.
    if (pszLoginId && !strcmp(pszLoginId, "root")) {
        ceError = CENTERROR_GP_NOT_AD_USER;
        goto error;
    }

    // Pass in NULL login name to get actual AD name
    ceError = GPAFindUserById(NULL, uid, &pUserInfo);
    if (CENTERROR_EQUAL(ceError, CENTERROR_GP_NOT_AD_USER)) {
        goto error;
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPACreateGPUserContext(pUserInfo, &pUserContext);
    BAIL_ON_CENTERIS_ERROR(ceError);
    pUserInfo = NULL;

    *ppUserContext = pUserContext;

cleanup:
    if (pUserInfo)
    {
        GPAFreeUser(pUserInfo);
    }
    return ceError;

error:
    *ppUserContext = NULL;
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);
    goto cleanup;
}

static
void
InsertUserSortedAscending(
    PGPUSERCONTEXT* ppUserContextList,
    PGPUSERCONTEXT pUserContext
    )
{
    PGPUSERCONTEXT pPrev = NULL;
    PGPUSERCONTEXT pContext = (*ppUserContextList ? *ppUserContextList : NULL);

    while (pContext &&
           (pContext->pUserInfo->uid <= pUserContext->pUserInfo->uid)) {
          pPrev = pContext;
          pContext = pContext->pNext;
    }

    if (!pPrev) {
        pUserContext->pNext = pContext;
        *ppUserContextList = pUserContext;
    } else {
        pUserContext->pNext = pPrev->pNext;
        pPrev->pNext = pUserContext;
    }

    return;
}

#if defined(HAVE_UTMPX_H)
CENTERROR
GPAGetCurrentADUsersFromUtmpx(
    PGPUSERCONTEXT* ppUserContextList
    )
{
    static pthread_mutex_t utmpx_lock = PTHREAD_MUTEX_INITIALIZER;
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPUSERCONTEXT pUserContextList = NULL;
    PGPUSERCONTEXT pUserContext = NULL;
    PSTR           pszDomainSID = NULL;
    CHAR           szBuf[128];
    struct utmpx *pEntry = NULL;
    BOOLEAN        bFound = FALSE;
    uid_t          uid;

    ceError = GPAGetDomainSID(&pszDomainSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    pthread_mutex_lock(&utmpx_lock);

    setutxent();

    while ((pEntry = getutxent())) {

        if (pEntry->ut_type != USER_PROCESS &&
            pEntry->ut_type != DEAD_PROCESS &&
            pEntry->ut_type != EMPTY)
        {
            GPA_LOG_VERBOSE("Entry (%s:%d) is not a user, skipping entry.", pEntry->ut_user ? pEntry->ut_user : "<no name>", pEntry->ut_type);
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        if (!pEntry->ut_user)
        {
            GPA_LOG_VERBOSE("Entry does not have a name, skipping entry.");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        if (!pEntry->ut_user[0])
        {
            GPA_LOG_VERBOSE("Entry does not have a valid name, skipping entry.");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        GPA_LOG_VERBOSE("GPAGetCurrentADUsersFromUtmpx(), got pEntry (%s)", pEntry->ut_user);

        if (!strcmp(pEntry->ut_user, "root"))
        {
            GPA_LOG_VERBOSE("User (root) is logged on, skipping entry.");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        ceError = GPAGetUserUID(pEntry->ut_user, &uid);
        if (!CENTERROR_IS_OK(ceError)) {
            GPA_LOG_VERBOSE("GPAgent user detection error - could not get UID for: name(%s) error: [%d] (%s)", pEntry->ut_user, ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        bFound = FALSE;
        for( pUserContext = pUserContextList; pUserContext; pUserContext = pUserContext->pNext) {
            if (pUserContext->pUserInfo->uid == uid) {
                GPA_LOG_VERBOSE("User (%s:%d) is already logged on, skipping duplicate", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);
                bFound = TRUE;
                break;
            }
        }

        //
        // Handle users having multiple login sessions.
        //
        if (bFound) {
            pUserContext = NULL;
            continue;
        }

        ceError = GPAGetADUserInfoForUID(pEntry->ut_user, uid, &pUserContext);
        if (!CENTERROR_IS_OK(ceError) &&
            !CENTERROR_EQUAL(ceError, CENTERROR_GP_NOT_AD_USER)) {
            GPA_LOG_VERBOSE("User (%d) is not an AD user, skipping entry.", uid);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (!pUserContext)
        {
            GPA_LOG_VERBOSE("User (%d) did not have a context, skipping entry.", uid);
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        GPA_LOG_VERBOSE("User (%s:%d) is logged on", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);

        // If it is a user in the local password database...
        sprintf(szBuf, "S-1-22-1-%ld", (long)pUserContext->pUserInfo->uid);

        // Skip if it is a local user in the password database
        // Skip if it is a unmapped local user in the samba database
        if (!strcmp(pUserContext->pUserInfo->pszSID, pszDomainSID) ||
            !strcmp(pUserContext->pUserInfo->pszSID, szBuf)) {
            GPA_LOG_VERBOSE("User (%d) matches local user database, skipping entry.", uid);
            GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);
            continue;
        }

        GPA_LOG_VERBOSE("User (%s:%d) added to logged on list.", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);
        InsertUserSortedAscending(&pUserContextList, pUserContext);
        pUserContext = NULL;
    }

    *ppUserContextList = pUserContextList;
    pUserContextList = NULL;

  error:

    endutxent();

    pthread_mutex_unlock(&utmpx_lock);

    LW_SAFE_FREE_STRING(pszDomainSID);

    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContextList);
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);

    return ceError;
}
#endif

#if !defined(__LWI_DARWIN_X64__)
CENTERROR
GPAGetCurrentADUsersFromUtmp(
    PGPUSERCONTEXT* ppUserContextList
    )
{
    /* utmp file path macro differs among operating systems */
#if defined(__LWI_FREEBSD__) || defined(__LWI_DARWIN__)
    const char *utmp_file = _PATH_UTMP;
#else
    const char *utmp_file = UTMP_FILE;
#endif

    static pthread_mutex_t utmp_lock = PTHREAD_MUTEX_INITIALIZER;
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* pfUtmp = NULL;
    struct utmp utmpEntry;
    CHAR szBuf[128];
    PSTR pszDomainSID = NULL;
    PGPUSERCONTEXT pUserContextList = NULL;
    PGPUSERCONTEXT pUserContext = NULL;
    uid_t uid = 0;
    BOOLEAN bFound = FALSE;

    pthread_mutex_lock(&utmp_lock);

    ceError = GPAGetDomainSID(&pszDomainSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if ((pfUtmp = fopen(utmp_file, "rb")) == NULL) {
       ceError = LwMapErrnoToLwError(errno);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    while (1) {

        memset(&utmpEntry, 0, sizeof(struct utmp));

        if (fread(&utmpEntry, sizeof(struct utmp), 1, pfUtmp) != 1)
        {
            if (feof(pfUtmp)) {
                break;
            }
            else {
                ceError = LwMapErrnoToLwError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        *(utmpEntry.ut_line + sizeof(utmpEntry.ut_line) - 1) = 0;
        LwStripWhitespace(utmpEntry.ut_line,1,1);
        if (IsNullOrEmptyString(utmpEntry.ut_line))
             continue;

        sprintf(szBuf, "/dev/%s", utmpEntry.ut_line);

        uid = 0;
        ceError = GPAGetOwnerUID(szBuf, &uid);
        if (!CENTERROR_IS_OK(ceError)) {
            GPA_LOG_VERBOSE("GPAgent user detection error - could not get owner UID for: file(%s) error: [%d] (%s)", szBuf, ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        // Skip root
        if (!uid)
        {
            GPA_LOG_VERBOSE("User (root:0) is logged on, skipping entry.");
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        bFound = FALSE;
        for( pUserContext = pUserContextList; pUserContext; pUserContext = pUserContext->pNext) {
            if (pUserContext->pUserInfo->uid == uid) {
                GPA_LOG_VERBOSE("User (%s:%d) is already logged on, skipping duplicate", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);
                bFound = TRUE;
                break;
            }
        }

        //
        // Handle users having multiple login sessions.
        //
        if (bFound) {
            pUserContext = NULL;
            continue;
        }

        // We pass in a null string for the login id
        // The uid is looked up to find out the login id
        ceError = GPAGetADUserInfoForUID("", uid, &pUserContext);
        if (!CENTERROR_IS_OK(ceError) &&
            !CENTERROR_EQUAL(ceError, CENTERROR_GP_NOT_AD_USER)) {
            GPA_LOG_VERBOSE("User (%d) is not an AD user, skipping entry.", uid);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if (!pUserContext)
        {
            GPA_LOG_VERBOSE("User (%d) did not have a context, skipping entry.", uid);
            ceError = CENTERROR_SUCCESS;
            continue;
        }

        GPA_LOG_VERBOSE("User (%s:%d) is logged on", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);

        // If it is a user in the local password database...
        sprintf(szBuf, "S-1-22-1-%ld", (long)pUserContext->pUserInfo->uid);

        // Skip if it is a local user in the password database
        // Skip if it is a unmapped local user in the samba database
        if (!strcmp(pUserContext->pUserInfo->pszSID, pszDomainSID) ||
            !strcmp(pUserContext->pUserInfo->pszSID, szBuf)) {
            GPA_LOG_VERBOSE("User (%d) matches local user database, skipping entry.", uid);
            GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);
            continue;
        }

        GPA_LOG_VERBOSE("User (%s:%d) added to logged on list.", pUserContext->pUserInfo->pszName, pUserContext->pUserInfo->uid);
        InsertUserSortedAscending(&pUserContextList, pUserContext);
        pUserContext = NULL;
    }

    *ppUserContextList = pUserContextList;

cleanup:

    if (pfUtmp) {
        fclose(pfUtmp);
    }

    pthread_mutex_unlock(&utmp_lock);

    LW_SAFE_FREE_STRING(pszDomainSID);

    return ceError;

error:

    *ppUserContextList = NULL;

    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContextList);
    GPA_SAFE_FREE_USER_CONTEXT_LIST(pUserContext);

    goto cleanup;
}
#endif // !LWI_DARWIN_X64

CENTERROR
GPAGetCurrentADUsers(
    PSTR pszDistroName,
    PSTR pszDistroVersion,
    PGPUSERCONTEXT* ppUserContextList
    )
{
#if !defined(__LWI_DARWIN_X64__)
    if (pszDistroName && pszDistroVersion)
    {
        if (!strcmp(pszDistroName, "Darwin"))
        {
            if (!strncmp(pszDistroVersion, "8.", 2))
            {
                /* Looking for Darwin Kernel 8.X */
                return GPAGetCurrentADUsersFromUtmp(ppUserContextList);
            }
        }

        if (!strcmp(pszDistroName, "Mac OSX"))
        {
            if (strstr(pszDistroVersion, "10.4"))
            {
                /* Looking for Tiger OS X 10.4.X */
                return GPAGetCurrentADUsersFromUtmp(ppUserContextList);
            }
        }
    }
#endif

    /* all others (Linux, Mac OSX 10.5, etc) */
#if defined(HAVE_UTMPX_H)
    return GPAGetCurrentADUsersFromUtmpx(ppUserContextList);
#else
#if !defined(__LWI_DARWIN_X64__)
    return GPAGetCurrentADUsersFromUtmp(ppUserContextList);
#else
    return CENTERROR_NOT_IMPLEMENTED;
#endif
#endif
}

