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
 *        lwsmsyslog.h
 *
 * Abstract:
 *
 *        Support for syslog based loggers
 */
#ifndef __LWSM_SYSLOG_H__
#define __LWSM_SYSLOG_H__

/* maps syslog facility name, to syslog facility code */
typedef const struct _lw_sm_syslog_fac {
    const PCSTR name;
    const int facility;
    const PCSTR description;
} LW_SM_SYSLOG_FACILITY, *PLW_SM_SYSLOG_FACILITY;


/**
 * @brief Return the syslog facility entry matching (case insensitive) the supplied name
 * @param facilityName syslog facility name, e.g. auth, daemon
 * @return the matching syslog facility entry or NULL
 */
PLW_SM_SYSLOG_FACILITY
LwSmGetSyslogFacilityByName(
    PCSTR facilityName
    );

/**
 * @brief Return the syslog facility entry matching the supplied facilty value
 * @param value syslog facility value, e.g. LOG_AUTH, LOG_DAEMON
 * @return the matching syslog facility entry or NULL
 */
PLW_SM_SYSLOG_FACILITY
LwSmGetSyslogFacilityByValue(
    const int facilityValue
    );

#endif
