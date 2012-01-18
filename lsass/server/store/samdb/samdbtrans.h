/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        samdbtrans.h
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
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
