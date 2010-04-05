#ifndef __MAIN_H__
#define __MAIN_H__

CENTERROR
GetConfigPath(
    PSTR* ppszPath
    );

CENTERROR
GetPolicySettingsPath(
    PSTR* ppszPath
    );

CENTERROR
GetExtensionLibPath(
    PSTR* ppszPath
    );

CENTERROR
GetPrefixPath(
    PSTR* ppszPath
    );

CENTERROR
GetComputerPollingInterval(
    PDWORD pdwPollingInterval
    );

CENTERROR
GetUserPollingInterval(
    PDWORD pdwPollingInterval
    );

CENTERROR
GetEnableEventLog(
    PBOOLEAN pbEnableEventLog
    );

CENTERROR
GetLoopbackProcessingMode(
    LoopbackProcessingMode * plpmUserPolicyMode
    );

CENTERROR
GetDistroName(
    PSTR* ppszDistroName
    );

CENTERROR
GetDistroVersion(
    PSTR* ppszDistroVersion
    );

BOOLEAN
ProcessShouldExit();

void
SetProcessShouldExit(
    BOOLEAN val
    );

CENTERROR
SetKrb5ConfigEnv(
    PSTR pszShortDomainName
    );

BOOLEAN
IsKrb5ConfigEnvSet();

CENTERROR
group_policy_process_run(
    int argc,
    char* argv[]
    );

void
group_policy_process_exit(
    int retCode
    );

/* HACK - This is to satisfy the less than modular test code */
CENTERROR
GPOSetServerDefaults();

CENTERROR
LoadGroupPolicyConfigurationSettings(
    BOOLEAN bRefresh
    );

CENTERROR
FlushDirectoryServiceCache();

#endif /* __MAIN_H__ */
