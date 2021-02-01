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
