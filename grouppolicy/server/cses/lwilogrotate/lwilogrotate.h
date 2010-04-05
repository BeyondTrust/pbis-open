#ifndef __LWILOGROTATE_H__
#define __LWILOGROTATE_H__

#define LWILOGROTATE_CLIENT_GUID            "{B1BBA22A-08FF-4826-9B4B-151C8A0BC1CA}"
#define LWILOGROTATE_ITEM_GUID              "{98AAAFB1-F311-40e7-B58D-B93C0A21827F}"

#define CENTERIS_GP_DIRECTORY               CACHEDIR "/"

#define LWI_LOGROTATE_POLICY_FILE_HEADER    "## NOTE:\n"\
                                            "## This file is managed by Likewise Enterprise Group Policy.\n"

#define LWI_LOGROTATE_CENTERIS_GP_DIRECTORY "/etc/logrotate.conf.lwidentity.orig"

#define LWI_LOGROTATE_CONF_FILE          "/etc/logrotate.conf"
#define LWI_LOGROTATE_CONF_FILE_GP       "/etc/logrotate.conf.gp"
#define LWI_LOGROTATED_DIR_PATH          "/etc/logrotate.d/"

#define MAX_BUF_SIZE                     512
#define STATIC_PATH_BUFFER_SIZE          256

#define LWI_LOG_FILE_PATH_TAG            "LogFilePath"
#define LWI_LOG_PRE_COMMAND_TAG          "prerotate"
#define LWI_LOG_POST_COMMAND_TAG         "postrotate"
#define LWI_LOG_ROTATE_ITEM_TAG          "LogRotateItem"
#define LWI_LOG_FILE_TYPE_TAG            "LogFileType"
#define LWI_LOG_APP_NAME_TAG             "LogAppName"
#define LWI_LOG_ENDSCRIPT_TAG            "endscript"

#define LWI_LOG_ROTATE                   "logrotate"
#define LWI_LOG_ROTATE_D                 "logrotate.d"

#define LWI_LOG_OPTIONS_TAG              "LogOptions"
#define LWI_LOG_COMPRESS_ATTR            "compress"
#define LWI_LOG_COMPRESSCMD_ATTR         "compresscmd"
#define LWI_LOG_UNCOMPRESSCMD_ATTR       "uncompresscmd"
#define LWI_LOG_COMPRESSEXT_ATTR         "compressext"
#define LWI_LOG_COMPRESSOPTIONS_ATTR     "compressoptions"
#define LWI_LOG_COPY_ATTR                "copy"
#define LWI_LOG_COPYTRUNCATE_ATTR        "copytruncate"
#define LWI_LOG_CREATE_ATTR              "create"
#define LWI_LOG_DAILY_ATTR               "daily"
#define LWI_LOG_DELAYCOMPRESS_ATTR       "delaycompress"
#define LWI_LOG_ERRORS_ATTR              "errors"
#define LWI_LOG_EXTENSION_ATTR           "extension"
#define LWI_LOG_FIRSTACTION_ATTR         "firstaction"
#define LWI_LOG_GROUP_ATTR               "group"
#define LWI_LOG_IFEMPTY_ATTR             "ifempty"
#define LWI_LOG_INCLUDE_ATTR             "include"
#define LWI_LOG_MAIL_ATTR                "mail"
#define LWI_LOG_MAILFIRST_ATTR           "mailfirst"
#define LWI_LOG_MAILLAST_ATTR            "maillast"
#define LWI_LOG_MISSINGOK_ATTR           "missingok"
#define LWI_LOG_MODE_ATTR                "mode"
#define LWI_LOG_MONTHLY_ATTR             "monthly"
#define LWI_LOG_NOCOMPRESS_ATTR          "nocompress"
#define LWI_LOG_NOCOPY_ATTR              "nocopy"
#define LWI_LOG_NOCOPYTRUNCATE_ATTR      "nocopytruncate"
#define LWI_LOG_NOCREATE_ATTR            "nocreate"
#define LWI_LOG_NODELAYCOMPRESS_ATTR     "nodelaycompress"
#define LWI_LOG_NOMAIL_ATTR              "nomail"
#define LWI_LOG_NOMISSINGOK_ATTR         "nomissingok"
#define LWI_LOG_NOOLDDIR_ATTR            "noolddir"
#define LWI_LOG_NOSHAREDSCRIPTS_ATTR     "nosharedscripts"
#define LWI_LOG_NOTIFEMPTY_ATTR          "notifempty"
#define LWI_LOG_OLDDIR_ATTR              "olddir"
#define LWI_LOG_OWNER_ATTR               "owner"
#define LWI_LOG_LASTACTION_ATTR          "lastaction"
#define LWI_LOG_ROTATE_ATTR              "rotate"
#define LWI_LOG_SIZE_ATTR                "size"
#define LWI_LOG_SHAREDSCRIPTS_ATTR       "sharedscripts"
#define LWI_LOG_START_ATTR               "start"
#define LWI_LOG_TABOOEXT_ATTR            "tabooext"
#define LWI_LOG_WEEKLY_ATTR              "weekly"
#define LWI_LOG_YEARLY_ATTR              "yearly"

#define DIR_PERMS                        S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH

typedef struct FilePath {
    PSTR pszFile;
    struct FilePath *pNext;
}LOGFILEPATH, *PLOGFILEPATH;

typedef struct LOGROTATE_ENTRY {
    PSTR pszFileType;
    PLOGFILEPATH pFilePath;
    PSTR pszAppName;
    CHAR cCompress;
    PSTR pszCompressCmd;
    PSTR pszCompressExt;
    PSTR pszCompressOptions;
    CHAR cCopy;
    CHAR cCopyTruncate;
    PSTR pszCreate;
    CHAR cDelayCompress;
    PSTR pszExtension;
    PSTR pszFirstAction;
    CHAR cIfempty;
    PSTR pszInclude;
    PSTR pszLastAction;
    PSTR pszLogFrequency;
    PSTR pszMail;
    CHAR cMailFirst;
    CHAR cMailLast;
    PSTR pszMaxLogSize;
    CHAR cMissingOk;
    CHAR cNoCompress;
    CHAR cNoCopy;
    CHAR cNoCopyTruncate;
    CHAR cNoCreate;
    CHAR cNoDelayCompress;
    CHAR cNoMail;
    CHAR cNoMissingOk;
    CHAR cNoOldDir;
    CHAR cNoSharedScripts;
    CHAR cNotIfempty;
    PSTR pszOldDir;
    PSTR pszPostRotate;
    PSTR pszPreRotate;
    PSTR pszRotate;
    CHAR cSharedScripts;
    PSTR pszStart;
    PSTR pszTabooExt;
    PSTR pszUnCompressCmd;
    CHAR cDone;
    struct LOGROTATE_ENTRY *pNext;
}LOGROTATE_POLICY, *PLOGROTATE_POLICY; 

CENTERROR
ProcessLogRotateGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetLogRotateGroupPolicy();

#endif /* __LWILOGROTATE_H__ */

