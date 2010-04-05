/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        macadutil.h
 *
 * Abstract:
 *
 *       AD Utility (Public Header)
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LWADUTIL_H__
#define __LWADUTIL_H__

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <lw/types.h>
#include <lw/attrs.h>

#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2
#define UNKNOWN_GROUP_POLICY 3

/* 
 * Logging
 */
typedef enum
{
    LWUTIL_LOG_LEVEL_ALWAYS = 0,
    LWUTIL_LOG_LEVEL_ERROR,
    LWUTIL_LOG_LEVEL_WARNING,
    LWUTIL_LOG_LEVEL_INFO,
    LWUTIL_LOG_LEVEL_VERBOSE
} LWUtilLogLevel;

typedef VOID (*PFN_LWUTIL_LOG_MESSAGE)(
                    HANDLE      hLog,
                    LWUtilLogLevel logLevel,
                    PCSTR       pszFormat,
                    va_list     msgList
                    );


#ifndef GPO_DEFINED
#define GPO_DEFINED 1
typedef struct _GROUP_POLICY_OBJECT {
    DWORD dwOptions;    // GPLink options - do we care?
    DWORD dwVersion;    // Version - extract this from the gpt.ini
    PSTR pszPolicyDN;
    PSTR pszDSPath;
    PSTR pszDisplayName;
    PSTR pszgPCFileSysPath;
    PSTR pszgPCMachineExtensionNames;
    PSTR pszgPCUserExtensionNames;
    DWORD gPCFunctionalityVersion;
    DWORD dwFlags;
    BOOLEAN bNewVersion;
    struct _GROUP_POLICY_OBJECT *pNext;
} GROUP_POLICY_OBJECT, *PGROUP_POLICY_OBJECT;
#endif

#ifndef DSATTRIBUTEVALUE_DEFINED
#define DSATTRIBUTEVALUE_DEFINED 1

typedef struct __DSATTRIBUTEVALUE
{
    uint32_t valLen;
    char *   pszValue;
    struct __DSATTRIBUTEVALUE * pNext;
} DSATTRIBUTEVALUE, *PDSATTRIBUTEVALUE;

#endif /* DSATTRIBUTEVALUE_DEFINED */

#ifndef MCXVALUE_DEFINED
#define MCXVALUE_DEFINED 1

typedef struct __MCXVALUE
{
    char *   pValueData;
    int      iValLen;
    struct __MCXVALUE * pNext;
} MCXVALUE, *PMCXVALUE;

#endif /* MCXVALUE_DEFINED */

#ifndef LWUTIL_USER_ATTRIBUTES_DEFINED
#define LWUTIL_USER_ATTRIBUTES_DEFINED 1

typedef struct __LWUTIL_USER_ATTRIBUTES
{
    /* Name attributes */
    PSTR  pszDisplayName;
    PSTR  pszFirstName;
    PSTR  pszLastName;
    PSTR  pszADDomain;
    PSTR  pszKerberosPrincipal;

    /* Email attributes */
    PSTR  pszEMailAddress;
    PSTR  pszMSExchHomeServerName;
    PSTR  pszMSExchHomeMDB;

    /* Phone attributes */
    PSTR  pszTelephoneNumber;
    PSTR  pszFaxTelephoneNumber;
    PSTR  pszMobileTelephoneNumber;

    /* Address attributes */
    PSTR  pszStreetAddress;
    PSTR  pszPostOfficeBox;
    PSTR  pszCity;
    PSTR  pszState;
    PSTR  pszPostalCode;
    PSTR  pszCountry;

    /* Work attributes */
    PSTR  pszTitle;
    PSTR  pszCompany;
    PSTR  pszDepartment;

    /* Network setting attributes */
    PSTR  pszHomeDirectory;
    PSTR  pszHomeDrive;
    PSTR  pszPasswordLastSet;
    PSTR  pszUserAccountControl;
    PSTR  pszMaxMinutesUntilChangePassword;
    PSTR  pszMinMinutesUntilChangePassword;
    PSTR  pszMaxFailedLoginAttempts;
    PSTR  pszAllowedPasswordHistory;
    PSTR  pszMinCharsAllowedInPassword;

} LWUTIL_USER_ATTRIBUTES, *PLWUTIL_USER_ATTRIBUTES;

#endif /* LWUTIL_USER_ATTRIBUTES_DEFINED */

#endif /* __LWADUTIL_H__ */
