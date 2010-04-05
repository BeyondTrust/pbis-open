#ifndef __LWIPROMPT_H__
#define __LWIPROMPT_H__

#define LWIPROMPT_CLIENT_GUID       "{9020E541-F49C-4ab8-88F3-55BE2D95B440}"
#define LWIPROMPT_ITEM_GUID         "{4BC2DDEB-8E7A-48ae-90CF-6D568231471A}"

#define LOGIN_PROMPT_XML_NODE_QUERY "loginPrompt"

#define CONFIG_FILE_PATH_SEC        "ConfigFilePath"

typedef struct __POLICYLIST
{
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

static const PSTR ISSUE_FILE             = "/etc/issue";
static const PSTR ISSUE_OLD_FILE         = "/etc/issue.lwidentity.orig";
static const PSTR CENTERIS_GP_DIRECTORY  = CACHEDIR "/";

#endif

