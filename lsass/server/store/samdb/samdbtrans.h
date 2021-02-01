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



/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbtrans.h
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
 *
 *      Transaction
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#ifndef __SAM_DB_TRANS_H__
#define __SAM_DB_TRANS_H__

#define SAM_DB_BEGIN_TRANSACTION(bTxStarted, hDirectory) \
    if (!(dwError = SamDbBeginTransaction(hDirectory))) { \
        bTxStarted = TRUE; \
    } else {\
        BAIL_ON_SAMDB_ERROR(dwError); \
    }

#define SAM_DB_END_TRANSACTION(bTxStarted, dwError, hDirectory) \
    if (bTxStarted) \
    { \
        DWORD dwError1 = 0; \
        if (dwError) { \
            if ((dwError1 = SamDbRollbackTransaction(hDirectory))) { \
                SAMDB_LOG_ERROR("Failed to rollback transaction [code:%u]", dwError1); \
            } \
        } \
        else { \
            if ((dwError1 = SamDbCommitTransaction(hDirectory))) { \
                SAMDB_LOG_ERROR("Failed to commit transaction [code:%u]", dwError1); \
            } \
        } \
    }

DWORD
SamDbBeginTransaction(
    HANDLE hDirectory
    );

DWORD
SamDbRollbackTransaction(
    HANDLE hDirectory
    );

DWORD
SamDbCommitTransaction(
    HANDLE hDirectory
    );

#endif /* __SAM_DB_TRANS_H__ */
