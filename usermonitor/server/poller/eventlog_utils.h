/*
 * Copyright BeyondTrust Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU LESSER GENERAL
 * PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR
 * WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY
 * BEYONDTRUST, PLEASE CONTACT BEYONDTRUST AT beyondtrust.com/contact
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

