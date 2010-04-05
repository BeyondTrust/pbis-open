#ifndef __LWISELINUX_H__
#define __LWISELINUX_H__

#define LWISELINUX_CLIENT_GUID "{0BCE95E2-5332-49dc-9878-D3F8B678734B}"
#define LWISELINUX_ITEM_GUID "{151FD89A-18AA-45e4-9466-D84E35FD6083}"

#define SELINUX_XML_NODE_QUERY "selinux"

#define SELINUX_MODE_KEYWORD "SELINUX"
#define SELINUX_TYPE_KEYWORD "SELINUXTYPE"

#define SELINUX_MODE_TAG_NAME "SELinuxMode"
#define SELINUX_TYPE_TAG_NAME "SELinuxType"

#define  LWISELINUX_FILE_HEADER "## NOTE:\n"\
                                "## This file is managed by Likewise Enterprise Group Policy.\n\n"

#define SELINUX_SYSTEM_HEADER "# This file controls the state of SELinux on the system.\n" \
                              "# SELINUX= can take one of these three values:\n" \
                              "#   enforcing - SELinux security policy is enforced.\n" \
                              "#   permissive - SELinux prints warnings instead of enforcing.\n" \
                              "#   disabled - SELinux is fully disabled."

#define SELINUXTYPE_SYSTEM_HEADER "# SELINUXTYPE= type of policy in use. Possible values are:\n" \
                                  "#   targeted - Only targeted network daemons are protected.\n" \
                                  "#   strict - Full SELinux protection."

#define REDHAT_RELEASE_FILE "/etc/redhat-release"

static const PSTR SELINUX_DIRECTORY = "/etc/selinux/";
static const PSTR SELINUX_FILE     = "/etc/selinux/config";
static const PSTR SELINUX_OLD_FILE  = "/etc/selinux/config.lwidentity.orig";
static const PSTR CENTERIS_GP_DIRECTORY  = CACHEDIR "/";

typedef struct __POLICYLIST
{
    PSTR FileName;
    struct __POLICYLIST *next;
} POLICYLIST, * PPOLICYLIST;

#endif

