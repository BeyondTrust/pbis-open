#ifndef __CSEEVENTS_H__
#define __CSEEVENTS_H__

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
#define GPAGENT_EVENT_INFO_SUDOER_FILE_CHANGED_BY_POLICY           1202
#define GPAGENT_EVENT_WARNING_SUDOER_FILE_CHANGED_LOCALLY          1203


void
InitializeCSEEvents();

void
UninitializeCSEEvents();

CENTERROR
PostCSEEvent(
    DWORD dwEventID,
    PCSTR pszUser, // NULL defaults to SYSTEM
    PCSTR pszType,
    PCSTR pszCategory,
    PCSTR pszDescription,
    CENTERROR ceErrorCode
    );

CENTERROR
GPAGetErrorMessageForLoggingEvent(
    CENTERROR ceErrorCode,
    PSTR * ppszData
    );

CENTERROR
MonitorSystemFiles(
    PSTR pszSystemFilePath,
    PSTR pszPrevCalMd5Sum,
    PSTR *ppszNewMd5Sum
    );

CENTERROR
GetMonitorFile(
    xmlNodePtr root_node,
    PSTR pszSetting,
    PBOOLEAN pbMonitor  
    );

#define CSE_SUCCESS_EVENT(event, description, code)                    \
      PostCSEEvent(event, NULL, SUCCESS_AUDIT_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define CSE_FAILURE_EVENT(event, description, code)                    \
      PostCSEEvent(event, NULL, FAILURE_AUDIT_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define CSE_ERROR_EVENT(event, description, code)                      \
      PostCSEEvent(event, NULL, ERROR_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define CSE_WARN_EVENT(event, description, code)                       \
      PostCSEEvent(event, NULL, WARNING_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define CSE_INFO_EVENT(event, description, code)                       \
      PostCSEEvent(event, NULL, INFORMATION_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#endif /* __CSEEVENTS_H__ */

