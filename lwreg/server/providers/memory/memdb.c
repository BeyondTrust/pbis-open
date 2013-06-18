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
 *        memdb.c
 *
 * Abstract:
 *        Database implementation for registry memory provider backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "includes.h"
#if 0
#define __MEMDB_PRINTF__ 1
#endif
/*
 * All functions that implement the memory-based registry
 * provider are implemented in this file.
 */



NTSTATUS
MemDbOpen(
    OUT PMEMREG_NODE *ppDbRoot
    )
{
    NTSTATUS status = 0;
    PMEMREG_NODE pDbRoot = NULL;

    status = MemRegStoreOpen(&pDbRoot);
    BAIL_ON_NT_STATUS(status);

    *ppDbRoot = pDbRoot;

cleanup:
    return status;

error:
    goto cleanup;
}


static void *pfDeleteNodeCallback(
    PMEMREG_NODE pEntry,
    PVOID userContext,
    PWSTR subStringPrefix,
    NTSTATUS *pStatus)
{
    NTSTATUS status = 0;

    status = MemRegStoreDeleteNode(pEntry);
    if (pStatus)
    {
        *pStatus = status;
    }

    return NULL;
}


void *
pfMemRegExportToFile(
    PMEMREG_NODE pEntry, 
    PVOID userContext,
    PWSTR subStringPrefix,
    NTSTATUS *pstatus)
{
    DWORD dwError = 0;
    PSTR pszDumpString = NULL;
    PSTR pszValueName = NULL;
    PSTR pszValueNameEsc = NULL;
    DWORD dwValueNameEscLen = 0;
    PSTR pszEnumValue = NULL;
    PSTR pszStringSecurityDescriptor = NULL;
    SECURITY_INFORMATION SecInfoAll = OWNER_SECURITY_INFORMATION
                                     |GROUP_SECURITY_INFORMATION
                                     |DACL_SECURITY_INFORMATION
                                     |SACL_SECURITY_INFORMATION;
    DWORD dwDumpStringLen = 0;
    DWORD index = 0;
    DWORD enumIndex = 0;
    DWORD valueType = 0;
    PMEMREG_VALUE Value = NULL;
    PLWREG_VALUE_ATTRIBUTES Attr = NULL;
    PMEMDB_FILE_EXPORT_CTX exportCtx = (PMEMDB_FILE_EXPORT_CTX) userContext;
    char fmtbuf[64]; // Way longer than needed for data format
    int wfd = exportCtx->wfd;
    int sts = 0;

    /* Format key first */
    LWREG_SAFE_FREE_STRING(pszDumpString);
    dwError = RegExportEntry(
                  (PSTR) subStringPrefix,
                  "", // PCSTR pszSddlCString,
                  0, //     REG_DATA_TYPE valueType,
                  NULL, //     PCSTR valueName,
                  REG_WKEY, // DWORD Type
                  NULL, // LW_PVOID value,
                  0, //     DWORD valueLen,
                  &pszDumpString,
                  &dwDumpStringLen);
    /* Map to NT error status ? */
    BAIL_ON_NT_STATUS(dwError);
    sts = write(wfd, pszDumpString, dwDumpStringLen);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
    sts = write(wfd, "\n", 1);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));

    if ((pEntry->NodeType == MEMREG_TYPE_KEY ||
        pEntry->NodeType == MEMREG_TYPE_HIVE) &&
        pEntry->pNodeSd && pEntry->pNodeSd->SecurityDescriptorAllocated)
    {
        dwError = RegNtStatusToWin32Error(
                     RtlAllocateSddlCStringFromSecurityDescriptor(
                         &pszStringSecurityDescriptor,
                         pEntry->pNodeSd->SecurityDescriptor,
                         SDDL_REVISION_1,
                         SecInfoAll));
        BAIL_ON_NT_STATUS(dwError);
        sts = write(wfd, "@security=", 10);
        BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
        sts = write(wfd,
                    pszStringSecurityDescriptor,
                    strlen(pszStringSecurityDescriptor));
        BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
        sts = write(wfd, "\n", 1);
        BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
        LWREG_SAFE_FREE_STRING(pszStringSecurityDescriptor);
    }

    if (pEntry->Values)
    {
        /* Iterate through all values */
        for (index=0; index<pEntry->ValuesLen; index++)
        {
            Value = pEntry->Values[index];

            /* Fix up string type, as value is PWSTR */
            if (Value->Type == REG_SZ)
            {
                valueType = REG_WSZ;
            }
            else
            {
                valueType = Value->Type;
            }
            LWREG_SAFE_FREE_STRING(pszValueName);
            LWREG_SAFE_FREE_STRING(pszValueNameEsc);
            dwError = LwRtlCStringAllocateFromWC16String(
                          &pszValueName, 
                          Value->Name);
            BAIL_ON_REG_ERROR(dwError);
            dwError = RegShellUtilEscapeString(
                          pszValueName,
                          &pszValueNameEsc,
                          &dwValueNameEscLen);
            BAIL_ON_REG_ERROR(dwError);
            sts = write(wfd, "\"", 1);
            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
            sts = write(wfd, pszValueNameEsc, dwValueNameEscLen);
            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
            sts = write(wfd, "\" = {\n", 6);
            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));
             
      
            /* Deal with an override value first */
            if (Value->Data && Value->DataLen)
            {
                LWREG_SAFE_FREE_STRING(pszDumpString);
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "value",
                              valueType,
                              Value->Data,
                              Value->DataLen,
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);

                sts = write(wfd, "\t", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, pszDumpString, strlen(pszDumpString));
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, "\n", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
            }

            /* Deal with default values now */
            Attr = &Value->Attributes;
            if (Attr->pDefaultValue && Attr->DefaultValueLen)
            {
                LWREG_SAFE_FREE_STRING(pszDumpString);
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "default",
                              valueType,
                              Attr->pDefaultValue,
                              Attr->DefaultValueLen,
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);
    
                sts = write(wfd, "\t", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, pszDumpString, strlen(pszDumpString));
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, "\n", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
            }
 
            if (Attr->pwszDocString &&
                LwRtlWC16StringNumChars(Attr->pwszDocString))
            {
                LWREG_SAFE_FREE_STRING(pszDumpString);
                dwError = RegExportEntry(
                              NULL,
                              "", // PCSTR pszSddlCString
                              REG_SZ, // valueName type
                              "doc",
                              REG_WSZ,
                              Attr->pwszDocString,
                              LwRtlWC16StringNumChars(Attr->pwszDocString),
                              &pszDumpString,
                              &dwDumpStringLen);
                /* Map to NT error status ? */
                BAIL_ON_NT_STATUS(dwError);
    
                sts = write(wfd, "\t", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, pszDumpString, strlen(pszDumpString));
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
                sts = write(wfd, "\n", 1);
                BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                    sts == -1 ? errno : 0));
            }

            switch (Attr->RangeType)
            {
                case LWREG_VALUE_RANGE_TYPE_BOOLEAN:
                    sts = write(wfd, "\t\"range\"=boolean\n", 17);
                    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                        sts == -1 ? errno : 0));
                    break;

                case LWREG_VALUE_RANGE_TYPE_ENUM:
                    sts = write(wfd, "\trange=string:", 14);
                    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                        sts == -1 ? errno : 0));
                    for (enumIndex=0; 
                         Attr->Range.ppwszRangeEnumStrings[enumIndex];
                         enumIndex++)
                    {
                        LWREG_SAFE_FREE_MEMORY(pszEnumValue);
                        LwRtlCStringAllocateFromWC16String(
                             &pszEnumValue,
                             Attr->Range.ppwszRangeEnumStrings[enumIndex]);
                        if (enumIndex)
                        {
                            sts = write(wfd, "\t\t\"", 3);
                            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                                sts == -1 ? errno : 0));
                        }
                        else
                        {
                            sts = write(wfd, "\"", 1);
                            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                                sts == -1 ? errno : 0));
                        } 
                        sts = write(wfd, pszEnumValue, strlen(pszEnumValue));
                        BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                            sts == -1 ? errno : 0));
                        sts = write(wfd, "\"", 1);
                        BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                            sts == -1 ? errno : 0));

                        if (Attr->Range.ppwszRangeEnumStrings[enumIndex+1])
                        {
                            sts = write(wfd, " \\\n", 3);
                            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                                sts == -1 ? errno : 0));
                        }
                    }
                    sts = write(wfd, "\n", 1);
                    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                        sts == -1 ? errno : 0));
                    break;

                case LWREG_VALUE_RANGE_TYPE_INTEGER:
                    sprintf(fmtbuf, "\t\"range\" = integer:%d-%d\n",
                        Attr->Range.RangeInteger.Min,
                        Attr->Range.RangeInteger.Max);
                    sts = write(wfd, fmtbuf, strlen(fmtbuf)); 
                    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                        sts == -1 ? errno : 0));
                break;

                default:
                    break;
            }

            LWREG_SAFE_FREE_STRING(pszDumpString);
            sts = write(wfd, "}\n", 2);
            BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(
                sts == -1 ? errno : 0));
        }
    }
    sts = write(wfd, "\n", 1);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError(sts == -1 ? errno : 0));

cleanup:
    LWREG_SAFE_FREE_STRING(pszStringSecurityDescriptor);
    LWREG_SAFE_FREE_STRING(pszDumpString);
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszValueNameEsc);
    LWREG_SAFE_FREE_MEMORY(pszEnumValue);
    return NULL;

error:
    goto cleanup;
}


VOID
MemDbExportEntryChanged(
    VOID)
{
    pthread_mutex_lock(&MemRegRoot()->ExportMutex);
    MemRegRoot()->valueChangeCount++;
    pthread_mutex_unlock(&MemRegRoot()->ExportMutex);
    pthread_cond_signal(&MemRegRoot()->ExportCond);
}


VOID
MemDbStopExportToFileThread(
    VOID)
{
    if (!MemRegRoot() || !MemRegRoot()->ExportCtx)
    {
        return;
    }
    pthread_mutex_lock(&MemRegRoot()->ExportMutexStop);
    MemRegRoot()->ExportCtx->bStopThread = TRUE;
    pthread_cond_signal(&MemRegRoot()->ExportCond);
    pthread_mutex_unlock(&MemRegRoot()->ExportMutexStop);

    pthread_join(MemRegRoot()->hThread, NULL);
}


/*
 * Export thread is a big state machine. This is the order of states:
 * MEMDB_EXPORT_START = 1,
 * MEMDB_EXPORT_CHECK_CHANGES,
 * MEMDB_EXPORT_INIT_TO_INFINITE,
 * MEMDB_EXPORT_INIT_TO_SHORT,
 * MEMDB_EXPORT_WAIT,
 * MEMDB_EXPORT_TEST_CHANGE,
 * MEMDB_EXPORT_TEST_MAX_TIMEOUT,
 * MEMDB_EXPORT_UPDATE_SHORT_TIMEOUT,
 * MEMDB_EXPORT_WRITE_CHANGES,
 *
 * State machine below removes MEMDB_EXPORT_ prefix for states.
 * _TO_ is abbreviation for Time Out
 * (state change) denotes a state transition
 *
 *   +-----<--------------------<------------------------------<-+
 *   |                                                           |
 *   |                                                           |
 *   v                                                           |
 * START -> CHECK_CHANGES -> INIT_TO_INFINITE                    |
 *                |                 |                            |
 *                |                 |                            |
 *           (change pending)       |                            |
 *                |                 |                            |
 *                |                 |                            |
 *                v             (T.O. 30days)                    |
 *           INIT_TO_SHORT          |                            |
 *             ^    |               v                            ^
 *             |    +-(5sec)-> EXPORT_WAIT ----(TIMEOUT)--> WRITE_CHANGES
 *             |                    |   ^                        ^
 *             |               (change) |                        |
 *             |                    |   +-----------+            |
 *             |                    v               |            |
 *             +----(changed)----TEST_CHANGE        |            |
 *                                  |               |            |
 *                                  |           (T.O. 5sec)      |
 *                            (max timeout?)        |            |
 *                                  |               |            |
 *                                  |   UPDATE_SHORT_TIMEOUT     |
 *                                  |      ^                     |
 *                                  |      |                     |
 *                                  |    (no)                    |
 *                                  v      |                     |
 *                             TEST_MAX_TIMEOUT >----(yes)-------+
 */
 
PVOID
MemDbExportToFileThread(
    PVOID ctx)
{
    NTSTATUS status = 0;
    MEMDB_EXPORT_STATE state = MEMDB_EXPORT_START;
    PMEMDB_FILE_EXPORT_CTX exportCtx = (PMEMDB_FILE_EXPORT_CTX) ctx;
    struct timespec timeOutShort = {0};
    struct timespec timeOutMax = {0};
    struct timespec *pTimeOutMax = NULL;
    BOOLEAN bTimeOutInfinite = FALSE;
    DWORD changeCountInit = 0;
    int sts = 0;

    REG_LOG_INFO("MemDbExportToFileThread: Thread started.");
    do
    {
        switch (state)
        {
            case MEMDB_EXPORT_START:
                pthread_mutex_lock(&MemRegRoot()->ExportMutex);
                changeCountInit = MemRegRoot()->valueChangeCount;
                state = MEMDB_EXPORT_CHECK_CHANGES;
                break;
       
            case MEMDB_EXPORT_CHECK_CHANGES:
                if (changeCountInit == 0)
                {
                    state = MEMDB_EXPORT_INIT_TO_INFINITE;
                }
                else
                {
                    state = MEMDB_EXPORT_INIT_TO_SHORT;
                }
                break;

            case MEMDB_EXPORT_INIT_TO_INFINITE:
                /* No changes pending, so wait "forever", 30 days */
                timeOutShort.tv_sec = time(NULL);
                timeOutShort.tv_nsec = 0;
                timeOutShort.tv_sec += MEMDB_FOREVER_EXPORT_TIMEOUT; // 1month
                bTimeOutInfinite = TRUE;
                state = MEMDB_EXPORT_WAIT;
                break;

            case MEMDB_EXPORT_INIT_TO_SHORT:
                timeOutShort.tv_sec = time(NULL);
                timeOutShort.tv_nsec = 0;
                timeOutShort.tv_sec += MEMDB_CHANGED_EXPORT_TIMEOUT; // 5s
                if (!pTimeOutMax)
                {
                    timeOutMax = timeOutShort;
                    timeOutMax.tv_sec += MEMDB_MAX_EXPORT_TIMEOUT; // 10m
                    pTimeOutMax = &timeOutMax;
                }
                bTimeOutInfinite = FALSE;
                state = MEMDB_EXPORT_WAIT;
                break;

            case MEMDB_EXPORT_WAIT:
                sts = pthread_cond_timedwait(
                          &MemRegRoot()->ExportCond, 
                          &MemRegRoot()->ExportMutex,
                          &timeOutShort);
                if (exportCtx->bStopThread)
                {
                    pthread_mutex_unlock(&MemRegRoot()->ExportMutex);
                    break;
                }
                else if (sts == ETIMEDOUT)
                {
                    changeCountInit = MemRegRoot()->valueChangeCount;
                    state = MEMDB_EXPORT_WRITE_CHANGES;
                }
                else if (changeCountInit > 0 &&
                         changeCountInit == MemRegRoot()->valueChangeCount)
                {
                    /* False wakeup? */
                    state = MEMDB_EXPORT_WAIT;
                }
                else 
                {
                    state = MEMDB_EXPORT_TEST_CHANGE;
                }
                pthread_mutex_unlock(&MemRegRoot()->ExportMutex);
                break;

            case MEMDB_EXPORT_TEST_CHANGE:
                if (bTimeOutInfinite)
                {
                    state = MEMDB_EXPORT_INIT_TO_SHORT;
                }
                else
                {
                    state = MEMDB_EXPORT_TEST_MAX_TIMEOUT;
                }
                break;

            case MEMDB_EXPORT_TEST_MAX_TIMEOUT:
                if (pTimeOutMax && timeOutShort.tv_sec > pTimeOutMax->tv_sec)
                {
                    REG_LOG_DEBUG("MemDbExportToFileThread: Forced timeout "
                                  "expired, Exporting registry to save file");
                    pTimeOutMax = NULL;
                    state = MEMDB_EXPORT_WRITE_CHANGES;
                }
                else
                {
                    state = MEMDB_EXPORT_UPDATE_SHORT_TIMEOUT;
                }
                break;

            case MEMDB_EXPORT_UPDATE_SHORT_TIMEOUT:
                timeOutShort.tv_sec = time(NULL);
                timeOutShort.tv_nsec = 0;
                timeOutShort.tv_sec += MEMDB_CHANGED_EXPORT_TIMEOUT; //5s
                state = MEMDB_EXPORT_WAIT;
                pthread_mutex_lock(&MemRegRoot()->ExportMutex);
                break;

            case MEMDB_EXPORT_WRITE_CHANGES:
                pthread_mutex_lock(&MemRegRoot()->ExportMutex);
                pthread_rwlock_rdlock(&MemRegRoot()->lock);
                status = 0;
                if (changeCountInit > 0)
                {
                    REG_LOG_DEBUG("MemDbExportToFileThread: "
                                  "Exporting registry to save file...");
                    status = MemDbExportToFile(exportCtx);
                    REG_LOG_ERROR("MemDbExportToFileThread: "
                                  "Exporting registry to save file completed.");
                }
                pthread_rwlock_unlock(&MemRegRoot()->lock);
                if (status)
                {
                    REG_LOG_DEBUG("Failed exporting registry to %s",
                                  MEMDB_EXPORT_FILE);
                }

                changeCountInit = 0;
                MemRegRoot()->valueChangeCount = 0;
                pthread_mutex_unlock(&MemRegRoot()->ExportMutex);
            
                state = MEMDB_EXPORT_START;
                break;
        }
    } while (!exportCtx->bStopThread);

    REG_LOG_INFO("MemDbExportToFileThread: Thread is terminating!!!");
    pthread_mutex_lock(&MemRegRoot()->ExportMutexStop);
    exportCtx->bStopThread = FALSE;
    pthread_mutex_unlock(&MemRegRoot()->ExportMutexStop);
    pthread_cond_signal(&MemRegRoot()->ExportCondStop);

    /* This is the return type for a function running as a thread */
    return NULL;
}


NTSTATUS
MemDbExportToFile(
    PMEMDB_FILE_EXPORT_CTX pExportCtx)
{
    NTSTATUS status = 0;
    REG_DB_CONNECTION regDbConn = {0};
    int wfd = -1;
    int dfd = -1;

    dfd = open(MEMDB_EXPORT_DIR, O_RDONLY);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((dfd == -1) ? errno : 0));

    wfd = open(MEMDB_EXPORT_FILE ".tmp", O_RDWR | O_CREAT | O_TRUNC, 0600);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((wfd == -1) ? errno : 0));
    pExportCtx->wfd = wfd;

    regDbConn.ExportCtx = pExportCtx;
    regDbConn.pMemReg = pExportCtx->hNode;
    status = MemDbRecurseRegistry(
                 NULL,
                 &regDbConn,
                 NULL,
                 pfMemRegExportToFile,
                 pExportCtx);
    BAIL_ON_NT_STATUS(status);

    status = fsync(wfd);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((status == -1) ? errno : 0));

    status = close(wfd);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((status == -1) ? errno : 0));
    wfd = -1;

    status = rename(MEMDB_EXPORT_FILE ".tmp", MEMDB_EXPORT_FILE);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((status == -1) ? errno : 0));

    status = fsync(dfd);
    BAIL_ON_REG_ERROR(RegMapErrnoToLwRegError((status == -1) ? errno : 0));

cleanup:
    if (dfd != -1)
    {
        close(dfd);
    }
    
    return status;

error:
    if (wfd != -1)
    {
        close(wfd);
    }
    goto cleanup;
}


NTSTATUS
MemDbStartExportToFileThread(VOID)
{
    NTSTATUS status = 0;
    PMEMDB_FILE_EXPORT_CTX exportCtx = {0};
    PWSTR pwszRootKey = NULL;

    status = LW_RTL_ALLOCATE(
                 (PVOID*) &exportCtx,
                 PMEMDB_FILE_EXPORT_CTX,
                 sizeof(MEMDB_FILE_EXPORT_CTX));
    BAIL_ON_NT_STATUS(status);

    exportCtx->hNode = MemRegRoot()->pMemReg;

    MemRegRoot()->ExportCtx = exportCtx;
    status = pthread_create(&MemRegRoot()->hThread, 
                            NULL, 
                            MemDbExportToFileThread, 
                            (PVOID) exportCtx);
    status = RegMapErrnoToLwRegError(status);
    BAIL_ON_REG_ERROR(status);

cleanup:
    if (status)
    {
        LWREG_SAFE_FREE_MEMORY(exportCtx);
    }
    LWREG_SAFE_FREE_MEMORY(pwszRootKey);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(exportCtx);
    goto cleanup;
}


DWORD
pfImportFile(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hSubKey = NULL;
    REG_DB_CONNECTION regDbConn = {0};
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;
    PWSTR pwszStringData = NULL;
    PMEMDB_IMPORT_FILE_CTX pImportCtx = (PMEMDB_IMPORT_FILE_CTX) userContext;
    PVOID pData = NULL;
    DWORD dwDataLen = 0;
    DWORD dwLineNum = 0;
    PMEMREG_VALUE pRegValue = NULL;
    DWORD dataType = 0;
    LWREG_VALUE_ATTRIBUTES tmpAttr = {0};
    PMEMREG_NODE_SD pNodeSd = NULL;

#ifdef __MEMDB_PRINTF__ 
FILE *dbgfp = fopen("/tmp/lwregd-import.txt", "a");
#endif

    if (pItem->status == LWREG_ERROR_INVALID_CONTEXT)
    {
        RegParseGetLineNumber(pImportCtx->parseHandle, &dwLineNum);
        REG_LOG_ERROR(
            "WARNING: Inconsistent data/type/range found importing "
            "from file %s: line=%d [%s] -> %s",
            pImportCtx->fileName,
            dwLineNum,
            pItem->keyName,
            pItem->valueName);
    }

    if (pItem->type == REG_KEY)
    {
        regDbConn.pMemReg = MemRegRoot()->pMemReg;

        // Open subkeys that exist
        status = LwRtlWC16StringAllocateFromCString(
                     &pwszSubKey,
                     pItem->keyName);
        BAIL_ON_NT_STATUS(status);

        status = MemDbOpenKey(
                      NULL,
                      &regDbConn,
                      pwszSubKey,
                      KEY_ALL_ACCESS,
                      &hSubKey);
        if (status == 0)
        {
            regDbConn.pMemReg = hSubKey;
            pImportCtx->hSubKey = hSubKey;
        }

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            status = MemDbCreateKeyEx(
                         NULL,
                         &regDbConn,
                         pwszSubKey,
                         0, // IN DWORD dwReserved,
                         NULL,  // IN OPTIONAL PWSTR pClass,
                         0, // IN DWORD dwOptions,
                         0, // IN ACCESS_MASK 
                         NULL, // IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE 
                         0, // IN ULONG ulSecDescLength,
                         &hSubKey,
                         NULL); // OUT OPTIONAL PDWORD pdwDisposition
            BAIL_ON_NT_STATUS(status);
            pImportCtx->hSubKey = hSubKey;
        }
        else
        {
            BAIL_ON_NT_STATUS(status);
        }

#ifdef __MEMDB_PRINTF__ /* Debugging only */
fprintf(dbgfp, "pfImportFile: type=%d valueName=%s\n",
       pItem->type,
       pItem->keyName);
#endif
    }
    else if (pItem->valueType == REG_KEY_DEFAULT &&
             !strcmp(pItem->valueName, "@security"))
    {
        hSubKey = pImportCtx->hSubKey;
        status = MemRegStoreCreateNodeSdFromSddl(
                     (PSTR) pItem->value,
                     pItem->valueLen,
                     &pNodeSd);
        BAIL_ON_NT_STATUS(status);

        /* 
         * Replace SD created during registry bootstrap with the value
         * imported from the save file. The only trick here is to replace
         * the SD, but not its container, which everyone is pointing at.
         */
        if (hSubKey->pNodeSd->SecurityDescriptorAllocated)
        {
            LWREG_SAFE_FREE_MEMORY(hSubKey->pNodeSd->SecurityDescriptor);
        }
        hSubKey->pNodeSd->SecurityDescriptor =
            pNodeSd->SecurityDescriptor;
        hSubKey->pNodeSd->SecurityDescriptorLen =
            pNodeSd->SecurityDescriptorLen;
        hSubKey->pNodeSd->SecurityDescriptorAllocated = TRUE;
        LWREG_SAFE_FREE_MEMORY(pNodeSd);
    }
    else 
    {
        status = LwRtlWC16StringAllocateFromCString(
                     &pwszValueName,
                     pItem->valueName);
        BAIL_ON_NT_STATUS(status);
        if (pItem->type == REG_SZ || pItem->type == REG_DWORD ||
            pItem->type == REG_BINARY || pItem->type == REG_MULTI_SZ)
        {
    
            if (pItem->type == REG_SZ)
            {
                status = LwRtlWC16StringAllocateFromCString(
                             &pwszStringData,
                             pItem->value);
                BAIL_ON_NT_STATUS(status);
                pData = pwszStringData;
                dwDataLen = wc16slen(pwszStringData) * 2 + 2;
                dataType = REG_SZ;
            }
            else
            {
                pData = pItem->value,
                dwDataLen = pItem->valueLen;
                dataType = pItem->type;
            }
            status = MemRegStoreAddNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         0, // Not used?
                         dataType,
                         pData,
                         dwDataLen);
            BAIL_ON_NT_STATUS(status);
        }
        else if (pItem->type == REG_ATTRIBUTES)
        {
            dataType = pItem->regAttr.ValueType;
            pData = NULL;
            dwDataLen = 0;
            if (pItem->value && pItem->valueLen)
            {
                pData = pItem->value;
                dwDataLen = pItem->valueLen;

                if (dataType == REG_SZ)
                {
                    LWREG_SAFE_FREE_MEMORY(pwszStringData);
                    status = LwRtlWC16StringAllocateFromCString(
                                     &pwszStringData,
                                     pData);
                    BAIL_ON_NT_STATUS(status);
                    dwDataLen = wc16slen(pwszStringData) * 2 + 2;
                    pData = pwszStringData;
                }
            }

            status = MemRegStoreAddNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         0, // Not used?
                         dataType,
                         pData, 
                         dwDataLen);
            BAIL_ON_NT_STATUS(status);
            status = MemRegStoreFindNodeValue(
                         pImportCtx->hSubKey,
                         pwszValueName,
                         &pRegValue);
            BAIL_ON_NT_STATUS(status);
            tmpAttr = pItem->regAttr;
            if (tmpAttr.ValueType == REG_SZ)
            {
                LWREG_SAFE_FREE_MEMORY(pwszStringData);
                status = LwRtlWC16StringAllocateFromCString(
                                     &pwszStringData,
                                 pItem->regAttr.pDefaultValue);
                BAIL_ON_NT_STATUS(status);
                tmpAttr.pDefaultValue = (PVOID) pwszStringData;
                tmpAttr.DefaultValueLen = 
                    wc16slen(tmpAttr.pDefaultValue) * 2 + 2;
            }
            status = MemRegStoreAddNodeAttribute(
                         pRegValue,
                         &tmpAttr);
            BAIL_ON_NT_STATUS(status);
        }
        LWREG_SAFE_FREE_MEMORY(pwszValueName);
    }

#ifdef __MEMDB_PRINTF__  /* Debug printf output */
char *subKey = NULL;
LwRtlCStringAllocateFromWC16String(&subKey, pImportCtx->hSubKey->Name);
        fprintf(dbgfp, "pfImportFile: type=%d subkey=[%s] valueName=%s\n",
                pItem->type,
                subKey,
                pItem->valueName ? pItem->valueName : "");
LWREG_SAFE_FREE_STRING(subKey);
fclose(dbgfp);
#endif

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszValueName);
    LWREG_SAFE_FREE_MEMORY(pwszStringData);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return status;
error:
    goto cleanup;
}

NTSTATUS
MemDbImportFromFile(
    PSTR pszImportFile,
    PFN_REG_CALLBACK pfCallback,
    PMEMDB_IMPORT_FILE_CTX userContext)
{
    DWORD dwError = 0;
    DWORD dwLineNum = 0;
    HANDLE parseH = NULL;

    if (access(pszImportFile, R_OK) == -1)
    {
        return 0;
    }
    dwError = RegParseOpen(
                  pszImportFile,
                  pfCallback,
                  userContext,
                  &parseH);
    BAIL_ON_REG_ERROR(dwError);

    userContext->parseHandle = parseH;
    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);


cleanup:
    RegParseClose(parseH);
    return dwError;

error:
    if (dwError == LWREG_ERROR_PARSE || dwError == LWREG_ERROR_SYNTAX)
    {
        RegParseGetLineNumber(parseH, &dwLineNum);
        REG_LOG_ERROR("Error parsing file %s: line=%d",
                      pszImportFile, dwLineNum);
    }
    goto cleanup;
}



NTSTATUS
MemDbClose(
    IN PREG_DB_CONNECTION hDb)
{
    NTSTATUS status = 0;

    if (!hDb || !hDb->pMemReg)
    {
        goto cleanup;
    }
    status = MemDbRecurseDepthFirstRegistry(
                 NULL,
                 hDb,
                 NULL,
                 pfDeleteNodeCallback,
                 NULL);
    BAIL_ON_NT_STATUS(status);

    MemDbStopExportToFileThread();

    MemRegStoreClose(hDb->pMemReg);
cleanup:
    return status;

error:
    goto cleanup;
}


PWSTR
pwstr_wcschr(
    PWSTR pwszHaystack, WCHAR wcNeedle)
{
    DWORD i = 0;
    for (i=0; pwszHaystack[i] != '\0'; i++)
    {
        if (pwszHaystack[i] == wcNeedle)
        {
            return &pwszHaystack[i];
        }
    }
    return NULL;
}

NTSTATUS
MemDbOpenKey(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN PCWSTR pwszFullKeyPath,
    IN ACCESS_MASK AccessDesired,
    OUT OPTIONAL PMEMREG_NODE *pRegKey)
{
    NTSTATUS status = 0;
    PWSTR pwszPtr = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszTmpFullPath = NULL;

    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    BOOLEAN bEndOfString = FALSE;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    ACCESS_MASK AccessGranted = 0;
     
    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pwszFullKeyPath);
    BAIL_ON_NT_STATUS(status);

    if (!hDb)
    {
        hParentKey = MemRegRoot()->pMemReg;
    }
    else
    {
        hParentKey = hDb->pMemReg;
    }
    pwszSubKey = pwszTmpFullPath;
    do 
    {
        pwszPtr = pwstr_wcschr(pwszSubKey, L'\\');
        if (pwszPtr)
        {
            *pwszPtr++ = L'\0';
        }
        else
        {
            pwszPtr = pwszSubKey;
            bEndOfString = TRUE;
        }
        /*
         * Iterate over subkeys in \ sepearated path.
         */
        status = MemRegStoreFindNode(
                     hParentKey,
                     pwszSubKey,
                     &hSubKey);
        hParentKey = hSubKey;
        pwszSubKey = pwszPtr;
    } while (status == 0 && !bEndOfString);
    BAIL_ON_NT_STATUS(status);

    if (pServerState && hSubKey->pNodeSd)
    {
        status = RegSrvAccessCheckKey(
                     pServerState->pToken,
                     hSubKey->pNodeSd->SecurityDescriptor,
                     hSubKey->pNodeSd->SecurityDescriptorLen,
                     AccessDesired,
                     &AccessGranted);
        BAIL_ON_NT_STATUS(status);
    }
    hParentKey->NodeRefCount++;
    *pRegKey = hParentKey;

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszTmpFullPath);
    return status;

error:
    goto cleanup;
}


VOID
MemDbCloseKey(
    IN HKEY hKey)
{
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;

    if (hKey)
    {
        if (pKeyHandle->pKey->hNode->NodeRefCount >= 1)
        {
            pKeyHandle->pKey->hNode->NodeRefCount--;
        }
        LWREG_SAFE_FREE_MEMORY(pKeyHandle->pKey);
        LWREG_SAFE_FREE_MEMORY(pKeyHandle);
    }
}


NTSTATUS
MemDbAccessCheckKey(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength)
{
    NTSTATUS status = 0;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor = NULL;
    DWORD SecurityDescriptorLen = 0;
    ACCESS_MASK AccessGranted = 0;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;

    if (pSecDescRel)
    {
        SecurityDescriptor = pSecDescRel;
        SecurityDescriptorLen = ulSecDescLength;
    }
    else
    {
        if (hDb->pMemReg && hDb->pMemReg->pNodeSd)
        {
            SecurityDescriptor =
                hDb->pMemReg->pNodeSd->SecurityDescriptor;
            SecurityDescriptorLen =
                hDb->pMemReg->pNodeSd->SecurityDescriptorLen;
        }
    }

    if (pServerState && pServerState->pToken &&
        SecurityDescriptor && SecurityDescriptorLen>0)
    {
        status = RegSrvAccessCheckKey(pServerState->pToken,
                                      SecurityDescriptor,
                                      SecurityDescriptorLen,
                                      AccessDesired,
                                      &AccessGranted);
        if (STATUS_NO_TOKEN == status)
        {
            status = 0;
            AccessGranted = 0;
        }
        BAIL_ON_NT_STATUS(status);
    }
cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbCreateKeyEx(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN PCWSTR pcwszSubKey,
    IN DWORD dwReserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PMEMREG_NODE *pphSubKey,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    NTSTATUS status = 0;
    DWORD dwDisposition = 0;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    PWSTR pwszTmpFullPath = NULL;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszPtr = NULL;
    BOOLEAN bEndOfString = FALSE;
    
    status = MemDbAccessCheckKey(
                 Handle,
                 hDb,
                 AccessDesired,
                 pSecDescRel,
                 ulSecDescLength);
    BAIL_ON_NT_STATUS(status);

    /*
     * Iterate over subkeys in \ sepearated path.
     */
    status = LwRtlWC16StringDuplicate(&pwszTmpFullPath, pcwszSubKey);
    BAIL_ON_NT_STATUS(status);

    pwszSubKey = pwszTmpFullPath;
    hParentKey = hDb->pMemReg;
    do 
    {
        pwszPtr = pwstr_wcschr(pwszSubKey, L'\\');
        if (pwszPtr)
        {
            *pwszPtr++ = L'\0';
        }
        else
        {
            pwszPtr = pwszSubKey;
            bEndOfString = TRUE;
        }

        /*
         * Iterate over subkeys in \ sepearated path.
         */
        status = MemRegStoreFindNode(
                     hParentKey,
                     pwszSubKey,
                     &hSubKey);
        if (status == 0)
        {
            hParentKey = hSubKey;
            *pphSubKey = hParentKey;
        }
        pwszSubKey = pwszPtr;

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* New node for current subkey, add it */
            status = MemRegStoreAddNode(
                             hParentKey,
                             pwszSubKey,
                             MEMREG_TYPE_KEY,
                             pSecDescRel,  // SD parameter
                             ulSecDescLength,
                             NULL,
                             &hParentKey);
            BAIL_ON_NT_STATUS(status);
            dwDisposition = REG_CREATED_NEW_KEY;
            *pphSubKey = hParentKey;
        }
        else if (bEndOfString)
        {
            dwDisposition = REG_OPENED_EXISTING_KEY;
            *pphSubKey = hParentKey;
        }
    } while (status == 0 && !bEndOfString);

    if (pdwDisposition)
    {
        *pdwDisposition = dwDisposition;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszTmpFullPath);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbQueryInfoKey(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    /*
     * A pointer to a buffer that receives the user-defined class of the key. 
     * This parameter can be NULL.
     */
    OUT OPTIONAL PWSTR pClass, 
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pdwReserved, /* This parameter is reserved and must be NULL. */
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen, /* implement this later */
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime /* implement this later */
    )
{
    PMEMREG_NODE hKeyNode = NULL;
    NTSTATUS status = 0;
    DWORD keyLen = 0;
    DWORD valueLen = 0;
    DWORD valueNameLen = 0;
    DWORD maxKeyLen = 0;
    DWORD maxValueNameLen = 0;
    DWORD maxValueLen = 0;
    DWORD indx = 0;

    BAIL_ON_NT_STATUS(status);
    
    /*
     * Query info about keys
     */

    hKeyNode = hDb->pMemReg;
    if (pcSubKeys)
    {
        *pcSubKeys = hKeyNode->NodesLen;
    }

    if (pcMaxSubKeyLen)
    {
        for (indx=0, keyLen=0, maxKeyLen; indx < hKeyNode->NodesLen; indx++)
        {
            keyLen = RtlWC16StringNumChars(hKeyNode->SubNodes[indx]->Name);
            if (keyLen > maxKeyLen)
            {
                maxKeyLen = keyLen;
            }
        
        }
        *pcMaxSubKeyLen = maxKeyLen;
    }

    /*
     * Query info about values
     */
    if (pcValues)
    {
        *pcValues = hKeyNode->ValuesLen;
    }

    if (pcMaxValueNameLen)
    {
        for (indx=0, valueNameLen=0, maxValueLen=0; indx < hKeyNode->ValuesLen; indx++)
        {
            valueNameLen = RtlWC16StringNumChars(hKeyNode->Values[indx]->Name);
            if (valueNameLen > maxValueNameLen)
            {
                maxValueNameLen = valueNameLen;
            }
        
        }
        *pcMaxValueNameLen = maxValueNameLen;
    }

    if (pcMaxValueLen)
    {
        for (indx=0, valueLen=0, maxValueLen=0; indx < hKeyNode->ValuesLen; indx++)
        {
            if (hKeyNode->Values[indx]->DataLen > 
                hKeyNode->Values[indx]->Attributes.DefaultValueLen)
            {
                valueLen = hKeyNode->Values[indx]->DataLen;
            }
            else
            {
                valueLen = hKeyNode->Values[indx]->Attributes.DefaultValueLen;
            }

            if (valueLen > maxValueLen)
            {
                maxValueLen = valueLen;
            }
        }
        *pcMaxValueLen = maxValueLen;
    }

    if (pcbSecurityDescriptor)
    {
        *pcbSecurityDescriptor = hKeyNode->pNodeSd->SecurityDescriptorLen;
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbEnumKeyEx(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN DWORD dwIndex,
    OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pdwReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    )
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    DWORD keyLen = 0;


    hKeyNode = hDb->pMemReg;
    if (dwIndex >= hKeyNode->NodesLen)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    /*
     * Query info about keys
     */
    keyLen = RtlWC16StringNumChars(hKeyNode->SubNodes[dwIndex]->Name);
    if (keyLen > *pcName)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pName, hKeyNode->SubNodes[dwIndex]->Name, keyLen * sizeof(WCHAR));
    *pcName = keyLen;

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbSetValueEx(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwReserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    PMEMREG_VALUE pRegValue = NULL;
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)Handle;
    ACCESS_MASK AccessGranted = 0;

    BAIL_ON_NT_STATUS(status);

    hKeyNode = hDb->pMemReg;
    if (hKeyNode->pNodeSd)
    {
        status = RegSrvAccessCheckKey(
                     pServerState->pToken,
                     hKeyNode->pNodeSd->SecurityDescriptor,
                     hKeyNode->pNodeSd->SecurityDescriptorLen,
                     KEY_WRITE,
                     &AccessGranted);
        BAIL_ON_NT_STATUS(status);
    }
    status = MemRegStoreFindNodeValue(
                 hKeyNode,
                 pValueName,
                 &pRegValue);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {

        status = MemRegStoreAddNodeValue(
                     hKeyNode,
                     pValueName,
                     dwReserved, // Not used?
                     dwType,
                     pData,
                     cbData);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        /* Modify existing node value */
        status = MemRegStoreChangeNodeValue(
                     pRegValue,
                     pData,
                     cbData);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetValue(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    PMEMREG_VALUE hValue = NULL;

    hKeyNode = hDb->pMemReg;

    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKeyNode;
        status = MemRegStoreFindNodeSubkey(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKeyNode = hSubKey;
    }


    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKeyNode,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    /*
     * Return data from value node. Storage for return values is
     * passed into this function by the caller.
     */
    *pdwType = hValue->Type;
    if (pcbData)
    {
        if (hValue->DataLen)
        {
            *pcbData = hValue->DataLen;
        }
        else if (hValue->Attributes.DefaultValueLen)
        {
            *pcbData = hValue->Attributes.DefaultValueLen;
        }
          
    }
    if (pData && pcbData)
    {
        if (hValue->Data && hValue->DataLen)
        {
            memcpy(pData, hValue->Data, hValue->DataLen);
        }
        else if (hValue->Attributes.pDefaultValue)
        {
            memcpy(pData,
                   hValue->Attributes.pDefaultValue,
                   hValue->Attributes.DefaultValueLen);
        }
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbEnumValue(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pdwReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    DWORD valueLen = 0;

    hKeyNode = hDb->pMemReg;
    if (dwIndex >= hKeyNode->ValuesLen)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }
    
    /*
     * Query info about indexed value
     */
    valueLen = RtlWC16StringNumChars(hKeyNode->Values[dwIndex]->Name);
    if (valueLen > *pcchValueName)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }


    memcpy(pValueName, hKeyNode->Values[dwIndex]->Name, valueLen * sizeof(WCHAR));
    *pcchValueName = valueLen;
    if (pType)
    {
        *pType = hKeyNode->Values[dwIndex]->Type;
    }
    if (pcbData)
    {
        if (hKeyNode->Values[dwIndex]->Data && hKeyNode->Values[dwIndex]->DataLen)
        {
            *pcbData = hKeyNode->Values[dwIndex]->DataLen;
            if (pData && hKeyNode->Values[dwIndex]->Data)
            {
                memcpy(pData, 
                       hKeyNode->Values[dwIndex]->Data,
                       hKeyNode->Values[dwIndex]->DataLen);
            }
        }
        else if (hKeyNode->Values[dwIndex]->Attributes.pDefaultValue &&
                 hKeyNode->Values[dwIndex]->Attributes.DefaultValueLen > 0)
        {
            *pcbData = hKeyNode->Values[dwIndex]->Attributes.DefaultValueLen;
            if (pData && hKeyNode->Values[dwIndex]->Attributes.pDefaultValue)
            {
                memcpy(pData, 
                       hKeyNode->Values[dwIndex]->Attributes.pDefaultValue,
                       hKeyNode->Values[dwIndex]->Attributes.DefaultValueLen);
            }
        }
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetKeyAcl(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    OUT PULONG pSecDescLen)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;

    BAIL_ON_NT_INVALID_POINTER(hDb);
    hKeyNode = hDb->pMemReg;

    if (hKeyNode->pNodeSd)
    {
        if (pSecDescLen)
        {
            *pSecDescLen = hKeyNode->pNodeSd->SecurityDescriptorLen;
            if (pSecDescRel)
            {
                memcpy(pSecDescRel,
                       hKeyNode->pNodeSd->SecurityDescriptor,
                       *pSecDescLen);
            }
        }
    }
cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbSetKeyAcl(
    IN HANDLE Handle,
    IN PREG_DB_CONNECTION hDb,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG secDescLen)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor = NULL;
    PMEMREG_NODE_SD pNodeSd = NULL;
    

    BAIL_ON_NT_INVALID_POINTER(hDb);
    if (!pSecDescRel || secDescLen == 0)
    {
        goto cleanup;
    }
    BAIL_ON_NT_INVALID_POINTER(pSecDescRel);

    hKeyNode = hDb->pMemReg;

    if ((hKeyNode->pNodeSd &&
         memcmp(hKeyNode->pNodeSd->SecurityDescriptor,
                pSecDescRel,
                secDescLen) != 0) ||
        !hKeyNode->pNodeSd)
    {
        if (!hKeyNode->pNodeSd)
        {
            status = LW_RTL_ALLOCATE((PVOID*) &pNodeSd,
                                              PMEMREG_NODE_SD,
                                              sizeof(*pNodeSd));
            BAIL_ON_NT_STATUS(status);
            hKeyNode->pNodeSd = pNodeSd;
        }
        else
        {
            if (hKeyNode->pNodeSd->SecurityDescriptorAllocated)
            {
                LWREG_SAFE_FREE_MEMORY(hKeyNode->pNodeSd->SecurityDescriptor);
            }
        }
        status = LW_RTL_ALLOCATE((PVOID*) &SecurityDescriptor, 
                                 BYTE, 
                                 secDescLen);
        BAIL_ON_NT_STATUS(status);

        hKeyNode->pNodeSd->SecurityDescriptor = SecurityDescriptor;
        memcpy(hKeyNode->pNodeSd->SecurityDescriptor, pSecDescRel, secDescLen);

        hKeyNode->pNodeSd->SecurityDescriptorLen = secDescLen;
        hKeyNode->pNodeSd->SecurityDescriptorAllocated = TRUE;
    }

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pNodeSd);
    LWREG_SAFE_FREE_MEMORY(SecurityDescriptor);
    goto cleanup;
}


NTSTATUS
MemDbSetValueAttributes(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    IN PLWREG_VALUE_ATTRIBUTES pValueAttributes)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    PMEMREG_VALUE hValue = NULL;

    hKeyNode = hDb->pMemReg;

    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKeyNode;
        status = MemRegStoreFindNode(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKeyNode = hSubKey;
    }

    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKeyNode,
                 pValueName,
                 &hValue);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        status = MemRegStoreAddNodeValue(
                     hKeyNode,
                     pValueName,
                     0, // Not used?
                     pValueAttributes->ValueType,
                     NULL,
                     0);
        BAIL_ON_NT_STATUS(status);  
    }
    status = MemRegStoreFindNodeValue(
                 hKeyNode,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    /*
     * Add attributes to the specified node.
     */
    status = MemRegStoreAddNodeAttribute(
                 hValue,
                 pValueAttributes);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbGetValueAttributes(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pSubKey,
    IN PCWSTR pValueName,
    OUT OPTIONAL PLWREG_CURRENT_VALUEINFO* ppCurrentValue,
    OUT OPTIONAL PLWREG_VALUE_ATTRIBUTES* ppValueAttributes)
{
    NTSTATUS status = 0;
    PMEMREG_NODE hKeyNode = NULL;
    PMEMREG_NODE hParentKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    PMEMREG_VALUE hValue = NULL;

    hKeyNode = hDb->pMemReg;
    if (pSubKey)
    {
        /*
         * Find named subnode and use that to find the named value
         */
        hParentKey = hKeyNode;
        status = MemRegStoreFindNode(
                     hParentKey,
                     pSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKeyNode = hSubKey;
    }

    /*
     * Find named value within specified node
     */
    status = MemRegStoreFindNodeValue(
                 hKeyNode,
                 pValueName,
                 &hValue);
    BAIL_ON_NT_STATUS(status);

    status = MemRegStoreGetNodeValueAttributes(
                 hValue,
                 ppCurrentValue,
                 ppValueAttributes);
    BAIL_ON_NT_STATUS(status);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbStackInit(
    DWORD dwSize,
    PMEMDB_STACK *retStack)
{
    NTSTATUS status = 0;
    PMEMDB_STACK newStack = NULL;
    PMEMDB_STACK_ENTRY stackData = NULL;

    status = LW_RTL_ALLOCATE((PVOID*) &newStack,
                              PMEMDB_STACK, 
                              sizeof(MEMDB_STACK));
    BAIL_ON_NT_STATUS(status);
    memset(newStack, 0, sizeof(MEMDB_STACK));

    status = LW_RTL_ALLOCATE((PVOID*) &stackData,
                              PMEMDB_STACK_ENTRY,
                              sizeof(MEMDB_STACK_ENTRY) * dwSize);
    BAIL_ON_NT_STATUS(status);
    memset(stackData, 0, sizeof(MEMDB_STACK_ENTRY) * dwSize);

    newStack->stack = stackData;
    newStack->stackSize = dwSize;
    *retStack = newStack;

cleanup:
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(newStack);
    LWREG_SAFE_FREE_MEMORY(stackData);
    goto cleanup;
}


VOID
MemDbStackFinish(
    PMEMDB_STACK hStack)
{
    DWORD index = 0;


    for (index=0; index<hStack->stackSize; index++)
    {
        LWREG_SAFE_FREE_MEMORY(hStack->stack[index].pwszSubKeyPrefix);
    }
    LWREG_SAFE_FREE_MEMORY(hStack->stack);
    LWREG_SAFE_FREE_MEMORY(hStack);
}


NTSTATUS
MemDbStackPush(
    PMEMDB_STACK hStack,
    PMEMREG_NODE node,
    PWSTR pwszPrefix)
{
    NTSTATUS status = 0;
    MEMDB_STACK_ENTRY newNode = {0};
    PWSTR pwszPathPrefix = NULL;

    status = LwRtlWC16StringDuplicate(&pwszPathPrefix, pwszPrefix);
    BAIL_ON_NT_STATUS(status);

    newNode.pNode = node;
    newNode.pwszSubKeyPrefix = pwszPathPrefix;

    if (hStack->stackPtr+1 > hStack->stackSize)
    {
        status = ERROR_STACK_OVERFLOW;
        REG_LOG_ERROR("MemDbStackPush: Stack overflow %d", hStack->stackSize);
        BAIL_ON_NT_STATUS(status);
    }

    hStack->stack[hStack->stackPtr++] = newNode;
    if (hStack->stackPtr > hStack->stackSizeMax)
    {
        hStack->stackSizeMax = hStack->stackPtr;
        REG_LOG_DEBUG("MemDbStackPush: Max stack depth %d", 
            hStack->stackSizeMax);
    }

error:
    return status;
}


NTSTATUS
MemDbStackPop(
    PMEMDB_STACK hStack,
    PMEMREG_NODE *pNode,
    PWSTR *ppwszPrefix)
{
    NTSTATUS status = 0;

    if (hStack->stackPtr == 0)
    {
        status = ERROR_EMPTY;
        BAIL_ON_NT_STATUS(status);
    }

    hStack->stackPtr--;
    *pNode = hStack->stack[hStack->stackPtr].pNode;
    *ppwszPrefix = hStack->stack[hStack->stackPtr].pwszSubKeyPrefix;
    hStack->stack[hStack->stackPtr].pNode = NULL;
    hStack->stack[hStack->stackPtr].pwszSubKeyPrefix = NULL;

error:
    return status;
}


NTSTATUS
MemDbRecurseRegistry(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(PMEMREG_NODE hKeyNode, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix,
                           NTSTATUS *status),
    IN PVOID userContext)
{
    NTSTATUS status = 0;
    NTSTATUS statusCallback = 0;
    PMEMREG_NODE hKeyNode = NULL;
    INT32 index = 0;
    PMEMDB_STACK hStack = 0;
    PWSTR pwszSubKeyPrefix = NULL;
    PWSTR pwszSubKey = NULL;
    PMEMREG_NODE hSubKey = NULL;
    
    status = MemDbStackInit(MEMREG_MAX_SUBNODE_STACK, &hStack);
    BAIL_ON_NT_STATUS(status);
    hKeyNode = hDb->pMemReg;

    if (pwszOptSubKey)
    {
        status = MemRegStoreFindNodeSubkey(
                     hKeyNode,
                     pwszOptSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKeyNode = hSubKey;
    }

    /* Initially populate stack from top level node */
    if (!pwszOptSubKey)
    {
        for (index=hKeyNode->NodesLen-1; index>=0; index--)
        {
            LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
            status = LwRtlWC16StringAllocatePrintf(
                         &pwszSubKeyPrefix,
                         "%ws", hKeyNode->SubNodes[index]->Name);
            BAIL_ON_NT_STATUS(status);
            status = MemDbStackPush(
                         hStack,
                         hKeyNode->SubNodes[index],
                         pwszSubKeyPrefix);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else
    {
        status = LwRtlWC16StringAllocatePrintf(
                     &pwszSubKeyPrefix,
                     "%ws", pwszOptSubKey);
        BAIL_ON_NT_STATUS(status);
        status = MemDbStackPush(
                     hStack,
                     hSubKey,
                     pwszSubKeyPrefix);
        BAIL_ON_NT_STATUS(status);
    }
    LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);

    do
    {
        status = MemDbStackPop(hStack, &hKeyNode, &pwszSubKeyPrefix);
        if (status == 0)
        {
            pfCallback(hKeyNode,
                       (PVOID) userContext,
                       pwszSubKeyPrefix,
                       &statusCallback);
            /* Bail if callback returns an error */
            status = statusCallback;
            BAIL_ON_NT_STATUS(status);
            if (hKeyNode->SubNodes && hKeyNode->NodesLen > 0)
            {
                for (index=hKeyNode->NodesLen-1; index>=0; index--)
                {
                    status = LwRtlWC16StringAllocatePrintf(
                                 &pwszSubKey, 
                                 "%ws\\%ws",
                                 pwszSubKeyPrefix,
                                 hKeyNode->SubNodes[index]->Name);
                    BAIL_ON_NT_STATUS(status);
                    status = MemDbStackPush(
                                 hStack,
                                 hKeyNode->SubNodes[index],
                                 pwszSubKey);
                    BAIL_ON_NT_STATUS(status);
                    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
                }
            }
        }
        LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    } while (status != ERROR_EMPTY);

cleanup:
    if (status == ERROR_EMPTY)
    {
        status = 0;
    }

    MemDbStackFinish(hStack);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemDbRecurseDepthFirstRegistry(
    IN HANDLE hRegConnection,
    IN PREG_DB_CONNECTION hDb,
    IN OPTIONAL PCWSTR pwszOptSubKey,
    IN PVOID (*pfCallback)(PMEMREG_NODE hKeyNode, 
                           PVOID userContext,
                           PWSTR pwszSubKeyPrefix,
                           NTSTATUS *status),
    IN PVOID userContext)
{
    NTSTATUS status = 0;
    NTSTATUS statusCallback = 0;
    PMEMREG_NODE hKeyNode = NULL;
    INT32 index = 0;
    PMEMDB_STACK hStack = 0;
    PWSTR pwszSubKeyPrefix = NULL;
    PWSTR pwszSubKey = NULL;
    PMEMREG_NODE hSubKey = NULL;

    status = MemDbStackInit(MEMREG_MAX_SUBNODE_STACK, &hStack);
    BAIL_ON_NT_STATUS(status);
    hKeyNode = hDb->pMemReg;

    if (pwszOptSubKey)
    {
        status = MemRegStoreFindNodeSubkey(
                     hKeyNode,
                     pwszOptSubKey,
                     &hSubKey);
        BAIL_ON_NT_STATUS(status);
        hKeyNode = hSubKey;
    }

    /* Initially populate stack from top level node */
    if (!pwszOptSubKey)
    {
        for (index=hKeyNode->NodesLen-1; index>=0; index--)
        {
            status = LwRtlWC16StringAllocatePrintf(
                         &pwszSubKeyPrefix,
                         "%ws", hKeyNode->SubNodes[index]->Name);
            BAIL_ON_NT_STATUS(status);
            status = MemDbStackPush(
                         hStack,
                         hKeyNode->SubNodes[index],
                         pwszSubKeyPrefix);
            BAIL_ON_NT_STATUS(status);
            LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
        }
    }
    else
    {
        status = LwRtlWC16StringAllocatePrintf(
                     &pwszSubKeyPrefix,
                     "%ws", pwszOptSubKey);
        BAIL_ON_NT_STATUS(status);
        status = MemDbStackPush(
                     hStack,
                     hSubKey,
                     pwszSubKeyPrefix);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    }

    do
    {
        status = MemDbStackPop(hStack, &hKeyNode, &pwszSubKeyPrefix);
        if (status == 0)
        {
            if (hKeyNode->SubNodes && hKeyNode->NodesLen > 0)
            {
                /* This push should never fail */
                status = MemDbStackPush(hStack, hKeyNode, pwszSubKeyPrefix);
                BAIL_ON_NT_STATUS(status);
                for (index=hKeyNode->NodesLen-1; index>=0; index--)
                {
                    status = LwRtlWC16StringAllocatePrintf(
                                 &pwszSubKey, 
                                 "%ws\\%ws",
                                 pwszSubKeyPrefix,
                                 hKeyNode->SubNodes[index]->Name);
                    BAIL_ON_NT_STATUS(status);
                    status = MemDbStackPush(
                                 hStack,
                                 hKeyNode->SubNodes[index],
                                 pwszSubKey);
                    BAIL_ON_NT_STATUS(status);
                    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
                    LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
                }
            }
            else
            {
                /* This callback must do something to break the recursion */
                pfCallback(hKeyNode, (PVOID) userContext, 
                           pwszSubKeyPrefix, &statusCallback);
                LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);

                /* Bail if callback returns an error */
                status = statusCallback;
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            BAIL_ON_NT_STATUS(status);
        }
    } while (status != ERROR_EMPTY);

cleanup:
    if (status == ERROR_EMPTY)
    {
        status = 0;
    }

    MemDbStackFinish(hStack);
    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    LWREG_SAFE_FREE_MEMORY(pwszSubKeyPrefix);
    goto cleanup;
}
