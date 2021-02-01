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
 *        rsys-logging.h
 *
 * Abstract:
 *
 *        Reaper for syslog
 *        Logging interface
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */
#ifndef __RSYS_LOGGING_H__
#define __RSYS_LOGGING_H__

#include <lw/rtllog.h>

#define RSYS_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define REAPSYSL_FACILITY   "reapsysl"

#define RSYS_LOG_ALWAYS(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_ALWAYS, REAPSYSL_FACILITY, ## __VA_ARGS__)

#define RSYS_LOG_ERROR(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_ERROR, REAPSYSL_FACILITY, ## __VA_ARGS__)

#define RSYS_LOG_WARNING(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_WARNING, REAPSYSL_FACILITY, ## __VA_ARGS__)

#define RSYS_LOG_INFO(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_INFO, REAPSYSL_FACILITY, ## __VA_ARGS__)

#define RSYS_LOG_VERBOSE(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_VERBOSE, REAPSYSL_FACILITY, ## __VA_ARGS__)

#define RSYS_LOG_DEBUG(...) \
    LW_RTL_LOG_AT_LEVEL(LW_RTL_LOG_LEVEL_DEBUG, REAPSYSL_FACILITY, ## __VA_ARGS__)


#endif /* __RSYS_LOGGING_H__ */
