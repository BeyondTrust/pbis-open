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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     ad-types.h
 *
 * Abstract:
 *
 *     Likewise Security and Authentication Subsystem (LSASS)
 *
 *     Types used by AD provider APIs
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LSA_AD_TYPES_H__
#define __LSA_AD_TYPES_H__

typedef DWORD LSA_NET_JOIN_FLAGS, *PLSA_NET_JOIN_FLAGS;

#define LSA_NET_JOIN_DOMAIN_NOTIMESYNC         (0x00000001)
#define LSA_NET_JOIN_DOMAIN_MULTIPLE           (0x00000002)
// The following settings are not implemented
#define LSA_NET_JOIN_DOMAIN_ACCT_CREATE        (0x00000004)
#define LSA_NET_JOIN_DOMAIN_JOIN_IF_JOINED     (0x00000008)
#define LSA_NET_JOIN_DOMAIN_JOIN_UNSECURE      (0x00000010)
#define LSA_NET_JOIN_DOMAIN_MACHINE_PWD_PASSED (0x00000020)
#define LSA_NET_JOIN_DOMAIN_DEFER_SPN_SET      (0x00000040)
#define LSA_NET_JOIN_DOMAIN_JOIN_WITH_NEW_NAME (0x00000080)
#define LSA_NET_JOIN_DOMAIN_JOIN_READONLY      (0x00000100)

// The following setting is not implemented
#define LSA_NET_LEAVE_DOMAIN_ACCT_DELETE       (0x00000001)

#endif /* __LSA_AD_TYPES_H__ */
