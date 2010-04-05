#ifndef __MOUNTEXT_H__
#define __MOUNTEXT_H__

#define LWIAUTOMOUNT_ITEM_GUID      "{12587328-5C0D-46bd-BE9B-BF264F6CA720}"
#define LWIAUTOMOUNT_CLIENT_GUID    "{9994B0EB-ABE5-4654-A123-3B7818B2A999}"

#define AUTOMOUNT_XML_QUERY         "autoMount/setting/AUTOMOUNTPOLICY"

static const PCSTR GP_AM_MASTER_FILENAME        = "auto_master";
static const PCSTR GP_AM_REDIRECT_LINK          = "/etc/lwi_automount";
static const PSTR CENTERIS_GP_DIRECTORY         = CACHEDIR "/";
static const PSTR CENTERIS_GP_BACKUP_DIRECTORY  = "/etc/";

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__) || defined(__LWI_AIX__) || defined(__LWI_DARWIN__)
static const PCSTR AM_MASTER_FILENAME = "auto_master";
static const PCSTR AM_MASTER_FILEPATH = "/etc/auto_master";
#else
static const PCSTR AM_MASTER_FILENAME = "auto.master";
static const PCSTR AM_MASTER_FILEPATH = "/etc/auto.master";
#endif

typedef struct __MOUNTPOLICYLIST
{
    PSTR pszFilepath;
    PSTR pszDirpath;
    PSTR pszPolicyIdentifier;
    struct __MOUNTPOLICYLIST * pNext;
} MOUNTPOLICYLIST, *PMOUNTPOLICYLIST;

typedef enum
{
    AUTOFS = 0,
    AMD
} AutomountType;

typedef struct __MOUNTFILEINFO 
{
    PSTR pszFilename;
    BOOLEAN bIsExecutable;
    struct __MOUNTFILEINFO * pNext;
} MOUNTFILEINFO, *PMOUNTFILEINFO;

typedef struct __MOUNTINFO 
{
    AutomountType amtype;
    PMOUNTFILEINFO pMountFileInfoList;
    struct __MOUNTINFO * pNext;
} MOUNTINFO, * PMOUNTINFO;

typedef struct __MOUNTCONFIG 
{
    PMOUNTINFO pMountInfoList;
} MOUNTCONFIG, *PMOUNTCONFIG;


#endif /* __MOUNTEXT_H__ */

