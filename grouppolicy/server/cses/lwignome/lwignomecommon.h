#ifndef __LWIGNOMECOMMON_H__
#define __LWIGNOMECOMMON_H__

#define LWIGNOME_GCONF_PATH             "path"
#define LWIGNOME_GCONF_MANDATORY_PATH   "local-mandatory.path"
#define LWIGNOME_HOME_MAN_PATH          ".gconf.path.mandatory"
#define LWIGNOME_INCLUDE_IN_MAN_PATH    "include $(HOME)/.gconf.path.mandatory"
#define LWIGNOME_ADD_USR_GCONF_MANPATH  "xml:readonly:"

#define LWIGNOME_HOME_IDT               ".gconf.xml.mandatory" 
#define LWIGNOME_GLOBAL_MANDIR          "gconf.xml.mandatory" 

#define BUFFER_SIZE                     256
#define DIR_PERMS                       S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH


typedef struct {
    PSTR pszDirName;
    PSTR pszEntryName;
    PSTR pszMTime;
    PSTR pszType;
    PSTR pszLType;
    PSTR pszValue;
} GNOME_POLICY,*PGNOME_POLICY;

CENTERROR
UpdateMandatoryPathFile (
    PSTR szGconfDir,
    PGPUSER pUser
    );

CENTERROR
UpdatePathFileForMachine (
    PSTR szGconfDir
    );

BOOLEAN
IsValidGconfPath(
    PCSTR path
    );

CENTERROR
ApplyGnomePolicy(
    DWORD dwPolicyType,
    PSTR pszUser,
    PGPOLWIGPITEM pGPItem,
    PGNOME_POLICY pGnomeSettings
    );

CENTERROR
InsertMandatoryPath (
    PSTR pszFileName,
    PSTR pszPath
    );
#endif   //__LWIGNOMECOMMON_H__
