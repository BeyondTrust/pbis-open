#ifndef LWIAUTHDXML_H
#define LWIAUTHDXML_H

#define GPITEM_GUID_LWIAUTHD        "{804A1822-40E7-45B2-A549-EEB3EF545339}"
#define GPITEM_GUID_LWILOGON        "{FAC763F0-9CC0-4ECE-8188-287D3E128AC3}"

#define LWI_TAG_AUTHD               "lwiauthd"
#define LWI_TAG_LOGON	            "pam"

#define LWI_TAG_SECTION             "section"
#define LWI_TAG_COMMENT             "comment"
#define LWI_TAG_SETTING             "setting"
#define LWI_TAG_LINE                "line"
#define LWI_ATTR_NAME               "name"
#define LWI_ATTR_TYPE               "type"

/* valid types */
#define LWI_ATTR_TYPE_STRING        "string"
#define LWI_ATTR_TYPE_BOOL          "bool"

/* valid file types */
#define LWI_KRB5_CONF_FILE          "/etc/krb5.conf"
#define LWI_KRB5_CONF_FILE_GP       "/etc/krb5.conf.gp"
#define LWI_KRB5_CONF_OLD_FILE      "/etc/krb5.conf.lwidentity-gp.orig"

#define LWI_CLOCKSKEW               "clockskew"
#define LWI_SERVER_SIGN             "server signing"
#define LWI_NULL_PASSWD             "null passwords"
#define LWI_NAME_CACHE_TIMEOUT      "name cache timeout"

#define LWI_SOLARIS_SMBD_PATH_LOCAL "/usr/local/samba/sbin/smbd"
#define LWI_SOLARIS_SMBD_PATH_SFW   "/usr/sfw/sbin/smbd"
#define LWI_SOLARIS_SMBD_PATH_CSW   "/opt/csw/sbin/smbd"

#define STATIC_PATH_BUFFER_SIZE     256

typedef struct _SETTING_ {
    PSTR pszSectionName;
    PSTR pszName;
    PSTR pszValue;
    struct _SETTING_ *pNext;
} AD_SETTING, *PAD_SETTING;

CENTERROR
GPOLwiReadItem(
    PSTR pszFilePath,
    PGPOLWIGPITEM pGPItem,
    PSTR pszXmlTag
    );
    
CENTERROR
GPOLwiWriteItem(
    PSTR pszFilePath,
    PSTR pszFileHeader,
    PGPOLWIGPITEM pGPItem
    );

BOOLEAN
IsComment(
    PCSTR pszBuf
    );

CENTERROR
ComputeFileTimeStamps(
    PSTR pszBaseFile,
    PSTR pszConfFilePath,
    BOOLEAN *pbGPModified
    );

VOID
FreeADSetting();

CENTERROR
write_krb5_setting(
    PSTR pszName,
    PSTR pszValue
    );

#endif
