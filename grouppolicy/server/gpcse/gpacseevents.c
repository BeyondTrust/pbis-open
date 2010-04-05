#include "includes.h"

typedef DWORD (*PFN_OPEN_EVENT_LOG_EX)(
    PCSTR pszServerName,
    DWORD dwEventTableCategoryId,
    PCSTR pszSource,
    DWORD dwEventSourceId,
    PCSTR pszUser,
    PCSTR pszComputer,
    PHANDLE phEventLog
    );

typedef DWORD (*PFN_CLOSE_EVENT_LOG)(
    HANDLE hEventLog
    );

typedef DWORD (*PFN_WRITE_EVENT_LOG)(
    HANDLE hEventLog,
    PCSTR eventType,
    PCSTR eventCategory,
    PCSTR eventDescription
    );

static HANDLE gpEventLogHandle = (HANDLE)NULL;


void
InitializeCSEEvents()
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    GPA_LOG_VERBOSE("Initializing Group Policy CSE events");

    if (gpEventLogHandle == (HANDLE)NULL)
    {
        ceError = LWIOpenEventLogEx(
                        NULL,
                        "System",
                        "Likewise GPAGENT",
                        0,
                        "SYSTEM",
                        NULL,
                        &gpEventLogHandle);
        if (ceError) 
        {
            GPA_LOG_ERROR("Failed to open event log!");
        }
    }

}

void
UninitializeCSEEvents()
{
    GPA_LOG_VERBOSE("Unitializing Group Policy CSE events");

    if (gpEventLogHandle) 
    {
        LWICloseEventLog(gpEventLogHandle);
        gpEventLogHandle = (HANDLE)NULL;
    }

}

CENTERROR
PostCSEEvent(
    DWORD dwEventID,
    PCSTR pszUser, // NULL defaults to SYSTEM
    PCSTR pszType,
    PCSTR pszCategory,
    PCSTR pszDescription,
    CENTERROR ceErrorCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    EVENT_LOG_RECORD event = {0};
    PSTR pszData = NULL;

    if (ceErrorCode != CENTERROR_SUCCESS)
    {
        // Going to convert the error status to a message to show in the event data field
        ceError = GPAGetErrorMessageForLoggingEvent(ceErrorCode, &pszData);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    GPA_LOG_INFO("Posting Group Policy event:");
    GPA_LOG_INFO("Event ID   : %d", dwEventID);
    GPA_LOG_INFO("User       : %s", pszUser ? pszUser : "<not set>");
    GPA_LOG_INFO("Type       : %s", pszType ? pszType : "<not set>");
    GPA_LOG_INFO("Category   : %s", pszCategory ? pszCategory : "<not set>");
    GPA_LOG_INFO("Description: %s", pszDescription ? pszDescription : "<not set>");
    GPA_LOG_INFO("Data       : %s", pszData ? pszData : "<not set>");

    event.dwEventRecordId = 0;
    event.pszEventTableCategoryId = NULL;
    event.pszEventType = (PSTR) pszType;
    event.dwEventDateTime = 0;
    event.pszEventSource = NULL;
    event.pszEventCategory = (PSTR) pszCategory;
    event.dwEventSourceId = dwEventID;
    event.pszUser = (PSTR) pszUser;
    event.pszComputer = NULL;
    event.pszDescription = (PSTR) pszDescription;
    event.pszData = (PSTR) pszData;

    InitializeCSEEvents();

    /* Test that the eventlog subsystem is available */
    if (!gpEventLogHandle) 
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        goto error;
    }

    ceError = LWIWriteEventLogBase(gpEventLogHandle,
                                   event);
    if (ceError)
    {
        GPA_LOG_ERROR("Failed to open event log!");
    	UninitializeCSEEvents();
    }

error:

    return ceError;
}

CENTERROR
GPAGetErrorMessageForLoggingEvent(
    CENTERROR ceErrorCode,
    PSTR * ppszData
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pszName = CTErrorName(ceErrorCode);
    PCSTR pszDescription = CTErrorDescription(ceErrorCode);
    PSTR pszMessage = NULL;

    ceError = LwAllocateStringPrintf(&pszMessage, "Error [%d]: %s (%s)",
                                     ceErrorCode,
                                     pszName ? pszName : "<error name not found>",
                                     pszDescription ? pszDescription : "<error description not found>");
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (ppszData)
    {
        *ppszData = pszMessage;
        pszMessage = NULL;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMessage);

    return ceError;

error:

    goto cleanup;
}

CENTERROR
MonitorSystemFiles(
    PSTR pszSystemFilePath,
    PSTR pszPrevCalMd5Sum,
    PSTR *ppszNewMd5Sum
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szCommand[PATH_MAX]= "";
    PSTR pszMd5Sum = NULL;
    PSTR pszDescription = NULL;

    //Form the command
    memset (szCommand,0,sizeof(szCommand));
#if defined(__LWI_DARWIN__)
    sprintf( szCommand,
             "%s%s%s",
             "md5 ",
             pszSystemFilePath,
             "  | cut -d \" \" -f4"
            );
#else
    sprintf( szCommand,
             "%s%s%s",
             "md5sum ",
             pszSystemFilePath,
             "  | cut -d \" \" -f1"
            );
#endif

    //Get the md5sum
    ceError = GPACaptureOutput(szCommand, &pszMd5Sum);
    if (ceError)
    {
        GPA_LOG_ERROR("ERROR:Output capture for the command %s is failed",szCommand);
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    GPA_LOG_VERBOSE("Previously Calculated MD5Sum is %s",pszPrevCalMd5Sum);
    GPA_LOG_VERBOSE("Calculated MD5Sum for %s is %s",pszSystemFilePath,pszMd5Sum);

    /* Compare with previously calculated md5 sum
     * If calculated md5sum doesnt match with the previously calculated then log the msg
     */

    //If prior md5sum is null then consider it as for the first time, this doesn't need comparing
    if ( !IsNullOrEmptyString(pszPrevCalMd5Sum))
    {
        if (strcmp( (char*)pszMd5Sum,
                    (char*)pszPrevCalMd5Sum)) 
        {

            ceError = LwAllocateStringPrintf(&pszDescription,
                                             "File monitoring detected a configuration change.\r\n\r\nThe MD5 sum calculation for the file (%s) is now different. Note: This change occurred outside of Group Policy ",
                                             pszSystemFilePath);
            BAIL_ON_CENTERIS_ERROR(ceError);

            GPA_LOG_ERROR("ERROR:MD5Sum differs, %s file is modified",pszSystemFilePath);

            CSE_WARN_EVENT(GPAGENT_EVENT_WARNING_SUDOER_FILE_CHANGED_LOCALLY,
                           pszDescription,
                           0);
        }
    }

    //update global variable
    *ppszNewMd5Sum = pszMd5Sum;

cleanup:
    LW_SAFE_FREE_STRING(pszDescription);
    return ceError;

error:
    LW_SAFE_FREE_STRING(pszMd5Sum);
    goto cleanup;
}

CENTERROR
GetMonitorFile(
    xmlNodePtr root_node,
    PSTR pszSetting,
    PBOOLEAN pbMonitor  
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    xmlNodePtr cur_node = NULL;
    xmlChar* pszStatus = NULL;
    PBOOLEAN pbMonitorFile = pbMonitor;

    for (cur_node = root_node; cur_node; cur_node = cur_node->next) {

        if (cur_node->name && !strcmp( (char*)cur_node->name,
                                       pszSetting)) {

            pszStatus  = xmlGetProp( cur_node,
                                    (const xmlChar*)"monitor");

            if(pszStatus) {

                if(!strcmp((const char*)pszStatus, "true")) {
                    *pbMonitor = TRUE;
                }
            }

            xmlFree(pszStatus);

            break;
        }

        GetMonitorFile( cur_node->children,
                        pszSetting,
                        pbMonitorFile);

    }

    return ceError;
}
