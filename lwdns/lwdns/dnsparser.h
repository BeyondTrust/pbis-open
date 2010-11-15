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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dnsparser.h
 *
 * Abstract:
 *
 *        Likewise Dynamic DNS Updates (LWDNS) 
 * 
 *        DNS API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *          Danilo Alameida (dalmeida@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __DNSPARSER_H__
#define __DNSPARSER_H__

#if defined HAVE_DECL_RES_INIT && !HAVE_DECL_RES_INIT

int
res_init(void);

#endif

#if defined HAVE_DECL_RES_QUERY && !HAVE_DECL_RES_QUERY

#ifdef __LWI_HP_UX__

ssize_t
res_query(
    char*          dname,
    int            class,
    int            type,
    unsigned char* answer,
    int            anslen
    );

#else

int
res_query(
    const char*    dname,
    int            class,
    int            type,
    unsigned char* answer,
    int            anslen
    );

#endif /* __LWI_HP_UX__ */

#endif /* defined HAVE_DECL_RES_QUERY && !HAVE_DECL_RES_QUERY */

DWORD
DNSGetNameServers(
    IN PCSTR pszDomain,
    OUT PSTR* ppszZone,
    OUT PLW_NS_INFO* ppNSInfoList,
    OUT PDWORD pdwNumServers
    );

VOID
DNSFreeNameServerInfoArray(
    PLW_NS_INFO pNSInfoArray,
    DWORD       dwNumInfos
    );

VOID
DNSFreeNameServerInfo(
    PLW_NS_INFO pNSInfo
    );

VOID
DNSFreeNameServerInfoContents(
    PLW_NS_INFO pNSInfo
    );

#endif /* __DNSPARSER_H__ */
