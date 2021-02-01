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
 *        lwkrb5_p.h
 *
 * Abstract:
 *
 *        BeyondTrust Advanced API (lwadvapi) 
 *        
 *        KRB5 API (Private Header)
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __LWKRB5_P_H__
#define __LWKRB5_P_H__

#define AD_IF_RELEVANT_TYPE 1
#define AD_WIN2K_PAC        128

#define PAC_TYPE_LOGON_INFO              1
#define PAC_TYPE_SRV_CHECKSUM            6
#define PAC_TYPE_KDC_CHECKSUM            7
#define PAC_TYPE_LOGON_NAME             10
#define PAC_TYPE_CONSTRAINED_DELEGATION 11

typedef struct _PAC_BUFFER {
    DWORD dwType;
    DWORD dwSize;
    uint64_t qwOffset;
} PAC_BUFFER;

typedef struct _PAC_DATA {
    DWORD dwBufferCount;
    DWORD dwVersion;
    PAC_BUFFER buffers[1];
} PAC_DATA;

typedef struct _PAC_SIGNATURE_DATA {
    DWORD dwType;
    // Goes until the end of the buffer (defined in PAC_DATA)
    char pchSignature[1];
} PAC_SIGNATURE_DATA;

typedef uint64_t NtTime;

typedef struct _PAC_LOGON_NAME {
    NtTime ticketTime;
    WORD wAccountNameLen;
    wchar16_t pwszName[1];
} PAC_LOGON_NAME;

DWORD
LwKrb5CopyFromUserCache(
                krb5_context ctx,
                krb5_ccache destCC,
                uid_t uid
                );

DWORD
LwKrb5MoveCCacheToUserPath(
                krb5_context ctx,
                PCSTR pszNewCacheName,
                uid_t uid,
                gid_t gid
                );

DWORD
LwKrb5VerifyPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const struct berval *pPacBerVal,
    const krb5_keyblock *serviceKey,
    char** ppchLogonInfo,
    size_t* psLogonInfo
    );

DWORD
LwKrb5FindPac(
    krb5_context ctx,
    const krb5_ticket *pTgsTicket,
    const krb5_keyblock *serviceKey,
    OUT PVOID* ppchLogonInfo,
    OUT size_t* psLogonInfo
    );

#endif /* __LWKRB5_P_H__ */
