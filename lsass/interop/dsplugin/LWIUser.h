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

#ifndef __LWIUSER_H__
#define __LWIUSER_H__

#include "LWIPlugIn.h"

#ifdef __cplusplus
extern "C" {
#endif

long
CreateLWIUser(
    PCSTR pszName,
    PCSTR pszDisplayName,
    PCSTR pszNameAsQueried,
    PCSTR pszPassword,
    PCSTR pszClass,
    PCSTR pszGecos,
    PCSTR pszNFSHomeDirectory,
    PCSTR pszHomeDirectory,
    PCSTR pszOrigNFSHomeDirectory,
    PCSTR pszOrigHomeDirectory,
    PCSTR pszShell,
    uid_t uid,
    gid_t gid,
    PMCXVALUE pMCXValues,
    PAD_USER_ATTRIBUTES padUserADInfo,
    PLWIUSER* ppLWIUser
    );

void
FreeLWIUser(
    PLWIUSER pLWIUser
    );

#ifdef __cplusplus
}
#endif

#endif /* __LWIUSER_H__ */

