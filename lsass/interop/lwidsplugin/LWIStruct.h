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

#include "includes.h"


//#define LWI_UUID_UID "315F6FA0-4CCB-42AC-8CA8-A1126E0FA7AE"
#define LWI_UUID_UID_PREFIX "315F6FA0-4CCB-42AC-8CA8-A112"
//#define LWI_UUID_GID "9B5F5F9B-660D-4F41-A791-795FF6B5352A"
#define LWI_UUID_GID_PREFIX "9B5F5F9B-660D-4F41-A791-795F"
#define LWI_GUID_LENGTH            36
#define UNSET_GID_UID_ID           65535
#define MAX_DS_CONTINUE_HANDLE     670000
#define SPECIAL_DS_CONTINUE_HANDLE 676767

typedef struct __DSATTRIBUTEVALUE
{
    uint32_t valLen;
    char *   pszValue;
    struct __DSATTRIBUTEVALUE * pNext;
} DSATTRIBUTEVALUE, *PDSATTRIBUTEVALUE;

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
    PDSATTRIBUTE pAttributeListHead;
    PDSATTRIBUTE pAttributeListTail;
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
} LWIGROUP, *PLWIGROUP;

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

#endif /* __LWISTRUCT_H__ */
