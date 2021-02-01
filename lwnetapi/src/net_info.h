/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#ifndef _NET_INFO_H_
#define _NET_INFO_H_


#include "includes.h"

/*
 * push functions/macros transfer: net userinfo -> samr userinfo
 * pull functions/macros transfer: net userinfo <- samr userinfo
 */

#define PULL_ACCOUNT_FLAG(acct_flags, netacct_flags) \
    if (samrflags & (acct_flags)) {                  \
        flags |= (netacct_flags);                    \
    }


#define PULL_ACCOUNT_FLAGS(net_flags, samr_flags) \
    do {                                          \
        UINT32 flags = 0;                       \
        UINT32 samrflags = (samr_flags);        \
        \
        PULL_ACCOUNT_FLAG(ACB_DISABLED, UF_ACCOUNTDISABLE); \
        PULL_ACCOUNT_FLAG(ACB_HOMDIRREQ, UF_HOMEDIR_REQUIRED); \
        PULL_ACCOUNT_FLAG(ACB_PWNOTREQ, UF_PASSWD_NOTREQD); \
        PULL_ACCOUNT_FLAG(ACB_TEMPDUP, UF_TEMP_DUPLICATE_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_NORMAL, UF_NORMAL_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_DOMTRUST, UF_INTERDOMAIN_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_WSTRUST, UF_WORKSTATION_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_SVRTRUST, UF_SERVER_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_PWNOEXP, UF_DONT_EXPIRE_PASSWD); \
        PULL_ACCOUNT_FLAG(ACB_ENC_TXT_PWD_ALLOWED, UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED); \
        PULL_ACCOUNT_FLAG(ACB_SMARTCARD_REQUIRED, UF_SMARTCARD_REQUIRED); \
        PULL_ACCOUNT_FLAG(ACB_TRUSTED_FOR_DELEGATION, UF_TRUSTED_FOR_DELEGATION); \
        PULL_ACCOUNT_FLAG(ACB_NOT_DELEGATED, UF_NOT_DELEGATED); \
        PULL_ACCOUNT_FLAG(ACB_USE_DES_KEY_ONLY, UF_USE_DES_KEY_ONLY); \
        PULL_ACCOUNT_FLAG(ACB_DONT_REQUIRE_PREAUTH, UF_DONT_REQUIRE_PREAUTH); \
        PULL_ACCOUNT_FLAG(ACB_PW_EXPIRED, UF_PASSWORD_EXPIRED); \
        \
        (net_flags) = flags;                    \
    } while (0);


#define PUSH_ACCOUNT_FLAG(netacct_flags, samracct_flags) \
    if (netflags & (netacct_flags)) {                    \
        flags |= (samracct_flags);                       \
    }


#define PUSH_ACCOUNT_FLAGS(info, net_flags, samr_flags, field_present) \
    do {                                                              \
        UINT32 flags = 0; \
        UINT32 netflags = (net_flags);          \
        \
        PUSH_ACCOUNT_FLAG(UF_ACCOUNTDISABLE, ACB_DISABLED); \
        PUSH_ACCOUNT_FLAG(UF_HOMEDIR_REQUIRED, ACB_HOMDIRREQ); \
        PUSH_ACCOUNT_FLAG(UF_PASSWD_NOTREQD, ACB_PWNOTREQ); \
        PUSH_ACCOUNT_FLAG(UF_TEMP_DUPLICATE_ACCOUNT, ACB_TEMPDUP); \
        PUSH_ACCOUNT_FLAG(UF_NORMAL_ACCOUNT, ACB_NORMAL); \
        PUSH_ACCOUNT_FLAG(UF_INTERDOMAIN_TRUST_ACCOUNT, ACB_DOMTRUST); \
        PUSH_ACCOUNT_FLAG(UF_WORKSTATION_TRUST_ACCOUNT, ACB_WSTRUST); \
        PUSH_ACCOUNT_FLAG(UF_SERVER_TRUST_ACCOUNT, ACB_SVRTRUST); \
        PUSH_ACCOUNT_FLAG(UF_DONT_EXPIRE_PASSWD, ACB_PWNOEXP);	\
        PUSH_ACCOUNT_FLAG(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED, ACB_ENC_TXT_PWD_ALLOWED); \
        PUSH_ACCOUNT_FLAG(UF_SMARTCARD_REQUIRED, ACB_SMARTCARD_REQUIRED); \
        PUSH_ACCOUNT_FLAG(UF_TRUSTED_FOR_DELEGATION, ACB_TRUSTED_FOR_DELEGATION); \
        PUSH_ACCOUNT_FLAG(UF_NOT_DELEGATED, ACB_NOT_DELEGATED); \
        PUSH_ACCOUNT_FLAG(UF_USE_DES_KEY_ONLY, ACB_USE_DES_KEY_ONLY); \
        PUSH_ACCOUNT_FLAG(UF_DONT_REQUIRE_PREAUTH, ACB_DONT_REQUIRE_PREAUTH); \
        PUSH_ACCOUNT_FLAG(UF_PASSWORD_EXPIRED, ACB_PW_EXPIRED); \
        \
        (info)->samr_flags = flags;          \
        (info)->fields_present |= (field_present);    \
    } while (0);


#endif /* _NET_INFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
