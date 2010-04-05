/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to Manage AD Join/Leave/Query
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef enum
{
    LW_DOMAIN_TASK_TYPE_UNKNOWN = 0,
    LW_DOMAIN_TASK_TYPE_JOIN,
    LW_DOMAIN_TASK_TYPE_LEAVE,
    LW_DOMAIN_TASK_TYPE_QUERY
} LW_DOMAIN_TASK_TYPE;

typedef struct _LW_DOMAIN_ARGS
{
    LW_DOMAIN_TASK_TYPE taskType;

    union
    {
        struct
        {
            PSTR pszUsername;
            PSTR pszPassword;
            PSTR pszDomainName;
            PSTR pszOU;
        } joinArgs;

        struct
        {
            PSTR pszUsername;
            PSTR pszPassword;
        } leaveArgs;

    } args;

} LW_DOMAIN_INFO_REQUEST, *PLW_DOMAIN_INFO_REQUEST;

typedef struct _LW_DOMAIN_DISTRO_INFO
{
    LwDomainOSType     osType;
    LwDomainDistroType distroType;
    LwDomainArchType   archType;
    PSTR               pszVersion;

} LW_DOMAIN_DISTRO_INFO, *PLW_DOMAIN_DISTRO_INFO;

typedef struct _LW_DOMAIN_ARCH_LOOKUP
{
    LwDomainArchType archType;
    PSTR             pszName;

} LW_DOMAIN_ARCH_LOOKUP, *PLW_DOMAIN_ARCH_LOOKUP;

typedef struct _LW_DOMAIN_DISTRO_LOOKUP
{
    LwDomainDistroType distroType;
    PSTR               pszName;
} LW_DOMAIN_DISTRO_LOOKUP, *PLW_DOMAIN_DISTRO_LOOKUP;

typedef struct _LW_DOMAIN_OS_LOOKUP
{
    LwDomainOSType osType;
    PSTR           pszName;
} LW_DOMAIN_OS_LOOKUP, *PLW_DOMAIN_OS_LOOKUP;

#endif /* __STRUCTS_H__ */

