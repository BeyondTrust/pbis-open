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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog parser to parse log files from Microsoft Event Viewer.
 *
 */
#include "includes.h"

//Character indices to use in PRINT_TABLE option.
//This allows printing a user-readable table to stdout that 80 characters wide.
#define TABLOC_ID 0
#define TABLOC_TYPE      (TABLOC_ID+8)
#define TABLOC_DATE      (TABLOC_TYPE+16)
#define TABLOC_TIME      (TABLOC_DATE+13)
#define TABLOC_SOURCE    (TABLOC_TIME+14)
#define TABLOC_CATEGORY  (TABLOC_SOURCE+20)
#define TABLOC_SOURCE_ID (TABLOC_CATEGORY+25)
#define TABLOC_USER      (TABLOC_SOURCE_ID+10)
#define TABLOC_BORDER " | "

//
// When you save the log file on windows, the following seems
// to be the default order of fields.
//
typedef enum
{
    EVENT_TABLE_CATEGORY_ID = 0,
    EVENT_DATE, //1
    EVENT_TIME, //2
    EVENT_SOURCE, //3
    EVENT_TYPE, //4
    EVENT_CATEGORY, //5
    EVENT_SOURCE_ID, //6
    EVENT_USER, //7
    EVENT_COMPUTER, //8
    EVENT_DESCRIPTION, //9
    EVENT_DATA, //10
    EVENT_FIELD_TYPE_SENTINEL
} EventFieldType;

static
DWORD
ExportEventRecord (
    PEVENT_LOG_RECORD pRecord,
    FILE* fpExport
    )
{

    DWORD dwError = 0;

    char eventDate[256];
    char eventTime[256];

    if (pRecord == NULL) return -1;
    if (fpExport == NULL) return -1;

    /*
     * CSV fields:
     *    LogfileName,Date,Time,Source,Type,Category,SourceID,User,Computer,Description,Data
     */
    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    fprintf(fpExport, "%s,%s,%s,%s,%s,%s,%d,%s,%s,\"%s\",\"%s\"\n",
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventTableCategoryId) ? "<null>" : (PSTR)pRecord->pszEventTableCategoryId,
        eventDate, //PSTR
        eventTime, //PSTR
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventSource,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventType) ? "<null>" : (PSTR)pRecord->pszEventType,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventCategory) ? "<null>" : (PSTR)pRecord->pszEventCategory,
        pRecord->dwEventSourceId, //DWORD
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszUser) ? "<null>" : (PSTR)pRecord->pszUser,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszComputer) ? "<null>" : (PSTR)pRecord->pszComputer,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszDescription) ? "<null>" : (PSTR)pRecord->pszDescription,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszData) ? "<null>" : (PSTR)pRecord->pszData);

    return dwError;
}

static
DWORD
PrintEventRecordTableRow (
    PEVENT_LOG_RECORD pRecord,
    FILE* fp
    )
{

    DWORD dwError = 0;
    DWORD i = 0;   

    char eventDate[256];
    char eventTime[256];


    char buf[256];

    if (pRecord == NULL) return -1;
    if (fp == NULL) return -1;

    //TableRow fields: RecordID,Type,Date,Time,Source,Category,EventID,User

    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    memset(buf, ' ', 255);

    sprintf(buf,                  "%d", pRecord->dwEventRecordId);

    sprintf(buf+TABLOC_TYPE,     "%s%s", TABLOC_BORDER,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventType) ? "<null>" : (PSTR)pRecord->pszEventType);

    sprintf(buf+TABLOC_DATE,     "%s%s", TABLOC_BORDER,
        eventDate);

    sprintf(buf+TABLOC_TIME,     "%s%s", TABLOC_BORDER,
        eventTime);

    sprintf(buf+TABLOC_SOURCE,   "%s%s", TABLOC_BORDER,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventSource);

    sprintf(buf+TABLOC_CATEGORY, "%s%s", TABLOC_BORDER,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventCategory) ? "<null>" : (PSTR)pRecord->pszEventCategory);
    
    sprintf(buf+TABLOC_SOURCE_ID, "%s%d", TABLOC_BORDER,
        pRecord->dwEventSourceId);
    
    sprintf(buf+TABLOC_USER, "%s%s", TABLOC_BORDER,
        LW_IS_NULL_OR_EMPTY_STR(pRecord->pszUser) ? "<null>" : (PSTR)pRecord->pszUser);

    for (i = 0; i <= TABLOC_USER; i++)
    {
        if (buf[i] == (char)0)
        {
            buf[i] = ' ';
        }
    }

    fprintf(fp, "%s\n", buf);

    return dwError;
}

DWORD
PrintEventRecords(
    FILE* output,
    EVENT_LOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    char eventDate[256];
    char eventTime[256];

    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;
    int iRecord = 0;

    for (iRecord = 0; iRecord < nRecords; iRecord++)
    {
    EVENT_LOG_RECORD* pRecord = &(eventRecords[iRecord]);

    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    printf("Event Record: (%d/%d) (%d total)\n", iRecord+1, nRecords, ++totalRecordsLocal);
    printf("========================================\n");
    printf("Event Record ID......... %d\n", pRecord->dwEventRecordId);
    printf("Event Table Category.... %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventTableCategoryId));
    printf("Event Type.............. %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventType));
    printf("Event Date.............. %s\n", eventDate);
    printf("Event Time.............. %s\n", eventTime);
    printf("Event Source............ %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventSource));
    printf("Event Category.......... %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventCategory));
    printf("Event Source ID......... %d\n", pRecord->dwEventSourceId);
    printf("Event User.............. %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszUser) ? "<null>" : (char*) (pRecord->pszUser));
    printf("Event Computer.......... %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszComputer) ? "<null>" : (char*) (pRecord->pszComputer));
    printf("Event Description....... %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszDescription) ? "<null>" : (char*) (pRecord->pszDescription));
    printf("Event Data.............. %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pRecord->pszData) ? "<null>" : (char*) (pRecord->pszData));
    printf("========================================\n");

    }

    *totalRecords = totalRecordsLocal;

    return dwError;
}

DWORD
PrintEventRecordsTable(
    FILE* output,
    EVENT_LOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;

    char buf[256];
    memset(buf, ' ', 255);

    sprintf(buf, "Id:   ");
    sprintf(buf+TABLOC_TYPE,     "%sType", TABLOC_BORDER);
    sprintf(buf+TABLOC_DATE,     "%sDate", TABLOC_BORDER);
    sprintf(buf+TABLOC_TIME,     "%sTime", TABLOC_BORDER);
    sprintf(buf+TABLOC_SOURCE,   "%sSource", TABLOC_BORDER);
    sprintf(buf+TABLOC_CATEGORY, "%sCategory", TABLOC_BORDER);
    sprintf(buf+TABLOC_SOURCE_ID,"%sEvent", TABLOC_BORDER);
    sprintf(buf+TABLOC_USER,"%sUser", TABLOC_BORDER);

    for (i = 0; i <= TABLOC_USER; i++)
    {
        if (buf[i] == (char)0)
        {
            buf[i] = ' ';
        }
    }

    fprintf(output, "%s\n", buf);

    for (i = 0; i < nRecords; i++)
    {
        dwError = PrintEventRecordTableRow(&(eventRecords[i]), output);
        BAIL_ON_EVT_ERROR(dwError);
        totalRecordsLocal++;
    }


 error:

    *totalRecords = totalRecordsLocal;

    return dwError;
}

DWORD
ReadAndExportEvents(
    PEVENT_LOG_HANDLE pEventLogHandle,
    PCWSTR pwszSqlFilter,
    FILE* fpExport
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    const DWORD pageSize = 2000;
    DWORD currentEntry = 0;
    DWORD entriesRead = 0;
    EVENT_LOG_RECORD* records = NULL;

    if (fpExport == NULL) return -1;
    if (pEventLogHandle == NULL) return -1;

    /*
     * CSV fields:
     *    LogfileName,Date,Time,Source,Type,Category,SourceID,User,Computer,Description,Data
     */
    fprintf(fpExport, "LogfileName,");
    fprintf(fpExport, "Date,");
    fprintf(fpExport, "Time,");
    fprintf(fpExport, "Source,");
    fprintf(fpExport, "Type,");
    fprintf(fpExport, "Category,");
    fprintf(fpExport, "SourceId,");
    fprintf(fpExport, "User,");
    fprintf(fpExport, "Computer,");
    fprintf(fpExport, "Description,");
    fprintf(fpExport, "Data\n");

    do
    {
        dwError = LWIReadEventLog(
                    (HANDLE)pEventLogHandle,
                    currentEntry,
                    pageSize,
                    pwszSqlFilter,
                    &entriesRead,
                    &records);
        BAIL_ON_EVT_ERROR(dwError);

        for (i = 0; i < entriesRead; i++) {
            dwError = ExportEventRecord(&(records[i]), fpExport);
            BAIL_ON_EVT_ERROR(dwError);
        }

        fflush(fpExport);

        currentEntry += entriesRead;

    } while (entriesRead == pageSize && entriesRead > 0);

 cleanup:

    RPCFreeMemory(records);

    LwFreeMemory(records);

    return dwError;

error:

    goto cleanup;

}

