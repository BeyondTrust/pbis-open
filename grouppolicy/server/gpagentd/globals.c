#include "includes.h"

pthread_mutex_t ghEventLogLock = PTHREAD_MUTEX_INITIALIZER;
HANDLE ghEventLog = (HANDLE)NULL;

GPOSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,     /* lock         */
    NULL,                          /* pszConfigFilePath */
    NULL,                          /* pszPolicySettingsFilePath */
    NULL,                          /* pszExtensionLibPath */
    NULL,                          /* pszPrefixPath */
    NULL,                          /* pszKrb5CredCacheEnvSetting */
    GPO_DEFAULT_POLL_TIMEOUT_SECS, /* dwComputerPollingInterval */
    GPO_DEFAULT_POLL_TIMEOUT_SECS, /* dwUserPollingInterval */
    FALSE,                         /* bEnableEventLog */
    FALSE,                         /* bMonitorSudoers */
    LOOPBACK_PROCESSING_MODE_USER_ONLY, /* lpmUserPolicyMode */
    FALSE,                         /* bProcessShouldExit */
    0,                             /* dwExitCode */
    NULL,                          /* pszDistroName */
    NULL                           /* pszDistroVersion */
};

pthread_t gServerThread;
PVOID     pgServerThread = NULL;

sigset_t group_policy_signal_set;
