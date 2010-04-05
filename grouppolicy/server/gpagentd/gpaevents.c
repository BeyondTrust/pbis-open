#include "includes.h"

VOID
GPAInitializeEvents(
    VOID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    pthread_mutex_lock(&ghEventLogLock);

    GPA_LOG_INFO("Initializing Group Policy events");

    if (ghEventLog == (HANDLE)NULL)
    {
        ceError = LWIOpenEventLogEx(
                        NULL,
                        "System",
                        "Likewise GPAGENT",
                        0,
                        "SYSTEM",
                        NULL,
                        &ghEventLog);
        if (ceError) {
            GPA_LOG_ERROR("Failed to open event log!");
        }
    }

    pthread_mutex_unlock(&ghEventLogLock);

}

CENTERROR
GPAPostEvent(
    DWORD dwEventID,
    PCSTR pszUser, // NULL defaults to SYSTEM
    PCSTR pszType,
    PCSTR pszCategory,
    PCSTR pszDescription,
    CENTERROR ceErrorCode
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bUnlock = FALSE;
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

    if (!GPAEventLoggingIsEnabled())
    {
        goto cleanup;
    }

    GPAInitializeEvents();

    pthread_mutex_lock(&ghEventLogLock);
    bUnlock = TRUE;

    if (ghEventLog != (HANDLE)NULL)
    {
        /* Test the parameters */
        if (!pszType || !pszCategory || !pszDescription) {
            ceError = CENTERROR_INVALID_PARAMETER;
            goto error;
        }

        ceError = LWIWriteEventLogBase(
                        ghEventLog,
                        event);
        if (ceError)
        {
            GPAShutdownEvents_InLock();
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pszData);

    if (bUnlock)
    {
        pthread_mutex_unlock(&ghEventLogLock);
    }

    return ceError;

error:

    if (ceError)
    {
        GPA_LOG_VERBOSE("Failed to post event");
    }

    goto cleanup;
}

BOOLEAN
GPAEventLoggingIsEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;

    GPA_LOCK_SERVERINFO;

    bResult = gServerInfo.bEnableEventLog;

    GPA_UNLOCK_SERVERINFO;

    return bResult;
}

VOID
GPAShutdownEvents(
    VOID
    )
{
    pthread_mutex_lock(&ghEventLogLock);

    GPAShutdownEvents_InLock();

    pthread_mutex_unlock(&ghEventLogLock);
}

VOID
GPAShutdownEvents_InLock(
    VOID
    )
{
    GPA_LOG_INFO("Shutting down Group Policy events");

    if (ghEventLog != (HANDLE)NULL)
    {
        LWICloseEventLog(ghEventLog);
        ghEventLog = (HANDLE)NULL;
    }

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



