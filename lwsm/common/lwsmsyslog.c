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
 * Module Name:
 *
 *        lwsmsyslog.c
 *
 * Abstract:
 *
 *        Support for syslog based loggers
 */
#include <syslog.h>
#include "includes.h"

/* supported syslog facilities; this excludes internal and deprecated codes */
LW_SM_SYSLOG_FACILITY facilityCodes[] =
{
    {"auth", LOG_AUTH, "LOG_AUTH"},
#if HAVE_LOG_AUTHPRIV
    {"authpriv", LOG_AUTHPRIV, "LOG_AUTHPRIV"},
#endif
    {"cron", LOG_CRON, "LOG_CRON"},
    {"daemon",LOG_DAEMON, "LOG_DAEMON"},
#if HAVE_LOG_FTP
    {"ftp", LOG_FTP, "LOG_FTP"},
#endif
    {"lpr", LOG_LPR, "LOG_LPR"},
    {"mail", LOG_MAIL, "LOG_MAIL"},
    {"news", LOG_NEWS, "LOG_NEWS"},
    {"user", LOG_USER, "LOG_USER"},
    {"uucp", LOG_UUCP, "LOG_UUCP"},
    {"local0", LOG_LOCAL0, "LOG_LOCAL0"},
    {"local1", LOG_LOCAL1, "LOG_LOCAL1"},
    {"local2", LOG_LOCAL2, "LOG_LOCAL2"},
    {"local3", LOG_LOCAL3, "LOG_LOCAL3"},
    {"local4", LOG_LOCAL4, "LOG_LOCAL4"},
    {"local5", LOG_LOCAL5, "LOG_LOCAL5"},
    {"local6", LOG_LOCAL6, "LOG_LOCAL6"},
    {"local7", LOG_LOCAL7, "LOG_LOCAL7"},
    {NULL, -1, NULL}
};

PLW_SM_SYSLOG_FACILITY
LwSmGetSyslogFacilityByName(
    PCSTR facilityName
    )
{
    PLW_SM_SYSLOG_FACILITY entry = &facilityCodes[0];

    if (facilityName)
    {
        while (entry->name)
        {
            if (!strcasecmp(entry->name, facilityName))
            {
                return entry;
            }

            entry++;
        }
    }

    return NULL;
}

PLW_SM_SYSLOG_FACILITY
LwSmGetSyslogFacilityByValue(
    const int facilityValue
    )
{
    PLW_SM_SYSLOG_FACILITY entry = &facilityCodes[0];

    while (entry->name)
    {
        if (entry->facility == facilityValue)
        {
            return entry;
        }

        entry++;
    }

    return NULL;
}
