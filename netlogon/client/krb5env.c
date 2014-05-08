/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        krb5env.c
 *
 * Abstract:
 * 
 *        Likewise Site Manager
 * 
 *        Kerberos 5 Configuration Environment API
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */
#include "includes.h"

#define LWNET_KRB5_CONFIG_VARIABLE_NAME "KRB5_CONFIG"
#define LWNET_KRB5_BASE_CONF_PATH LWNET_CONFIG_DIR "/pbis-krb5-ad.conf"

#define LWNET_KRB5_DEFAULT_CONF_PATH "/etc/krb5.conf"

#ifdef MINIMAL_NETLOGON
// In a minimal build, the system krb5.conf is used instead of the likewise
// conf file. This saves the space of installing a second file, but the system
// krb5.conf must be fully configured.
#define LWNET_KRB5_ENV_PREFIX \
    LWNET_KRB5_CONF_PATH ":" LWNET_KRB5_DEFAULT_CONF_PATH
#else
#define LWNET_KRB5_ENV_PREFIX \
    LWNET_KRB5_CONF_PATH ":" LWNET_KRB5_BASE_CONF_PATH
#endif

#define LWNET_KRB5_ENV_PREFIX_LENGTH \
    (sizeof(LWNET_KRB5_ENV_PREFIX)-1)

LWNET_API
DWORD
LWNetExtendEnvironmentForKrb5Affinity(
    BOOLEAN bNoDefault
    )
{
    // NOTE: This intentionally leaks memory.  However, this function
    // tries to make sure that it only leaks once if called multiple
    // times.  The invariant is that it puts the appropriate prefix
    // to the file path in the KRB5_CONFIG environment variable,
    // not putting anything there if the correct prefix
    // already there.  Note that we still add a prefix if the "prefix"
    // is present, but is in the middle of the variable's value instead
    // of being at the start.

    DWORD dwError = 0;
    PCSTR pszEnvironmentValue = NULL;
    PSTR pszPutenvSetting = NULL;

    pszEnvironmentValue = getenv(LWNET_KRB5_CONFIG_VARIABLE_NAME);
    if (IsNullOrEmptyString(pszEnvironmentValue))
    {
        // If the previous value was empty, the behavior is to open /etc/krb5.conf.
        // To preserve this behavior, /etc/krb5.conf must be added expicitly.
        pszEnvironmentValue = LWNET_KRB5_DEFAULT_CONF_PATH;
    }
    else if (!strncmp(LWNET_KRB5_ENV_PREFIX, pszEnvironmentValue, LWNET_KRB5_ENV_PREFIX_LENGTH) &&
             (!pszEnvironmentValue[LWNET_KRB5_ENV_PREFIX_LENGTH] ||
              pszEnvironmentValue[LWNET_KRB5_ENV_PREFIX_LENGTH] == ':'))
    {
        // If we are in front, there is nothing to do.

        // Note that we do not care if we found the string in the middle.  We will just
        // prepend stuff anyhow.

        dwError = 0;
        goto error;
    }

    // The first setting takes precedence, so we want our overrides
    // to show up first.

    if (bNoDefault)
    {
        dwError = LwAllocateStringPrintf(&pszPutenvSetting,
                                            "%s=%s",
                                            LWNET_KRB5_CONFIG_VARIABLE_NAME,
                                            LWNET_KRB5_ENV_PREFIX);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(&pszPutenvSetting,
                                            "%s=%s:%s",
                                            LWNET_KRB5_CONFIG_VARIABLE_NAME,
                                            LWNET_KRB5_ENV_PREFIX,
                                            pszEnvironmentValue);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = putenv(pszPutenvSetting);
    if(dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    // Explicitly leak if we successfully called putenv.
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszPutenvSetting);
    }
    return dwError;
}

