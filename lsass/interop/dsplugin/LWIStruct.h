/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __LWISTRUCT_H__
#define __LWISTRUCT_H__

#include "includes.h"

#define LWDS_GPO_CACHE_DIR         "/var/lib/likewise/grouppolicy"
#define LWDS_USER_MCX_CSE_GUID     "{07E500C4-20FD-4829-8F38-B5FF63FA0493}"
#define LWDS_COMPUTER_MCX_CSE_GUID "{B9BF896E-F9EB-49b5-8E67-11E2EDAED06C}"

//#define LWI_UUID_UID             "315F6FA0-4CCB-42AC-8CA8-A1126E0FA7AE"
#define LWI_UUID_UID_PREFIX        "315F6FA0-4CCB-42AC-8CA8-A112"
//#define LWI_UUID_GID             "9B5F5F9B-660D-4F41-A791-795FF6B5352A"
#define LWI_UUID_GID_PREFIX        "9B5F5F9B-660D-4F41-A791-795F"
#define LWI_GUID_LENGTH            36

#define MACHINE_GROUP_POLICY       1
#define USER_GROUP_POLICY          2
#define UNKNOWN_GROUP_POLICY       3

#define COMPUTER_COMMENT_GPO       "This computer is automatically created to represent the computers that are implicitly defined by the current Group Policy Object (GPO) context. Use the computer group \"Group of Computers managed by GPO\" to store Macintosh computer system settings with Workgroup Manager. These settings will be applied to all computers that are influenced by this GPO."
#define COMPUTER_COMMENT_LOCAL     "The local Macintosh computer"
#define COMPUTER_GROUP_COMMENT     "This computer group is automatically created to represent the computers that are implicitly defined by the current Group Policy Object (GPO) context. Use this group to store Macintosh computer system settings with Workgroup Manager. These settings will be applied to all computers that are influenced by this GPO."
#define COMPUTER_LIST_COMMENT      "This computer list is automatically created to represent the computers that are implicitly defined by the current Group Policy Object (GPO) context. Use this list to store Macintosh computer system settings with Workgroup Manager. These settings will be applied to all computers that are influenced by this GPO."
#define USER_GROUP_COMMENT         "This user group is automatically created to represent the users that are implicitly defined by the current Group Policy Object (GPO) context. Use this group to store Macintosh user settings with Workgroup Manager. These settings will be applied to all users that are influenced by this GPO."
#define COMPUTER_ID_GPO            "FF:FF:FF:FF:FF:FF"
#define COMPUTER_ID_LOCAL          "127.0.0.1"
#define COMPUTER_MAC_GPO           "FF:FF:FF:FF:FF:FF"
#define COMPUTER_LOOPBACK_IP       "127.0.0.1"
#define UNSET_GID_UID_ID           65535
#define COMPUTER_GROUP_UID         51567
#define COMPUTER_GROUP_UID_ID      "51567"
#define COMPUTER_LIST_UID          40302
#define COMPUTER_LIST_UID_ID       "40302"
#define USER_NAME_GPO              "gpouser"
#define USER_UID_GPO               92970
#define USER_UID_GPO_ID            "92970"
#define GROUP_GID_GPO              22200
#define GROUP_GID_GPO_ID           "22200"
#define MAX_DS_CONTINUE_HANDLE     670000
#define SPECIAL_DS_CONTINUE_HANDLE 676767


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
    char*  pw_name_as_queried;
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
    char*  gr_name_as_queried;
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
