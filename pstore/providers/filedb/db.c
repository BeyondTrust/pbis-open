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
 *        db.c
 *
 * Abstract:
 *
 *        Machine Password Database API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "includes.h"

#define FILEDB_FORMAT_VERSION 1

static pthread_rwlock_t g_MachinePwdDBLock;

#define ENTER_MACHINEPWD_DB_RW_READER_LOCK(bInLock) \
    if (!bInLock) {                                 \
        pthread_rwlock_rdlock(&g_MachinePwdDBLock); \
        bInLock = TRUE;                             \
    }
#define LEAVE_MACHINEPWD_DB_RW_READER_LOCK(bInLock) \
    if (bInLock) {                                  \
        pthread_rwlock_unlock(&g_MachinePwdDBLock); \
        bInLock = FALSE;                            \
    }

#define ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock) \
    if (!bInLock) {                                 \
        pthread_rwlock_wrlock(&g_MachinePwdDBLock); \
        bInLock = TRUE;                             \
    }
#define LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock) \
    if (bInLock) {                                  \
        pthread_rwlock_unlock(&g_MachinePwdDBLock); \
        bInLock = FALSE;                            \
    }

DWORD
FileDBDbInitGlobals()
{
    DWORD dwError = 0;
    
    pthread_rwlock_init(&g_MachinePwdDBLock, NULL);
    
    dwError = FileDBCreateDb();
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
FileDBCreateDb()
{
    DWORD dwError = 0;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    DWORD dwVersion = FILEDB_FORMAT_VERSION;

    dwError = LwpsCheckDirectoryExists(
                  MACHINEPWD_DB_DIR,
                  &bExists);
    BAIL_ON_LWPS_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU;
        
        dwError = LwpsCreateDirectory(
                      MACHINEPWD_DB_DIR,
                      cacheDirMode);
        BAIL_ON_LWPS_ERROR(dwError);
        
    }

    dwError = LwpsChangeOwner(MACHINEPWD_DB_DIR, 0, 0);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsCheckFileExists(
                  MACHINEPWD_DB,
                  &bExists);
    BAIL_ON_LWPS_ERROR(dwError);

    if (bExists)
    {
       goto cleanup;
    }

    pFileDb = fopen(MACHINEPWD_DB, "w");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsChangePermissions(MACHINEPWD_DB, S_IRWXU);
    BAIL_ON_LWPS_ERROR(dwError);
        
cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError))
    {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
FileDBSetPwdEntry(
    PVOID  pData,
    size_t DataSize
    )
{
    DWORD dwError = 0;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    DWORD dwVersion = FILEDB_FORMAT_VERSION;
    
    ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);
    
    pFileDb = fopen(MACHINEPWD_DB, "w");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fwrite(&DataSize, sizeof(DataSize), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fwrite(pData, DataSize, 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError))
    {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
FileDBGetPwdEntry(
    PVOID *  ppData,
    size_t * pDataSize
    )
{
    DWORD    dwError = 0;
    PSTR     pszError = NULL;
    BOOLEAN  bInLock = FALSE;
    FILE *   pFileDb = NULL;
    size_t   Cnt = 0;
    DWORD    dwVersion = 0;
    size_t   DataSize = 0;
    PVOID    pData = NULL;
    
    ENTER_MACHINEPWD_DB_RW_READER_LOCK(bInLock);
    
    pFileDb = fopen(MACHINEPWD_DB, "r");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fread(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt == 0)
    {
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
    }
    else if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fread(&DataSize, sizeof(DataSize), 1, pFileDb);
    if (Cnt == 0)
    {
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
    }
    else if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateMemory(
                  DataSize,
                  (PVOID*)&pData);
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fread(pData, DataSize, 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    *ppData = pData;
    *pDataSize = DataSize;

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    LEAVE_MACHINEPWD_DB_RW_READER_LOCK(bInLock);

    return dwError;
    
error:

    *ppData = NULL;
    *pDataSize = 0;

    if (!IsNullOrEmptyString(pszError))
    {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
FileDBDeleteAllEntries(
    void
    )
{
    DWORD   dwError = 0;
    PSTR    pszError = NULL;
    BOOLEAN bInLock = FALSE;
    FILE *  pFileDb = NULL;
    size_t  Cnt = 0;
    DWORD   dwVersion = FILEDB_FORMAT_VERSION;
    
    ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);
    
    pFileDb = fopen(MACHINEPWD_DB, "w");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LWPS_ERROR(dwError);

    Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError))
    {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;    
}
