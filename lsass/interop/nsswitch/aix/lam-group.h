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
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 */

#ifndef __LSA_NSS_AIX_GROUP_H__
#define __LSA_NSS_AIX_GROUP_H__

void
LsaNssFreeLastGroup(
        VOID
        );

DWORD
LsaNssAllocateGroupFromInfo1(
        PLSA_GROUP_INFO_1 pInfo,
        struct group** ppResult
        );

DWORD
LsaNssAllocateGroupFromInfo0(
        PLSA_GROUP_INFO_0 pInfo,
        struct group** ppResult
        );

struct group *
LsaNssGetGrGid(
        gid_t gid
        );

struct group *
LsaNssGetGrNam(
        PCSTR pszName
        );

struct group *
LsaNssGetGrAcct(
        PVOID pId,
        int iType
        );

PSTR
LsaNssGetGrSet(
        PSTR pszName
        );

DWORD
LsaNssListGroups(
        HANDLE hLsaConnection,
        attrval_t* pResult
        );

DWORD
LsaNssGetGidList(
        HANDLE hLsaConnection,
        PLSA_USER_INFO_2 pInfo,
        PSTR* ppszList
        );

DWORD
LsaNssGetGroupList(
        HANDLE hLsaConnection,
        PLSA_USER_INFO_2 pInfo,
        PSTR* ppszList
        );

DWORD
LsaNssFillMemberList(
        PLSA_GROUP_INFO_1 pInfo,
        PSTR* ppResult
        );

VOID
LsaNssGetGroupAttr(
        HANDLE hLsaConnection,
        PLSA_GROUP_INFO_1 pInfo,
        PSTR pszAttribute,
        attrval_t* pResult
        );

DWORD
LsaNssGetGroupAttrs(
        HANDLE hLsaConnection,
        PSTR pszKey,
        PSTR* ppszAttributes,
        attrval_t* pResults,
        int iAttrCount
        );

#endif
