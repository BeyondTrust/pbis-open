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
 *        evtfwd-def.h
 *
 * Abstract:
 *
 *        User monitor service for local users and groups
 *
 *        Common definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __UMN_DEF_H__
#define __UMN_DEF_H__

#define BAIL_ON_UMN_ERROR(dwError) \
    if (dwError) {                    \
       UMN_LOG_DEBUG("Error in %s at %s:%d [code: %d]", __FUNCTION__, __FILE__, __LINE__, dwError); \
       goto error;                    \
    }

#define BAIL_ON_INVALID_STRING(pszParam)             \
        if (LW_IS_NULL_OR_EMPTY_STR(pszParam)) {         \
           dwError = EINVAL; \
           BAIL_ON_UMN_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_HANDLE(hParam)               \
        if (hParam == (HANDLE)NULL) {                \
           dwError = EINVAL; \
           BAIL_ON_UMN_ERROR(dwError);            \
        }

#define BAIL_ON_INVALID_POINTER(p)                   \
        if (NULL == p) {                             \
           dwError = EINVAL; \
           BAIL_ON_UMN_ERROR(dwError);            \
        }

#define UMN_NEXT_RECORD_DB     UMN_CACHE_DIR "/db/eventfwd-next-record.db"

#endif /* __UMN_DEF_H__ */
