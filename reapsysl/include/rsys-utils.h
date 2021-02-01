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
 *        rsys-utils.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *
 *        System utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __RSYS_UTILS_H__
#define __RSYS_UTILS_H__

//defined flags in dwOptions
#define RSYS_CFG_OPTION_STRIP_SECTION          0x00000001
#define RSYS_CFG_OPTION_STRIP_NAME_VALUE_PAIR  0x00000002
#define RSYS_CFG_OPTION_STRIP_ALL (RSYS_CFG_OPTION_STRIP_SECTION |      \
                                     RSYS_CFG_OPTION_STRIP_NAME_VALUE_PAIR)

// This standardizes the width to 64 bits.  This is useful for
/// writing to files and such.

// This is in seconds (or milliseconds, microseconds, nanoseconds) since
// Jan 1, 1970.
typedef int64_t RSYS_UNIX_TIME_T, *PRSYS_UNIX_TIME_T;
typedef int64_t RSYS_UNIX_MS_TIME_T, *PRSYS_UNIX_MS_TIME_T;
typedef int64_t RSYS_UNIX_US_TIME_T, *PRSYS_UNIX_US_TIME_T;
typedef int64_t RSYS_UNIX_NS_TIME_T, *PRSYS_UNIX_NS_TIME_T;

// This is in 100ns units from Jan 1, 1601:
typedef int64_t RSYS_WINDOWS_TIME_T, *PRSYS_WINDOWS_TIME_T;

#endif /* __RSYS_UTILS_H__ */
