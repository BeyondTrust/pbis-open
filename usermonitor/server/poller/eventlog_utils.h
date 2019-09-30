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
 *       eventlog_utils.h
 *
 * Abstract:
 *
 *        Event log client utilities
 */
#ifndef __EVENTLOG_UTILS_H__
#define __EVENTLOG_UTILS_H__

/**
 * @brief Return the fully qualified hostname in the 
 * supplied buffer.
 *
 * Returns the fully qualified hostname, truncated if
 * necessary to fit the supplied buffer.
 *
 * @param [in] pszFQDNHostname the destination buffer 
 * @param [in] len the size of the destination buffer
 *
 * @return errno error codes, specifically ENAMETOOLONG 
 * if the fully qualified hostname was truncated to fit
 */
DWORD
UmnEvtGetFQDN(
    char *pszFQDNHostname, 
    size_t len);

/**
 * @brief Set the event computer name that will be 
 * returned by UmnEvtGetEventComputerName().
 *
 * @param [in] pszEventComputerName the event computer name
 *
 * @return 0 on success, or an LW error code;
 * LW_ERROR_ERRNO_ENAMETOOLONG if the computer name was 
 * truncated to fit
 */
DWORD 
UmnEvtSetEventComputerName(
    const char *pszEventComputerName);

/**
 * @brief Return the event computer name, as set 
 * by UmnEvtSetEventComputerName().
 *
 * @return the event computer name
 */
PCWSTR 
UmnEvtGetEventComputerName();

/**
 * @brief Free the event computer name
 */
void 
UmnEvtFreeEventComputerName();

#endif

