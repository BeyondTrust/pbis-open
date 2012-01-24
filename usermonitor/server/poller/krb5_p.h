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
 *        krb5_p.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 * 
 *        Kerberos ticket and credentials cache helper functions
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 * 
 */
#ifndef __KRB5_P_H__
#define __KRB5_P_H__

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = UmnTranslateKrb5Error(ctx, ret, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                     \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {             \
        UMN_LOG_ERROR("GSS API Error: %d", dwMajorStatus);      \
        dwError = STATUS_ENCRYPTION_FAILED;                    \
        goto error;                                             \
    }

DWORD
UmnTranslateKrb5Error(
    krb5_context ctx,
    krb5_error_code krbError,
    PCSTR pszFile,
    DWORD dwLine
    );

DWORD
UmnSrvSetupCredCache(
    LW_PIO_CREDS* ppAccessToken
    );

#endif /* __KRB5_P_H__ */
