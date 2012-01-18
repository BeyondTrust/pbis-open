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
 *        defines.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __DEFINES_H__
#define __DEFINES_H__

#define MAP_LWNET_ERROR(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

#define NTLM_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define NTLM_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           abort(); \
       } \
       bInLock = FALSE; \
    }

// Message signature and sizes

#define NTLM_NETWORK_SIGNATURE      "NTLMSSP"
#define NTLM_NETWORK_SIGNATURE_SIZE 8
#define NTLM_WIN_SPOOF_SIZE         8
#define NTLM_CHALLENGE_SIZE         8
#define NTLM_OS_VER_INFO_SIZE       8
#define NTLM_LOCAL_CONTEXT_SIZE     8
#define NTLM_BLOB_SIGNATURE         {1,1,0,0}
#define NTLM_BLOB_SIGNATURE_SIZE    4
#define NTLM_BLOB_TRAILER_SIZE      4

// Message type

#define NTLM_NEGOTIATE_MSG           1
#define NTLM_CHALLENGE_MSG           2
#define NTLM_RESPONSE_MSG            3

// Response type

#define NTLM_RESPONSE_TYPE_LM           0
#define NTLM_RESPONSE_TYPE_LMv2         1
#define NTLM_RESPONSE_TYPE_NTLM         2
#define NTLM_RESPONSE_TYPE_NTLMv2       3
#define NTLM_RESPONSE_TYPE_NTLM2        4
#define NTLM_RESPONSE_TYPE_ANON_LM      5
#define NTLM_RESPONSE_TYPE_ANON_NTLM    6

// Response sizes... NTLMv2 is not listed since it is a variable sized response

#define NTLM_RESPONSE_SIZE_LM           24
#define NTLM_RESPONSE_SIZE_LMv2         24
#define NTLM_RESPONSE_SIZE_NTLM         24
#define NTLM_RESPONSE_SIZE_NTLM2        24
#define NTLM_RESPONSE_SIZE_ANON_LM      1
#define NTLM_RESPONSE_SIZE_ANON_NTLM    0

#define NTLM_LM_MAX_PASSWORD_SIZE       14
#define NTLM_HASH_SIZE                  16
#define NTLM_SESSION_KEY_SIZE           16
#define NTLM_SIGNATURE_SIZE             16
#define NTLM_LM_DES_STRING              "KGS!@#$%"

#define NTLM_COUNTER_VALUE              0x78010900
#define NTLM_PADDING_SIZE               4

#define NTLM_INITIAL BLOB_SIZE          32

// Name types

#define NAME_TYPE_END         0x0000
#define NAME_TYPE_SERVER      0x0001
#define NAME_TYPE_DOMAIN      0x0002
#define NAME_TYPE_SERVER_DNS  0x0003
#define NAME_TYPE_DOMAIN_DNS  0x0004

// Target information block

#define NTLM_TIB_TERMINATOR         0x0000
#define NTLM_TIB_SERVER_NAME        0x0001
#define NTLM_TIB_DOMAIN_NAME        0x0002
#define NTLM_TIB_DNS_SERVER_NAME    0x0003
#define NTLM_TIB_DNS_DOMAIN_NAME    0x0004

// RNG for crypto

#define NTLM_RANDOM_DEV     "/dev/random"
#define NTLM_URANDOM_DEV    "/dev/urandom"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwTranslateKrb5Error(ctx, ret, __FUNCTION__, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#endif /* __DEFINES_H__ */
