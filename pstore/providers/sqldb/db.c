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
#include "lwps-utils.h"
#include "sqlite3.h"
#include "lwps-sid.h"
#include "db_p.h"
#include "dbquery.h"

#define MACHINEPWD_DB_DIR LWPS_CACHE_DIR "/db"
#define MACHINEPWD_DB     MACHINEPWD_DB_DIR "/pstore.db"

static pthread_rwlock_t g_MachinePwdDBLock;
static BOOLEAN g_MachinePwdDBLockInit = FALSE;

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

VOID
SqlDBFreeMachineAcctInfo(
    PMACHINE_ACCT_INFO pAcct
    )
{
    LWPS_SAFE_FREE_STRING(pAcct->pszDomainSID);
    LWPS_SAFE_FREE_STRING(pAcct->pszDomainName);
    LWPS_SAFE_FREE_STRING(pAcct->pszDomainDnsName);
    LWPS_SAFE_FREE_STRING(pAcct->pszHostName);
    LWPS_SAFE_FREE_STRING(pAcct->pszHostDnsDomain);
    LWPS_SAFE_FREE_STRING(pAcct->pszMachineAccountName);
    LWPS_SAFE_FREE_STRING(pAcct->pszMachineAccountPassword);
    
    LWPS_SAFE_FREE_MEMORY(pAcct);
}


DWORD
SqlDBDbInitGlobals()
{
    DWORD dwError = 0;
   
    if (!g_MachinePwdDBLockInit)
    {
        pthread_rwlock_init(&g_MachinePwdDBLock, NULL);
        g_MachinePwdDBLockInit = TRUE;
    }
    
    dwError = SqlDBCreateDb();
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

//
// TODO: Implement a DB Handle Pool
//
DWORD
SqlDBOpen(
    PHANDLE phDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = NULL;
    
    dwError = sqlite3_open(MACHINEPWD_DB, &pDbHandle);
    BAIL_ON_LWPS_ERROR(dwError);
    
    *phDb = (HANDLE)pDbHandle;
    
cleanup:

    return dwError;
    
error:

    *(phDb) = (HANDLE)NULL;
    
    if (pDbHandle) {
        sqlite3_close(pDbHandle);
    }

    goto cleanup;
}

/*
 * This function assumes queries are a single atomic statement
 */
static
DWORD
SqlDBExec(
    sqlite3 * pDbHandle,
    PCSTR pszQuery,
    int (*callback)(void*, int, char**, char**),
    void * arg,
    PSTR *ppszError)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;

    do {
        dwError = sqlite3_exec(pDbHandle, pszQuery, callback, arg,
                ppszError);
        if (dwError == SQLITE_BUSY)
        {
            usleep(dwCount * 1000);
            if (dwCount < 50)
                dwCount++;
        }
    } while (dwError == SQLITE_BUSY);

    return dwError;
}

static
DWORD
SqlDBGetTable(
    sqlite3 *pDbHandle,
    PCSTR pszQuery,
    PSTR **pppszDbResult,
    int * pNumRows,
    int * pNumCols,
    PSTR * ppszError)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;

    do {
        dwError = sqlite3_get_table(pDbHandle, pszQuery, pppszDbResult,
                pNumRows, pNumCols, ppszError);

        if (dwError == SQLITE_BUSY)
        {
            usleep(dwCount * 1000);
            if (dwCount < 50)
                dwCount++;
        }
    } while (dwError == SQLITE_BUSY);

    return dwError;
}


void
SqlDBClose(
    HANDLE hDb
    )
{
    sqlite3* pDbHandle = (sqlite3*)hDb;
    if (pDbHandle) {
       sqlite3_close(pDbHandle);
    }
}

DWORD
SqlDBCreateDb()
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    sqlite3* pDbHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LwpsCheckDirectoryExists(MACHINEPWD_DB_DIR, &bExists);
    BAIL_ON_LWPS_ERROR(dwError);

    if (!bExists) {
        
        mode_t cacheDirMode = S_IRWXU;
        
        dwError = LwpsCreateDirectory(MACHINEPWD_DB_DIR, cacheDirMode);
        BAIL_ON_LWPS_ERROR(dwError);
        
    }

    dwError = LwpsChangeOwner(MACHINEPWD_DB_DIR, 0, 0);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = LwpsCheckFileExists(MACHINEPWD_DB, &bExists);
    BAIL_ON_LWPS_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists) {
       goto cleanup;
    }

    dwError = SqlDBOpen(&hDb);
    BAIL_ON_LWPS_ERROR(dwError);
    
    pDbHandle = (sqlite3*)hDb;
        
    dwError = SqlDBExec(pDbHandle,
                           DB_QUERY_CREATE_MACHINEPWD_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsChangePermissions(MACHINEPWD_DB, S_IRWXU);
    BAIL_ON_LWPS_ERROR(dwError);
        
cleanup:

    if (hDb != (HANDLE)NULL) {
        SqlDBClose(hDb);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
SqlDBSetPwdEntry(
    HANDLE hDb,
    PMACHINE_ACCT_INFO pAcct
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PLWPS_SECURITY_IDENTIFIER pSID = NULL;
    BOOLEAN bInLock = FALSE;
    
    //verify that pszDomainSID is properly formatted
    dwError = LwpsAllocSecurityIdentifierFromString(
                    pAcct->pszDomainSID,
                    &pSID);
    BAIL_ON_LWPS_ERROR(dwError);
    
    ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);
     
    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_MACHINEPWD_ENTRY_BY_HOST_NAME,
                    pAcct->pszHostName);
    if (!pszQuery) {
        dwError = LWPS_ERROR_QUERY_CREATION_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = SqlDBExec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);

    sqlite3_free(pszQuery);    
    pszQuery = sqlite3_mprintf(
                    DB_QUERY_INSERT_MACHINEPWD_ENTRY,
                    pAcct->pszDomainSID,
                    pAcct->pszDomainName,
                    pAcct->pszDomainDnsName,
                    pAcct->pszHostName,
                    pAcct->pszHostDnsDomain,
                    pAcct->pszMachineAccountName,
                    pAcct->pszMachineAccountPassword,
                    (DWORD) time(NULL),
                    (DWORD) pAcct->tPwdClientModifyTimestamp,
                    pAcct->dwSchannelType);
    if (!pszQuery) {
        dwError = LWPS_ERROR_QUERY_CREATION_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = SqlDBExec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }
    
    LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);

    if (pSID != NULL)
    {
        LwpsFreeSecurityIdentifier(pSID);
    }
    
    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError)) {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
SqlDBGetPwdEntry(
    HANDLE hDb,
    PCSTR pszQuery,
    PMACHINE_ACCT_INFO* ppAcct
    )
{
    DWORD    dwError = 0;
    PSTR     pszError = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR*    ppszDbResult = NULL;
    int      numRows = 0;
    int      numCols = 0;
    INT      i = 0;
    PSTR     pszValue = NULL;
    PMACHINE_ACCT_INFO pAcct = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD   dwExpectedNumCols = 10;
    PSTR    pszEndPtr = NULL;
    
    ENTER_MACHINEPWD_DB_RW_READER_LOCK(bInLock);
    
    dwError = SqlDBGetTable(pDbHandle,
                           pszQuery,
                           &ppszDbResult,
                           &numRows,
                           &numCols,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);

    if (ppszDbResult == NULL ||
        !numRows ||
        IsNullOrEmptyString( ppszDbResult[1] ))
    {
        dwError = LWPS_ERROR_INVALID_ACCOUNT;
    }
    else if (numRows != 1) 
    {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;  
    }    
    else if (numCols != dwExpectedNumCols) {
        dwError  = LWPS_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LWPS_ERROR(dwError);
    
    
    i = dwExpectedNumCols;

    dwError = LwpsAllocateMemory(
                    sizeof(MACHINE_ACCT_INFO),
                    (PVOID*)&pAcct);
    BAIL_ON_LWPS_ERROR(dwError);
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszDomainSID);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszDomainName);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszDomainDnsName);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszHostName);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    else if (!IsNullOrEmptyString(pAcct->pszDomainDnsName))
    {
        dwError = LwpsAllocateString(
                    pAcct->pszDomainDnsName,
                    &pAcct->pszHostDnsDomain);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszMachineAccountName);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pszValue = ppszDbResult[i++];
    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LwpsAllocateString(
                    pszValue,
                    &pAcct->pszMachineAccountPassword);
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    pAcct->tPwdCreationTimestamp = (time_t) strtoll(ppszDbResult[i],
                                                    &pszEndPtr,
                                                    10);
    if (!pszEndPtr || (pszEndPtr == ppszDbResult[i]) || *pszEndPtr)
    {
       dwError = LWPS_ERROR_DATA_ERROR;
       BAIL_ON_LWPS_ERROR(dwError);
    }

    i++;

    pAcct->tPwdClientModifyTimestamp = (time_t) strtoll(ppszDbResult[i],
                                                    &pszEndPtr,
                                                    10);
    if (!pszEndPtr || (pszEndPtr == ppszDbResult[i]) || *pszEndPtr)
    {
       dwError = LWPS_ERROR_DATA_ERROR;
       BAIL_ON_LWPS_ERROR(dwError);
    }

    i++;
    
    pAcct->dwSchannelType = (UINT32)atol(ppszDbResult[i++]);
    
    *ppAcct = pAcct;
    
cleanup:

    if (ppszDbResult) {
        sqlite3_free_table(ppszDbResult);
    }
    
    LEAVE_MACHINEPWD_DB_RW_READER_LOCK(bInLock);

    return dwError;
    
error:

    if(pAcct != NULL)
    {
        SqlDBFreeMachineAcctInfo(pAcct);
    }

    if (!IsNullOrEmptyString(pszError)) {
        LWPS_LOG_ERROR(pszError);
    }
    
    *ppAcct = NULL;

    goto cleanup;
}

DWORD
SqlDBGetPwdEntryByDomainDnsName(
    HANDLE hDb,
    PCSTR  pszDomainDnsName,
    PMACHINE_ACCT_INFO* ppAcct
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PMACHINE_ACCT_INFO pAcct = NULL;
    
    pszQuery = sqlite3_mprintf(
                    DB_QUERY_GET_MACHINEPWD_BY_DOMAIN_DNS_NAME,
                    pszDomainDnsName);
    if (!pszQuery) {
        dwError = LWPS_ERROR_QUERY_CREATION_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = SqlDBGetPwdEntry(
                hDb,
                pszQuery,
                &pAcct);
    BAIL_ON_LWPS_ERROR(dwError);
    
    *ppAcct = pAcct;
    
cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }
    
    return dwError;
    
error:
    
    if(pAcct)
    {
        SqlDBFreeMachineAcctInfo(pAcct);
    }
    
    *ppAcct = NULL;
    
    goto cleanup;
}

DWORD
SqlDBGetPwdEntryByHostName(
    HANDLE hDb,
    PCSTR  pszHostName,
    PMACHINE_ACCT_INFO* ppAcct
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PMACHINE_ACCT_INFO pAcct = NULL;
    
    pszQuery = sqlite3_mprintf(
                    DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME,
                    pszHostName);
    if (!pszQuery) {
        dwError = LWPS_ERROR_QUERY_CREATION_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = SqlDBGetPwdEntry(
                    hDb,
                    pszQuery,
                    &pAcct);
    BAIL_ON_LWPS_ERROR(dwError);
    
    *ppAcct = pAcct;
    
cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }
    
    return dwError;
    
error:
    
    if (pAcct)
    {
        SqlDBFreeMachineAcctInfo(pAcct);
    }
    
    *ppAcct = NULL;
    
    goto cleanup;
}

DWORD
SqlDBDeletePwdEntryByHostName(
    HANDLE hDb,
    PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PSTR  pszError = NULL;
    PSTR  pszQuery = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    BOOLEAN bInLock = FALSE;
    
    ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);
    
    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_MACHINEPWD_ENTRY_BY_HOST_NAME,
                    pszHostName);
    if (!pszQuery) {
        dwError = LWPS_ERROR_QUERY_CREATION_FAILED;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = SqlDBExec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }

    LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError)) {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;
}

DWORD
SqlDBDeleteAllEntries(
    HANDLE hDb
    )
{
    DWORD dwError = 0;
    PSTR  pszError = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    BOOLEAN bInLock = FALSE;
    
    ENTER_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);
    
    dwError = SqlDBExec(pDbHandle,
                           DB_QUERY_DELETE_ALL_MACHINEPWD_ENTRIES,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);
    
cleanup:

    LEAVE_MACHINEPWD_DB_RW_WRITER_LOCK(bInLock);

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError)) {
        LWPS_LOG_ERROR(pszError);
    }

    goto cleanup;    
}

DWORD
SqlDBGetPwdEntries(
    HANDLE                   hDb,
    PMACHINE_ACCT_INFO** pppAcct,
    PDWORD                   pdwNumEntries
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = 0;
    PMACHINE_ACCT_INFO* ppAcct = NULL;
    PMACHINE_ACCT_INFO pAcct = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    int     iRow = 0, numRows = 0;
    int     iCol = 0, numCols = 0;
    int     iVal = 0;
    DWORD   dwExpectedNumCols = 10;
    PSTR    pszError = NULL;
    PSTR*   ppszDbResult = NULL;
    PSTR    pszEndPtr = NULL;
    
    BAIL_ON_INVALID_HANDLE(hDb);
    
    ENTER_MACHINEPWD_DB_RW_READER_LOCK(bInLock);
    
    dwError = SqlDBGetTable(
                           pDbHandle,
                           DB_QUERY_GET_MACHINEPWD_ENTRIES,
                           &ppszDbResult,
                           &numRows,
                           &numCols,
                           &pszError);
    BAIL_ON_LWPS_ERROR(dwError);

    if (!ppszDbResult || !numRows || 
        IsNullOrEmptyString(ppszDbResult[1]))
    {
        goto done;
    }
    
    if (numCols != dwExpectedNumCols) {
        dwError  = LWPS_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LWPS_ERROR(dwError);
    }
    
    dwError = LwpsAllocateMemory(
                    (numRows) * sizeof(PMACHINE_ACCT_INFO),
                    (PVOID*)&ppAcct);
    BAIL_ON_LWPS_ERROR(dwError);
   
    iVal = dwExpectedNumCols; // Header columns to skip
    for (iRow = 0; iRow < numRows; iRow++)
    {
        dwError = LwpsAllocateMemory(
                        sizeof(MACHINE_ACCT_INFO),
                        (PVOID*)&pAcct);
        BAIL_ON_LWPS_ERROR(dwError);
        
        for (iCol = 0; iCol < numCols; iCol++)
        {
            switch(iCol)
            {
                case 0:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszDomainSID);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
               
                case 1:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszDomainName);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 2:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszDomainDnsName);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 3:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszHostName);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 4:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszHostDnsDomain);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 5:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszMachineAccountName);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 6:
                    
                    if (!IsNullOrEmptyString(ppszDbResult[iVal]))
                    {
                        dwError = LwpsAllocateString(
                                    ppszDbResult[iVal],
                                    &pAcct->pszMachineAccountPassword);
                        BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 7:
                    
                    pAcct->tPwdCreationTimestamp = 
                                (time_t) strtoll(ppszDbResult[iVal],
                                                 &pszEndPtr,
                                                 10);
                    if (!pszEndPtr ||
                        (ppszDbResult[iVal] == pszEndPtr) ||
                        *pszEndPtr)
                    {
                       dwError = LWPS_ERROR_DATA_ERROR;
                       BAIL_ON_LWPS_ERROR(dwError);
                    }

                    break;
                
                case 8:
                    
                    pAcct->tPwdClientModifyTimestamp = 
                                (time_t) strtoll(ppszDbResult[iVal],
                                                 &pszEndPtr,
                                                 10);
                    if (!pszEndPtr ||
                        (ppszDbResult[iVal] == pszEndPtr) ||
                        *pszEndPtr)
                    {
                       dwError = LWPS_ERROR_DATA_ERROR;
                       BAIL_ON_LWPS_ERROR(dwError);
                    }
                    break;
                    
                case 9:
                    
                    pAcct->dwSchannelType = (UINT32)atol(ppszDbResult[iVal]);
                    break;
            }
            iVal++;
        }
        
        *(ppAcct + iRow) = pAcct;
        pAcct = NULL;
    }
    
done:

    *pppAcct = ppAcct;
    *pdwNumEntries = numRows;
    
cleanup:

    if (ppszDbResult) {
        sqlite3_free_table(ppszDbResult);
    }

    LEAVE_MACHINEPWD_DB_RW_READER_LOCK(bInLock);

    return dwError;
    
error:

    *pppAcct = NULL;
    *pdwNumEntries = 0;
    
    if (ppAcct) {
        SqlDBFreeEntryList(ppAcct, numRows);
    }

    goto cleanup;
}

VOID
SqlDBFreeEntryList(
    PMACHINE_ACCT_INFO* ppAcct,
    DWORD               dwNumEntries
    )
{
    DWORD iRow = 0;
    
    for (iRow = 0; iRow < dwNumEntries; iRow++)
    {
        if (*(ppAcct+iRow)) {
            SqlDBFreeMachineAcctInfo(*(ppAcct+iRow));
        }
    }
    LwpsFreeMemory(ppAcct);
}
