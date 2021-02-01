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
 *        smbdef.h
 *
 * Abstract:
 *
 *        BeyondTrust IO (LWIO)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __SMBDEF_H__
#define __SMBDEF_H__

#ifndef SMB_API
#define SMB_API
#endif

#define SMB_SECONDS_IN_MINUTE (60)
#define SMB_SECONDS_IN_HOUR   (60 * SMB_SECONDS_IN_MINUTE)
#define SMB_SECONDS_IN_DAY    (24 * SMB_SECONDS_IN_HOUR)

#ifndef SMB_MAX
#define SMB_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SMB_MIN
#define SMB_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define SMB_DEFAULT_HANDLE_MAX 100000

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#if defined(HAVE_SOCKLEN_T) && defined(GETSOCKNAME_TAKES_SOCKLEN_T)
#    define SOCKLEN_TYPE socklen_t
#else
#    define SOCKLEN_TYPE int
#endif

#define LW_ASSERT(x)   ( (x) ? ((void) 0) : assert( (x) ) )

typedef ULONG KRB5_TIME, KRB5_ENCTYPE, KRB5_FLAGS;

struct __LW_IO_CREDS
{
    enum _LW_IO_CREDS_TYPE
    {
        IO_CREDS_TYPE_PLAIN = 0,
        IO_CREDS_TYPE_KRB5_CCACHE = 1,
        IO_CREDS_TYPE_KRB5_TGT = 2
    } type;
    union _LW_IO_CREDS_U
    {
        struct _LW_IO_CREDS_PLAIN
        {
            PWSTR pwszUsername;
            PWSTR pwszDomain;
            PWSTR pwszPassword;
        } plain;
        struct _LW_IO_CREDS_KRB5_CCACHE
        {
            PWSTR pwszPrincipal;
            PWSTR pwszCachePath;
        } krb5Ccache;
        struct _LW_IO_CREDS_KRB5_TGT
        {
            PWSTR pwszClientPrincipal;
            PWSTR pwszServerPrincipal;
            KRB5_TIME authTime;
            KRB5_TIME startTime;
            KRB5_TIME endTime;
            KRB5_TIME renewTillTime;
            KRB5_ENCTYPE keyType;
            ULONG ulKeySize;
            PBYTE pKeyData;
            KRB5_FLAGS tgtFlags;
            ULONG ulTgtSize;
            PBYTE pTgtData;
        } krb5Tgt;
    } payload;
};

typedef struct SMB_FILE_HANDLE SMB_FILE_HANDLE, *PSMB_FILE_HANDLE;

#endif /* __SMBDEF_H__ */
