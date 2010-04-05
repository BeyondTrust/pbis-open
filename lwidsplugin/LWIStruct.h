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
    char*  pw_passwd;
    uid_t  pw_uid;
    gid_t  pw_gid;
    time_t pw_change;
    char*  pw_class;
    char*  pw_gecos;
    char*  pw_dir;
    char*  pw_shell;
    time_t pw_expire;
} LWIUSER, *PLWIUSER;

typedef struct __LWIGROUP
{
    char*  gr_name;
    char*  gr_passwd;
    gid_t  gr_gid;
    char** gr_mem;
} LWIGROUP, *PLWIGROUP;

typedef struct __LWIBITVECTOR
{
    uint8_t* data;
    uint32_t nBits;
} LWIBITVECTOR, * PLWIBITVECTOR;

typedef long MACERROR;

#endif /* __LWISTRUCT_H__ */
