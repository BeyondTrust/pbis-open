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

#define AD_DEFAULT_SHELL            "/bin/sh"

#define AD_DEFAULT_UMASK            022

#define AD_DEFAULT_HOMEDIR_TEMPLATE "%H/local/%D/%U"

#define AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS   (0)
#define AD_CACHE_ENTRY_EXPIRY_DEFAULT_SECS   (4 * LSA_SECONDS_IN_HOUR)
#define AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS   (1 * LSA_SECONDS_IN_DAY)

#define AD_LOGIN_UPDATE_CACHE_ENTRY_SECS     (60)

#define AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS LSA_SECONDS_IN_HOUR
#define AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS (30 * LSA_SECONDS_IN_DAY)
#define AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS (60 * LSA_SECONDS_IN_DAY)

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

#define LSASS_DB_DIR CACHEDIR "/db"
#define LSASS_AD_SQLITE_CACHE_DB     LSASS_DB_DIR "/lsass-adcache.db"
#define LSASS_AD_MEMORY_CACHE_DB     LSASS_DB_DIR "/lsass-adcache.filedb"

#define LSASS_CACHE_PATH    ("FILE:" CACHEDIR "/krb5cc_lsass")

#endif /* __AD_DEF_H__ */

