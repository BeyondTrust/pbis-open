/*
 *  LWIStruct.h
 *  LWIDSPlugIn
 *
 *  Created by Sriram Nambakam on 5/25/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __LWISTRUCT_H__
#define __LWISTRUCT_H__

#include "macadutil.h"

#define LWDS_GPO_CACHE_DIR         "/var/lib/likewise/grouppolicy"
#define LWDS_USER_MCX_CSE_GUID     "{07E500C4-20FD-4829-8F38-B5FF63FA0493}"
#define LWDS_COMPUTER_MCX_CSE_GUID "{B9BF896E-F9EB-49b5-8E67-11E2EDAED06C}"

//#define LWI_UUID_UID "315F6FA0-4CCB-42AC-8CA8-A1126E0FA7AE"
#define LWI_UUID_UID_PREFIX "315F6FA0-4CCB-42AC-8CA8-A112"
//#define LWI_UUID_GID "9B5F5F9B-660D-4F41-A791-795FF6B5352A"
#define LWI_UUID_GID_PREFIX "9B5F5F9B-660D-4F41-A791-795F"
#define LWI_GUID_LENGTH 36

#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2
#define UNKNOWN_GROUP_POLICY 3

typedef struct __DSDIRNODE
{
    char* pszDirNodePath;
    char* pszDirNodeUserUPN;
    bool  fPlugInRootConnection;
    PGROUP_POLICY_OBJECT pDirNodeGPO;
} DSDIRNODE, *PDSDIRNODE;

typedef struct __DSATTRIBUTE
{
    uint16_t nameLen;
    char *            pszName;
    PDSATTRIBUTEVALUE pValueListHead;
    PDSATTRIBUTEVALUE pValueListTail;
    struct __DSATTRIBUTE * pNext;
} DSATTRIBUTE, *PDSATTRIBUTE;

typedef struct __DSRECORD
{
    uint16_t     typeLen;
    char *       pszType;
    uint16_t     nameLen;
    char *       pszName;
    long         dirNodeRef;
    PDSATTRIBUTE pAttributeListHead;
    PDSATTRIBUTE pAttributeListTail;
    BOOLEAN      fDirty;
    struct __DSRECORD * pNext;
} DSRECORD, *PDSRECORD;

typedef struct __DSMESSAGEHEADER
{
  uint32_t  startTag; // StdA, StdB, Gdni, DbgA, DbgB
  uint32_t  nRecords; // Number of Records in buffer
  uint32_t* pOffsets; // Offset to each Record in buffer
  uint32_t  endTag;   // EndT
} DSMESSAGEHEADER, *PDSMESSAGEHEADER;

typedef struct __DSMESSAGE
{
    PDSMESSAGEHEADER pHeader;
    PDSRECORD        pRecordList;
} DSMESSAGE, *PDSMESSAGE;

typedef struct __LWIUSER
{
    char*  pw_name;
    char*  pw_display_name;
    char*  pw_passwd;
    uid_t  pw_uid;
    gid_t  pw_gid;
    time_t pw_change;
    char*  pw_class;
    char*  pw_gecos;
    char*  pw_nfs_home_dir;
    char*  pw_home_dir;
    char*  pw_orig_nfs_home_dir;
    char*  pw_orig_home_dir;
    char*  pw_shell;
    time_t pw_expire;
    PMCXVALUE pMCXValues;
    PAD_USER_ATTRIBUTES padUserInfo;
} LWIUSER, *PLWIUSER;

typedef struct __LWIGROUP
{
    char*  gr_name;
    char*  gr_passwd;
    char*  guid;
    gid_t  gr_gid;
    char** gr_membership; /* Members list by name */
    char** gr_members;    /* Members list by GUID */
    char*  shortname;
    char*  comment;
    PMCXVALUE pMCXValues;
} LWIGROUP, *PLWIGROUP;

typedef struct __LWICOMPUTER
{
    char*  name;
    char*  shortname;
    char*  comment;
    char** keywords;
    char*  ethernetID;
    char*  IPaddress;
    char** URLs;
    char*  guid;
    PMCXVALUE pMCXValues;
} LWICOMPUTER, *PLWICOMPUTER;

typedef struct __LWICOMPUTERGROUP
{
    char*  name;
    char*  shortname;
    char*  comment;
    char** computers;
    char*  guid;
    int    primaryId;
    char** membership;
    char** members;
    PMCXVALUE pMCXValues;
} LWICOMPUTERGROUP, *PLWICOMPUTERGROUP;

typedef struct __LWICOMPUTERLIST
{
    char*  name;
    char*  shortname;
    char*  comment;
    char** computers;
    char*  guid;
    int    primaryId;
    PMCXVALUE pMCXValues;
} LWICOMPUTERLIST, *PLWICOMPUTERLIST;

typedef struct __LWIBITVECTOR
{
    uint8_t* data;
    uint32_t nBits;
} LWIBITVECTOR, * PLWIBITVECTOR;

typedef struct __LWIMEMBERLIST
{
    PSTR   pszName;
    PSTR   pszUPN;
    uid_t  uid;
    struct __LWIMEMBERLIST * pNext;
} LWIMEMBERLIST, *PLWIMEMBERLIST;

typedef long MACERROR;

#define LWE_DS_FLAG_NO_OPTIONS_SET                               0x00000000
#define LWE_DS_FLAG_IS_LEOPARD                                   0x00000001
#define LWE_DS_FLAG_MERGE_MODE_MCX                               0x00000002
#define LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_SMB             0x00000004
#define LWE_DS_FLAG_USE_AD_UNC_FOR_HOME_LOCATION_AFP             0x00000008
#define LWE_DS_FLAG_FORCE_LOCAL_HOME_DIRECTORY_ON_STARTUP_DISK   0x00000010
#define LWE_DS_FLAG_DONT_REMOVE_LOCAL_ADMINS                     0x00000020
#define LWE_DS_FLAG_IS_SNOW_LEOPARD                              0x00000040
typedef DWORD LWE_DS_FLAGS;


#endif /* __LWISTRUCT_H__ */
