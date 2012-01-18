/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        auth.h
 *
 * Abstract:
 *
 *        Authentication-related methods.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 12, 2010
 *
 */

#ifndef _AD_TOOL_AUTH_H_
#define _AD_TOOL_AUTH_H_

/**
 * Get and cache TGT using the provided UPN and password.
 *
 * @param userPrincipal User principal name.
 * @param password Password.
 * @param cachePath Path to the cache file.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetTgtFromNamePassword(PCSTR userPrincipal,
                                    PCSTR password,
                                    PCSTR cachePath,
                                    PDWORD pdwGoodUntilTime);

/**
 * Get and cache TGT using the keytab file. Keytab file path can be
 * set via KRB5_KTNAME environment variable.
 *
 * @param userPrincipal User principal name.
 * @param cachePath Path to the cache file.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetTgtFromKeytab(PCSTR userPrincipal,
                              PCSTR cachePath,
                              PDWORD pdwGoodUntilTime);


#endif /* _AD_TOOL_AUTH_H_ */
