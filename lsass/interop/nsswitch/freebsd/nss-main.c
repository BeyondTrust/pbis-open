/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss-main.c
 *
 * Abstract:
 *
 *        Name Server Switch (BeyondTrust LSASS)
 *
 *        Main Entry Points
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "nss-user.h"
#include "nss-group.h"
#include "externs.h"

#if defined(__LWI_FREEBSD__)

static
int
LsaNSSFindUserById(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSFindUserByName(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSBeginEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSEndEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSFindGroupById(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSFindGroupByName(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSBeginEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSEndEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

static
int
LsaNSSGetGroupMembership(
    PVOID pResult,
    PVOID pData,
    va_list ap
    );

ns_mtab *
nss_module_register(
    const char*   pszModName,
    unsigned int* pLen,
    nss_module_unregister_fn* pFnUnregister
    )
{
    static ns_mtab fnTable[] =
          {
	    { "passwd", "getpwuid_r",         &LsaNSSFindUserById,       0},
	    { "passwd", "getpwnam_r",         &LsaNSSFindUserByName,     0},
	    { "passwd", "setpwent",           &LsaNSSBeginEnumUsers,     0},
	    { "passwd", "getpwent_r",         &LsaNSSEnumUsers,          0},
	    { "passwd", "endpwent",           &LsaNSSEndEnumUsers,       0},
	    { "group",  "getgrnam_r",         &LsaNSSFindGroupByName,    0},
	    { "group",  "getgrgid_r",         &LsaNSSFindGroupById,      0},
	    { "group",  "setgrent",           &LsaNSSBeginEnumGroups,    0},
	    { "group",  "getgrent_r",         &LsaNSSEnumGroups,         0},
	    { "group",  "endgrent",           &LsaNSSEndEnumGroups,      0},
            { "group",  "getgroupmembership", &LsaNSSGetGroupMembership, 0}
	  };

    *pLen = sizeof(fnTable)/sizeof(fnTable[0]);
    *pFnUnregister = NULL;

    return fnTable;
}

static
int
LsaNSSFindUserById(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    uid_t uid = 0;
    struct passwd* pResultUser = NULL;
    PSTR pszBuf = NULL;
    size_t stBufLen = 0;
    PINT   pErrorNumber = 0;

    uid = va_arg(ap, uid_t);
    pResultUser = (struct passwd*)va_arg(ap, struct passwd *);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int *);

    ret = _nss_lsass_getpwuid_r(
		     uid,
		     pResultUser,
		     pszBuf,
		     stBufLen,
		     pErrorNumber);

    if(pResult)
    {
        *((struct passwd**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultUser;
    }

    return ret;
}

static
int
LsaNSSFindUserByName(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    PCSTR  pszLoginId = NULL;
    struct passwd* pResultUser = NULL;
    PSTR   pszBuf = NULL;
    size_t stBufLen = 0;
    PINT   pErrorNumber = NULL;

    pszLoginId = va_arg(ap, PCSTR);
    pResultUser = (struct passwd*)va_arg(ap, struct passwd *);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int *);

    ret = _nss_lsass_getpwnam_r(
		     pszLoginId,
		     pResultUser,
		     pszBuf,
		     stBufLen,
		     pErrorNumber);

    if(pResult)
    {
        *((struct passwd**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultUser;
    }


    return ret;
}

static
int
LsaNSSBeginEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = _nss_lsass_setpwent();

    return ret;
}

static
int
LsaNSSEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    struct passwd* pResultUser = NULL;
    PSTR pszBuf = NULL;
    size_t stBufLen = 0;
    PINT pErrorNumber = NULL;

    pResultUser = (struct passwd*)va_arg(ap, struct passwd*);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int*);

    ret = _nss_lsass_getpwent_r(
				pResultUser,
				pszBuf,
				stBufLen,
				pErrorNumber);

    if(pResult)
    {
        *((struct passwd**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultUser;
    }

    return ret;
}

static
int
LsaNSSEndEnumUsers(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = _nss_lsass_endpwent();

    return ret;
}

static
int
LsaNSSFindGroupById(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    gid_t gid = 0;
    struct group* pResultGroup = NULL;
    PSTR pszBuf = NULL;
    size_t stBufLen = 0;
    PINT   pErrorNumber = 0;

    gid = va_arg(ap, gid_t);
    pResultGroup = (struct group*)va_arg(ap, struct group *);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int *);

    ret = _nss_lsass_getgrgid_r(
		     gid,
		     pResultGroup,
		     pszBuf,
		     stBufLen,
		     pErrorNumber);

    if(pResult)
    {
	*((struct group**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultGroup;
    }

    return ret;
}

static
int
LsaNSSFindGroupByName(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    PCSTR  pszGroupName = NULL;
    struct group* pResultGroup = NULL;
    PSTR   pszBuf = NULL;
    size_t stBufLen = 0;
    PINT   pErrorNumber = NULL;

    pszGroupName = va_arg(ap, PCSTR);
    pResultGroup = (struct group*)va_arg(ap, struct group *);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int *);

    ret = _nss_lsass_getgrnam_r(
		     pszGroupName,
		     pResultGroup,
		     pszBuf,
		     stBufLen,
		     pErrorNumber);

    if(pResult)
    {
	*((struct group**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultGroup;
    }


    return ret;
}

static
int
LsaNSSBeginEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = _nss_lsass_setgrent();

    return ret;
}

static
int
LsaNSSEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0;
    struct group* pResultGroup = NULL;
    PSTR pszBuf = NULL;
    size_t stBufLen = 0;
    PINT pErrorNumber = NULL;

    pResultGroup = (struct group*)va_arg(ap, struct group*);
    pszBuf = (PSTR)va_arg(ap, char*);
    stBufLen = (size_t)va_arg(ap, size_t);
    pErrorNumber = (PINT)va_arg(ap, int*);

    ret = _nss_lsass_getgrent_r(
				pResultGroup,
				pszBuf,
				stBufLen,
				pErrorNumber);

    if(pResult)
    {
	*((struct group**)pResult) = ret != NSS_STATUS_SUCCESS ? NULL : pResultGroup;
    }

    return ret;
}

static
int
LsaNSSEndEnumGroups(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = _nss_lsass_endgrent();

    return ret;
}

static
int
LsaNSSGetGroupMembership(
    PVOID pResult,
    PVOID pData,
    va_list ap
    )
{
    int ret = 0, err = 0;

    PCSTR pszUserName = va_arg(ap, PCSTR);
    __attribute__((unused)) gid_t groupGid = va_arg(ap, gid_t);
    gid_t* pResultGids = va_arg(ap, gid_t*);
    size_t maxResultGids = (size_t) va_arg(ap, int);
    PINT pNumResultGids = va_arg(ap, PINT);

    size_t myResultsSize = *pNumResultGids;

    ret = LsaNssCommonGroupGetGroupsByUserName(
        &lsaConnection,
        pszUserName,
        *pNumResultGids,
        maxResultGids,
        &myResultsSize,
        pResultGids,
        &err);

    if (myResultsSize > maxResultGids)
        myResultsSize = maxResultGids;

    *pNumResultGids = (int) myResultsSize;

    errno = err;

    return ret;
}

#endif /* __LWI__FREEBSD__ */
