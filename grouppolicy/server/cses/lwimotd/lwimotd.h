#ifndef __LWIMOTD_H__
#define __LWIMOTD_H__

#define LWIMOTD_CLIENT_GUID     "{9A9F29C0-B1B1-467d-A255-0BD3D7AAAE59}"
#define LWIMOTD_ITEM_GUID       "{051B4CD2-0AF3-485f-B437-6D46370CBD8D}"

#define MOTD_XML_NODE_QUERY     "motd"

#define CONFIG_FILE_PATH_SEC    "ConfigFilePath"

static const PSTR MOTD_FILE              = "/etc/motd";
static const PSTR MOTD_OLD_FILE          = "/etc/motd.lwidentity.orig";
static const PSTR CENTERIS_GP_DIRECTORY  = CACHEDIR "/";

typedef struct __POLICYLIST
{
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

#endif

