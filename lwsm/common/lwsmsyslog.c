/*
 * Copyright (c) BeyondTrust Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * BEYONDTRUST SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE OR BEYONDTRUST SOFTWARE, THEN YOU MAY ELECT TO USE
 * THE SOFTWARE UNDER THE TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF
 * THE TERMS OF THE GNU GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE
 * NOTICE. IF YOU HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE
 * LICENSING TERMS OFFERED BY BEYONDTRUST SOFTWARE, PLEASE CONTACT BEYONDTRUST
 * SOFTWARE AT beyondtrust.com/contact
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
