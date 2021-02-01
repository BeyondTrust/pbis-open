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
 *        nss-group.c
 *
 * Abstract:
 *
 *        Name Server Switch (BeyondTrust LSASS)
 *
 *        Handle NSS Group Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"
#include <assert.h>

static LSA_ENUMGROUPS_STATE gEnumGroupsState = {0};

NSS_STATUS
_nss_lsass_setgrent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupSetgrent(&lsaConnection,
                                       &gEnumGroupsState);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrent_r(
    struct group*  pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrent(
        &lsaConnection,
        &gEnumGroupsState,
        pResultGroup,
        pszBuf,
        bufLen,
        pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_endgrent(
    void
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupEndgrent(&lsaConnection, &gEnumGroupsState);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrgid_r(
    gid_t          gid,
    struct group*  pResultGroup,
    char*          pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrgid(&lsaConnection,
                                       gid,
                                       pResultGroup,
                                       pszBuf,
                                       bufLen,
                                       pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_getgrnam_r(
    const char *   pszGroupName,
    struct group * pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    NSS_STATUS status;

    NSS_LOCK();

    status = LsaNssCommonGroupGetgrnam(&lsaConnection,
                                       pszGroupName,
                                       pResultGroup,
                                       pszBuf,
                                       bufLen,
                                       pErrorNumber);

    NSS_UNLOCK();

    return status;
}

NSS_STATUS
_nss_lsass_initgroups_dyn(
    PCSTR     pszUserName,
    gid_t     groupGid,
    long int* pResultsSize,
    long int* pResultsCapacity,
    gid_t**   ppGidResults,
    long int  maxGroups,
    int*      pErrorNumber
    )
{
    int ret = NSS_STATUS_SUCCESS;
    size_t resultsCapacity = (size_t) *pResultsCapacity;
    size_t resultsExistingSize = (size_t) *pResultsSize;
    size_t resultsSize = 0;
    gid_t* pGidResults = *ppGidResults;
    gid_t* pGidResultsNew = NULL;

    NSS_LOCK();

    ret = LsaNssCommonGroupGetGroupsByUserName(
        &lsaConnection,
        pszUserName,
        resultsExistingSize,
        resultsCapacity,
        &resultsSize,
        pGidResults,
        pErrorNumber);

    if (ret != NSS_STATUS_SUCCESS)
        goto error;

    if (resultsSize > resultsCapacity)
    {
        /* More results were found than were stored, so
           reallocate array and try again. */
        pGidResultsNew = realloc(pGidResults, sizeof(*pGidResults) * resultsSize);
        if (!pGidResultsNew)
        {
            *pErrorNumber = ENOMEM;
            ret = NSS_STATUS_UNAVAIL;
            goto error;
        }
        else
        {
            pGidResults = pGidResultsNew;
            *ppGidResults = pGidResults;
        }

        /* The number of filled elements is whatever our old capacity was */
        resultsExistingSize = resultsCapacity;
        /* The new capacity is the number of available results */
        resultsCapacity = resultsSize;
        /* Try again */
        ret = LsaNssCommonGroupGetGroupsByUserName(
            &lsaConnection,
            pszUserName,
            resultsExistingSize,
            resultsCapacity,
            &resultsSize,
            pGidResults,
            pErrorNumber);
    }

    if (ret == NSS_STATUS_SUCCESS)
    {
        *pResultsSize = (long int) resultsSize;
        *pResultsCapacity = (long int) resultsCapacity;
    }

error:

    NSS_UNLOCK();

    return ret;
}

