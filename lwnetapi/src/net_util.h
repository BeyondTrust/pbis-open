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

#ifndef _NET_UTIL_H_
#define _NET_UTIL_H_


#define BAIL_ON_NT_STATUS(err)               \
    if ((err) != STATUS_SUCCESS) {           \
        goto error;                          \
    }

#define BAIL_ON_WIN_ERROR(err)               \
    if ((err) != ERROR_SUCCESS) {            \
        goto error;                          \
    }

#define BAIL_ON_LDERR_ERROR(e)               \
    if ((e) != LDAP_SUCCESS) {               \
        lderr = (e);                         \
        goto error;                          \
    }

#define BAIL_ON_NO_MEMORY(p, err)            \
    if ((p) == NULL) {                       \
        err = ERROR_OUTOFMEMORY;             \
        goto error;                          \
    }

#define BAIL_ON_INVALID_PTR(p, err)          \
    if ((p) == NULL) {                       \
        err = ERROR_INVALID_PARAMETER;       \
        goto error;                          \
    }

#define goto_if_no_memory_lderr(p, lbl) \
    if ((p) == NULL) {                  \
        lderr = LDAP_NO_MEMORY;         \
        err = ERROR_OUTOFMEMORY;        \
        goto lbl;                       \
    }


#if !defined(MACHPASS_LEN)
#define MACHPASS_LEN  (16)
#endif


#endif /* _NET_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
