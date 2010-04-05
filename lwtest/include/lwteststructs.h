#ifndef LWTSTRUCTS_H
#define LWTSTRUCTS_H

typedef struct _LWTCSV
{
    size_t nFields;
    char **ppszFieldNames;

    size_t nRows;
    size_t nRowsBeginOffset;

    int fd;
} LWTCSV, *PLWTCSV;


/*PLDIF file contaxt here */
typedef struct _LWTLDIF
{
    int fd;
    size_t tPosition;
    size_t nMaxEntries;
    size_t nFields;
}LWTLDIF, *PLWTLDIF;



typedef struct _LWT_DATA
{
    PSTR *ppszFieldName;
    PSTR *ppszFieldValue;
    int nFields;
}LWTDATA, *PLWTDATA;



typedef DWORD (*Lwt_DataInit) (PCSTR pszFileName, PVOID *ppContext);
typedef DWORD (*Lwt_DataGetNext)(PVOID pContext, size_t tindex, PLWTDATA *ppData);
typedef DWORD (*Lwt_ContextFree)(PVOID pContext);
typedef DWORD (*Lwt_Reset)(PVOID pContext);
typedef DWORD (*Lwt_GetMaxEntries)(PVOID pvContext);
/* Gemeral LWT DATA UTILITY STRUCT */
typedef struct _LWTDATAINTERFACE
{
    PVOID pvDataContext;
    Lwt_DataInit Init;
    Lwt_DataGetNext GetNext;
    Lwt_GetMaxEntries GetMaxEntries;
    Lwt_ContextFree ContextFree;
    Lwt_Reset Reset;
    
}LWTDATAIFACE, *PLWTDATAIFACE;



typedef struct _LWTUSER
{
    PSTR pszNTName;
    PSTR pszSamAccountName;
    PSTR pszAlias;
    PSTR pszSid;
    PSTR pszUserPrincipalName;
    PSTR pszFQDN;
    PSTR pszNetBiosName;
    PSTR pszPassword;
    DWORD dwEnabled;
    DWORD dwPasswordExpired;
    DWORD dwAccountExpired;
    DWORD dwUserLockedOut;

    /** Unix Specific **/
    PSTR pszUnixUid;
    uid_t nUnixUid;

    PSTR pszUnixGid;
    gid_t nUnixGid;

    gid_t nUserGid;    
    PSTR pszUnixGecos;
    PSTR pszUnixLoginShell;
    PSTR pszOriginalUnixHomeDirectory; /* Came from LDIF,CSV,etc... */
    PSTR pszUnixHomeDirectory;         /* Calculated from pszOriginalUnixHomeDirectory. */

} LWTUSER, *PLWTUSER;

typedef struct _LWTGROUP
{
    gid_t nGid;
    PSTR pszName;
    PSTR pszAlias;
    PSTR pszSid;
    PSTR pszDN;
    PSTR pszPassword;
    PSTR* ppszMembers;
    PSTR pszObjectGuid;
    PSTR pszSamAccountName;
    PSTR pszNetBiosName;
    DWORD dwObjectGuid;
    DWORD dwGroupInfoLevel;
    DWORD dwNumGroups;
} LWTGROUP, *PLWTGROUP;


typedef struct _TESTDATA
{
    DWORD dwNumUsers;
    DWORD dwNumGroups;
    DWORD dwNumInvalidDataSet;

    PLWTDATAIFACE pUserIface;
    PLWTDATAIFACE pGroupIface;
    PLWTDATAIFACE pInvalidDataIface;

} TESTDATA, *PTESTDATA;



/*
* Invalid Field Type 
* This keyword identifies the invalid key type to be fetched for the 
* invalid data
*/
typedef enum _LWTFIELDTYPE
{
    LWTUSER_INVALID,
    LWTGROUP_INVALID,
    LWTUID_INVALID,
    LWTGID_INVALID,
    LWTSID_INVALID,
    LWTUSERINFOLEVEL_INVALID,
    LWTGROUPINFOLEVEL_INVALID,
    LWTPASSWORD_INVALID,
    LWTMAXUSER_INVALID,
    LWTMAXGROUPS_INVALID
}LWTFIELDTYPE;



/*
* Invalid Failed Data records
* Some fields in this record may be empty depending on test cases
*/
typedef struct _LWTFAILDATA
{
    DWORD dwNumRecords;
    LWTFIELDTYPE Field;     /* Invalid Field Type */
    PSTR pszUserName;       /* UserName */
    PSTR pszGroupName;      /* GroupName */
    uid_t nUid;             /* Uid Paremeters */
    gid_t nGid;             /* Gid parameters */
    PSTR pszSid;            /* Security Identifier */
    DWORD dwLevel;          /* Invalid User or Group Info Levels based on the invalid test*/
    DWORD dwMaxEntries;     /* Invalid Max Enteries for User and Group*/
    DWORD dwErrorCode;      /* Expected error code */
    PSTR pszPassword;       /* Password field */
}LWTFAILDATA, *PLWTFAILDATA;


#endif
