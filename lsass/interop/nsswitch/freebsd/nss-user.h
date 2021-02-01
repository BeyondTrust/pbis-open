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
 *        nss-user.h
 *
 * Abstract:
 * 
 *        Name Server Switch (BeyondTrust LSASS)
 * 
 *        Handle NSS User Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __LSANSSUSER_H__
#define __LSANSSUSER_H__

NSS_STATUS
_nss_lsass_setpwent(
    void
    );

NSS_STATUS
_nss_lsass_getpwent_r(
    struct passwd * pUser,
    char *          pszBuf,
    size_t          bufLen,
    int*            pErrorNumber
    );

NSS_STATUS
_nss_lsass_endpwent(
    void
    );

NSS_STATUS
_nss_lsass_getpwnam_r(
    const char *    pszName,
    struct passwd * pUser,
    char *          pszBuf,
    size_t          bufLen,
    int*            pErrorNumber
    );

NSS_STATUS
_nss_lsass_getpwuid_r(
    uid_t           uid,
    struct passwd*  pUser,
    char *          pszBuf,
    size_t          bufLen,
    int*            pErrorNumber
    );

#endif /* __LSANSSUSER_H__ */

