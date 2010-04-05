#ifndef __GPADEFINES_H__
#define __GPADEFINES_H__

#define MAX_TOKEN_LENGTH 512

#define MAX_SOCKET_STALETIME 30

#define GPO_DEFAULT_POLL_TIMEOUT_SECS (30 * 60)
#define GPO_MIN_POLL_TIMEOUT_SECS     (60)
#define GPO_MAX_POLL_TIMEOUT_SECS     (24 * 60 * 60)

#define KRB5CCENVVAR "KRB5CCNAME"

#define DC_PREFIX "dc="

#define DEFAULT_CONFIG_FILE_NAME  "grouppolicy.conf"
#define POLICY_SETTINGS_FILE_NAME "grouppolicy-settings.conf"

#define GP_PID_DIR LOCALSTATEDIR "/run"
#define GP_AGENT_DAEMON_NAME "gpagentd"
#define GP_AGENT_PID_FILE GP_PID_DIR "/" GP_AGENT_DAEMON_NAME ".pid"

#define GPO_SUCCESS_EVENT(id, user, description)                  \
      GPAPostEvent(id, user, SUCCESS_AUDIT_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, CENTERROR_SUCCESS);

#define GPO_FAILURE_EVENT(id, user, description, code)            \
      GPAPostEvent(id, user, FAILURE_AUDIT_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define GPO_ERROR_EVENT(id, user, description, code)              \
      GPAPostEvent(id, user, ERROR_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define GPO_WARN_EVENT(id, user, description, code)               \
      GPAPostEvent(id, user, WARNING_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, code);

#define GPO_INFO_EVENT(id, user, description)                     \
      GPAPostEvent(id, user, INFORMATION_EVENT_TYPE, POLICY_EVENT_CATEGORY, description, CENTERROR_SUCCESS);

#define GPA_SUCCESS_EVENT(id, description)                        \
      GPAPostEvent(id, NULL, SUCCESS_AUDIT_EVENT_TYPE, SERVICE_EVENT_CATEGORY, description, CENTERROR_SUCCESS);

#define GPA_FAILURE_EVENT(id, description, code)                  \
      GPAPostEvent(id, NULL, FAILURE_AUDIT_EVENT_TYPE, SERVICE_EVENT_CATEGORY, description, code);

#define GPA_ERROR_EVENT(id, description, code)                    \
      GPAPostEvent(id, NULL, ERROR_EVENT_TYPE, SERVICE_EVENT_CATEGORY, description, code);

#define GPA_WARN_EVENT(id, description, code)                     \
      GPAPostEvent(id, NULL, WARNING_EVENT_TYPE, SERVICE_EVENT_CATEGORY, description, code);

#define GPA_INFO_EVENT(id, description)                           \
      GPAPostEvent(id, NULL, INFORMATION_EVENT_TYPE, SERVICE_EVENT_CATEGORY, description, CENTERROR_SUCCESS);

#endif /* __GPADEFINES_H__ */


