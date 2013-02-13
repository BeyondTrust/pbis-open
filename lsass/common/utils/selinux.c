/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "includes.h"

typedef char* security_context_t;
typedef struct __SELINUX
{
    void *dlhandle;
    int (*is_selinux_enabled)();
    int (*matchpathcon_init)(const char *path);
    void (*matchpathcon_fini)(void);
    int (*matchpathcon)(const char *path, mode_t mode, security_context_t *con);
    int (*setfilecon)(const char *path, security_context_t con);
    void (*freecon)(security_context_t con);

    BOOLEAN bEnabled;
} SELINUX;

DWORD
SELinuxCreate(
    PSELINUX *ppSELinux
    )
{
    DWORD dwError = 0;
    PSELINUX pSELinux = NULL;

    dwError = LwAllocateMemory(sizeof(SELINUX), (PVOID*)&pSELinux);
    BAIL_ON_LSA_ERROR(dwError);

    pSELinux->bEnabled = FALSE;

#if ENABLE_SELINUX
    pSELinux->dlhandle = dlopen(LIBSELINUX, RTLD_LAZY | RTLD_LOCAL);
    if (pSELinux->dlhandle == NULL)
    {
        LSA_LOG_ERROR("Could not load " LIBSELINUX ": %s", dlerror());
        goto cleanup;
    }
    else
    {
        pSELinux->is_selinux_enabled = dlsym(pSELinux->dlhandle, "is_selinux_enabled");
        pSELinux->matchpathcon_init = dlsym(pSELinux->dlhandle, "matchpathcon_init");
        pSELinux->matchpathcon_fini = dlsym(pSELinux->dlhandle, "matchpathcon_fini");
        pSELinux->matchpathcon = dlsym(pSELinux->dlhandle, "matchpathcon");
        pSELinux->setfilecon= dlsym(pSELinux->dlhandle, "setfilecon");
        pSELinux->freecon = dlsym(pSELinux->dlhandle, "freecon");
        if (!pSELinux->is_selinux_enabled ||
            !pSELinux->matchpathcon_init ||
            !pSELinux->matchpathcon_fini ||
            !pSELinux->matchpathcon ||
            !pSELinux->setfilecon ||
            !pSELinux->freecon)
        {
            LSA_LOG_ERROR("Could not find symbol in " LIBSELINUX);
            dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pSELinux->is_selinux_enabled() == 1)
        {
            LSA_LOG_DEBUG("SELinux is enabled.");
            pSELinux->matchpathcon_init(NULL);
            pSELinux->bEnabled = TRUE;
        }
    }
#endif
    *ppSELinux = pSELinux;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSELinux);
    goto cleanup;
}

DWORD
SELinuxSetContext(
    PCSTR pszPath,
    mode_t mode,
    PSELINUX pSELinux
    )
{
    DWORD dwError = 0;

#if ENABLE_SELINUX
    security_context_t context;

    if ((pSELinux && pSELinux->bEnabled))
    {
        if (pSELinux->matchpathcon(pszPath, mode, &context))
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            if (pSELinux->setfilecon(pszPath, context) == -1)
            {
               dwError = LwMapErrnoToLwError(errno);
            }
            pSELinux->freecon(context);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
#else
    return dwError;
#endif
}


VOID
SELinuxFree(
    PSELINUX pSELinux
    )
{
    if (pSELinux)
    {
#if ENABLE_SELINUX
        if (pSELinux->bEnabled)
        {
            pSELinux->matchpathcon_fini();
        }
        if (pSELinux->dlhandle)
            dlclose(pSELinux->dlhandle);
#endif
        LW_SAFE_FREE_MEMORY(pSELinux);
    }
}

DWORD
LsaSELinuxManageHomeDir(
    PCSTR pszHomeDir
    )
{
    DWORD dwError = 0;
    PCSTR pszSemanageFormat = "semanage fcontext -a -e /home %s";
    PSTR pszRootHomeDir = NULL;
    PSTR pszSemanageExecute = NULL;
    int systemresult = 0;

    dwError = LsaGetDirectoryFromPath(pszHomeDir, &pszRootHomeDir);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszRootHomeDir))
    {
        dwError = LW_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszRootHomeDir[0] != '/')
    {
        dwError = LW_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszRootHomeDir[1] == '\0')
    {
        dwError = LW_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszSemanageExecute,
                    pszSemanageFormat,
                    pszRootHomeDir);
    BAIL_ON_LSA_ERROR(dwError);

    systemresult = system(pszSemanageExecute);
    if (systemresult < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszRootHomeDir);
    LW_SAFE_FREE_STRING(pszSemanageExecute);

    return dwError;

error:
    goto cleanup;
}

