#ifndef __LWIWINBINDMODE_H__
#define __LWIWINBINDMODE_H__

#define  LWIAUTHD_FILEPATH                      "/etc/samba/lwiauthd.conf"
#define  LWIAUTHD_FILEPATH_GP                   "/etc/samba/lwiauthd.conf.gp"
#define  LWIAUTHD_WINBIND_POLICY_FILEPATH       "/etc/samba/lwiauthd.policy.conf"
#define  LWIAUTHD_WINBIND_POLICY_FILEPATH_GP    "/etc/samba/lwiauthd.policy.conf.gp"

#if defined(__LWI_SOLARIS__)
#define LWI_SMB_CONF_FILE           "/etc/sfw/smb.conf"
#define LWI_SMB_CONF_FILE_GP        "/etc/sfw/smb.conf.gp"
#define LWI_SMB_CONF_OLD_FILE       "/etc/sfw/smb.conf.lwidentity.orig"
#define LWI_SMB_CONF_TMP_FILE       "/etc/sfw/smb.conf.tmp"
#else
#define LWI_SMB_CONF_FILE           "/etc/samba/smb.conf"
#define LWI_SMB_CONF_FILE_GP        "/etc/samba/smb.conf.gp"
#define LWI_SMB_CONF_OLD_FILE       "/etc/samba/smb.conf.lwidentity.orig"
#define LWI_SMB_CONF_TMP_FILE       "/etc/samba/smb.conf.tmp"
#endif

#define  LWIKRB5_FILEPATH                       "/etc/krb5.conf"
#define  LWIKRB5_FILEPATH_GP                    "/etc/krb5.conf.gp"
#define  LWIKRB5_BAK_FILEPATH                   "/etc/krb5.conf.lwidentity.bak"

CENTERROR
ProcessWinbindSettingsMode(
    PGPOLWIGPITEM pRsopGPAuthItem,
    BOOLEAN bGPModified
    );

CENTERROR
write_smb_conf_setting();

CENTERROR
ResetCenterisSettings(
    PSTR pszConfFileName,
    PSTR pszConfFilePath
    );

CENTERROR
ResetWinbindSettings();

CENTERROR
SendSIGHUP();

CENTERROR
write_setting(
    PSTR pszName,
    PSTR pszValue,
    PSTR pszType,
    FILE *fp
    );

CENTERROR
AddSettings (
    PSTR pszName,
    PSTR pszValue
    );

#endif
