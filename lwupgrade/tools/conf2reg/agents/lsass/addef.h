/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        addef.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Private header for Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __AD_DEF_H__
#define __AD_DEF_H__

#define AD_CFG_TAG_AD_PROVIDER    "lsa-activedirectory-provider"
#define AD_CFG_TAG_AUTH_PROVIDER  "auth provider:"

#define AD_BUILTIN_GROUP_SID_PREFIX "S-1-5-32-"

#define AD_DEFAULT_SHELL            "/bin/sh"

#define AD_DEFAULT_UMASK            "022"

#if defined(__LWI_DARWIN__)

#define AD_DEFAULT_HOMEDIR_PREFIX "/Users"

#elif defined(__LWI_SOLARIS__)

#define AD_DEFAULT_HOMEDIR_PREFIX "/export/home"

#else

#define AD_DEFAULT_HOMEDIR_PREFIX "/home"
#endif

#if defined(__LWI_DARWIN__)
    // ISSUE-2008/10/03-dalmeida -- The only difference between this and the
    // createhomedir utility is that the latter uses locale info and sets
    // "AppleLocale" in ~/Library/Preferences/.GlobalPreferences.plist
    // The latter is done by adding at the end of the <dict> section:
    //
    //   <key>AppleLocale</key>
    //   <string>en_US</string>
    //
    // To determine the right thing to do wrt locale, verify createhomedir
    // behavior wrt locale.
    //
    // Note that we must do the non-localized first then the localized to
    // make sure that the localized files overwrite non-localized ones.
    // This actually comes into play, at least on Mac OS X 10.5.
    // Note that this does not handle ACLs (ls -le), which are normally
    // present, at least on Mac OS X 10.5.  But the default ACLs do
    // not appear to be useful.

#define AD_DEFAULT_SKELDIRS         "System/Library/User Template/Non_localized, /System/Library/User Template/English.lproj"

#else

#define AD_DEFAULT_SKELDIRS         "/etc/skel"

#endif

#define AD_DEFAULT_HOMEDIR_TEMPLATE "%H/%D/%U"

#define AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS (5 * UP_SECONDS_IN_MINUTE)
#define AD_CACHE_REAPER_TIMEOUT_DEFAULT_SECS (30 * UP_SECONDS_IN_DAY)
#define AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS (60 * UP_SECONDS_IN_DAY)

#define AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS   (0)
#define AD_CACHE_ENTRY_EXPIRY_DEFAULT_SECS   (4 * UP_SECONDS_IN_HOUR)
#define AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS   (1 * UP_SECONDS_IN_DAY)

#define AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS UP_SECONDS_IN_HOUR
#define AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS (30 * UP_SECONDS_IN_DAY)
#define AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS (60 * UP_SECONDS_IN_DAY)

#define AD_SPACE_REPLACEMENT_DEFAULT         '^'

#define AD_MAX_ALLOWED_CLOCK_DRIFT_SECONDS 60

#define AD_STR_IS_SID(str) \
    (!LW_IS_NULL_OR_EMPTY_STR(str) && !strncasecmp(str, "s-", sizeof("s-")-1))

typedef enum
{
    SchemaMode = 0,
    NonSchemaMode = 1,
    UnknownMode = 2
} ADConfigurationMode;

#define DEFAULT_MODE		1
#define CELL_MODE		2
#define UNPROVISIONED_MODE	3

typedef enum
{
    AD_CELL_SUPPORT_UNINITIALIZED = 0, //not used
    AD_CELL_SUPPORT_FULL          = 1, //default
    AD_CELL_SUPPORT_FILE          = 2, // unused
    AD_CELL_SUPPORT_UNPROVISIONED = 3,
    AD_CELL_SUPPORT_DEFAULT_SCHEMA = 4,
} AD_CELL_SUPPORT;

typedef enum
{
    AD_CACHE_UNINITIALIZED = 0, //not used
    AD_CACHE_SQLITE        = 1,
    AD_CACHE_IN_MEMORY     = 2,
} AD_CACHE_BACKEND;

#endif /* __AD_DEF_H__ */

