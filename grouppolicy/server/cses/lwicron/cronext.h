#ifndef __CRONEXT_H__
#define __CRONEXT_H__

#define LWICRON_CLIENT_GUID     "{B9CA8919-71D7-4aaa-9567-7225965F4A0E}"
#define LWICRON_ITEM_GUID       "{C54AD223-08E6-4174-B1C5-01B6ED7AEC6F}"

static DWORD gIsCronD; 

#if !defined(__LWI_SOLARIS__)
static const DWORD BUFSIZE              = 4096;
#endif
static const PSTR CRONTAB_OLD_FILE      =  "/etc/crontab.lwidentity.orig";
static const PSTR CRONTAB_FILE          =  "/etc/crontab";
static const PSTR CRON_TMP_FILE         = CACHEDIR "/likewise-cron.new";
static const PSTR CROND_DIRECTORY       = "/etc/cron.d/";
static const PSTR CENTERIS_GP_DIRECTORY = CACHEDIR "/";
static const PSTR CENTERIS_SYSVOL_PATH  = "\\Machine\\Centeris\\Identity\\";

/*static const xmlChar* CRON_XML_SETTING_NODE_NAME = (xmlChar*)"crontab file";*/
/*static const xmlChar* CRON_XML_ATTRIBUTE_NAME = (xmlChar*)"crontabFile";*/

#if defined(__LWI_DARWIN__)
#define MAC_CRON_PID_FILE   "/var/run/cron.pid"    
#endif

static CENTERROR BackupCrontab(PSTR);
static CENTERROR UpdateCrontab(PSTR);
static CENTERROR AddPolicyFileToHierarchy(PSTR);
static CENTERROR AddPolicyFileToRemoveList(PSTR);
static CENTERROR RemovePolicyFileFromRemoveList(PSTR);
static CENTERROR GetCurrentListOfCronDPolicies();
static CENTERROR ProcessCrontabPolicyFiles(BOOLEAN);
static CENTERROR ProcessPolicyRemoveList();
static void ResetPolicyFileHierarchy();
static void ResetPolicyRemoveList();
static CENTERROR RemoveCronPolicy(PGROUP_POLICY_OBJECT);
static CENTERROR AddCronPolicy(PGROUP_POLICY_OBJECT, BOOLEAN, PBOOLEAN);

static const PSTR CRONTAB_USR_LOCAL_BIN = "/usr/local/bin/crontab";
static const PSTR CRONTAB_USR_BIN       = "/usr/bin/crontab";
static const PSTR CRONTAB_BIN           = "/bin/crontab";

typedef struct __POLICYLIST {
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

#endif /* __CRONEXT_H__ */

