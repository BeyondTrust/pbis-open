#ifndef __GPAEVENTS_H__
#define __GPAEVENTS_H__

#define SERVICE_EVENT_CATEGORY      "Service"
#define POLICY_EVENT_CATEGORY       "Policy"

#define SUCCESS_AUDIT_EVENT_TYPE    "Success Audit"
#define FAILURE_AUDIT_EVENT_TYPE    "Failure Audit"
#define INFORMATION_EVENT_TYPE      "Information"
#define WARNING_EVENT_TYPE          "Warning"
#define ERROR_EVENT_TYPE            "Error"


#define GPAGENT_EVENT_INFO_SERVICE_STARTED                         1000
#define GPAGENT_EVENT_ERROR_SERVICE_START_FAILURE                  1001
#define GPAGENT_EVENT_INFO_SERVICE_STOPPED                         1002
#define GPAGENT_EVENT_ERROR_SERVICE_STOPPED                        1003
#define GPAGENT_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED           1004

// GPAgent policy update events
#define GPAGENT_EVENT_POLICY_UPDATED                               1100
#define GPAGENT_EVENT_POLICY_UPDATE_FAILURE                        1101

// GPAgent policy processing issue events
#define GPAGENT_EVENT_INFO_POLICY_PROCESSING_ISSUE_RESOLVED        1200
#define GPAGENT_EVENT_ERROR_POLICY_PROCESSING_ISSUE_ENCOUNTERED    1201

VOID
GPAInitializeEvents(
    VOID
    );

CENTERROR
GPAPostEvent(
    DWORD dwEventID,
    PCSTR pszUser, // NULL defaults to SYSTEM
    PCSTR pszType,
    PCSTR pszCategory,
    PCSTR pszDescription,
    CENTERROR ceErrorCode
    );

BOOLEAN
GPAEventLoggingIsEnabled(
    VOID
    );

VOID
GPAShutdownEvents(
    VOID
    );

VOID
GPAShutdownEvents_InLock(
    VOID
    );

CENTERROR
GPAGetErrorMessageForLoggingEvent(
    CENTERROR ceErrorCode,
    PSTR * ppszData
    );

#endif /* __GPAEVENTS_H__ */

