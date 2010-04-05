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
        if (ceError) {
            GPA_LOG_ERROR("Failed to open event log!");
        }
    }

}

void
UninitializeCSEEvents()
{
    GPA_LOG_VERBOSE("Unitializing Group Policy CSE events");

    if (gpEventLogHandle) {
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
    if (!gpEventLogHandle) {
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

