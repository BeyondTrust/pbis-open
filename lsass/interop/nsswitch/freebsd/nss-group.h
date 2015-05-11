/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss-group.h
 *
 * Abstract:
 * 
 *        Name Server Switch (Likewise LSASS)
 * 
 *        Handle NSS Group Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __LSANSSGRP_H__
#define __LSANSSGRP_H__

NSS_STATUS
_nss_lsass_setgrent(
    void
    );

NSS_STATUS
_nss_lsass_getgrent_r(
    struct group*  pGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    );

NSS_STATUS
_nss_lsass_endgrent(
    void
    );

NSS_STATUS
_nss_lsass_getgrgid_r(
    gid_t          gid,
    struct group*  pGroup,
    char*          pszGroup,
    size_t         bufLen,
    int*           pErrorNumber
    );

NSS_STATUS
_nss_lsass_getgrnam_r(
    const char *   pszGroupName,
    struct group * pGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    );

NSS_STATUS
_nss_lsass_initgroups_dyn(
    const char *user, 
    gid_t group, 
    long int *resultsSize, 
    long int *resultsCapacity, 
    gid_t **results, 
    long int maxGroups,
    int *errnop);

#endif /* __LSANSSGRP_H__ */

