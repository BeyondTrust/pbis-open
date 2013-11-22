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
 *        pam-notify.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Logon Notify API
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Glenn Curtis (glennc@likewise.com)
 */
#include "pam-lsass.h"

/* Define signatures for the entry points that are used from libgpapi.so */
#define GPAPI_DLL_NAME   "libgpapi.so"

typedef void (*gp_pam_msg_cb_t)(void *context, int is_err, char *format, ...);

typedef int (*PFN_GP_PAM_PROCESS_LOGIN)(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

typedef int (*PFN_GP_PAM_PROCESS_LOGOUT)(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

/* Local function pointer wrappers to libgpapi.so exports */
static PFN_GP_PAM_PROCESS_LOGIN  gpfnGPPamProcessLogin = NULL;
static PFN_GP_PAM_PROCESS_LOGOUT gpfnGPPamProcessLogout = NULL;

static void *                    gpGPLibHandle = (void*) NULL;
static BOOLEAN                   gbGPLibInitialized = FALSE;

/* Local function definitions */
DWORD
GPInitLibrary(
    );

DWORD
GPCloseLibrary(
    );

DWORD
GPNotifyLogin(
    PCSTR pszLoginId
    );

DWORD
GPNotifyLogout(
    PCSTR pszLoginId
    );

void
GPUserMessageCB(
    void *context,
    int is_err,
    char *format,
    ...)
{
    /* We don't do anything with this */
}

void
GPLogMessageCB(
    void *context,
    int is_err,
    char *format,
    ...)
{
    /* We don't do anything with this */
}

DWORD
LsaPamNotifyUserLogon(
    PSTR pszLoginId
    )
{
    DWORD dwError = 0;

    LSA_LOG_PAM_DEBUG("LsaPamNotifyUserLogon::begin");

    if (pszLoginId == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = GPNotifyLogin(pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamNotifyUserLogon::end");

    return dwError;

error:

    if ((dwError == LW_ERROR_NO_SUCH_USER) || (dwError == LW_ERROR_NOT_HANDLED))
    {
        LSA_LOG_PAM_WARNING("LsaPamNotifyUserLogon failed [login:%s][error code: %u]", 
                            LSA_SAFE_LOG_STRING(pszLoginId),
                            dwError);
    }
    else
    {
        LSA_LOG_PAM_INFO("LsaPamNotifyUserLogon failed [login:%s][error code: %u]", 
                         LSA_SAFE_LOG_STRING(pszLoginId),
                         dwError);
    }

    goto cleanup;
}


DWORD
LsaPamNotifyUserLogoff(
    PSTR pszLoginId
    )
{
    DWORD dwError = 0;

    LSA_LOG_PAM_DEBUG("LsaPamNotifyUserLogoff::begin");

    if (pszLoginId == NULL)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = GPNotifyLogout(pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_LOG_PAM_DEBUG("LsaPamNotifyUserLogoff::end");
    return dwError;

error:

    if ((dwError == LW_ERROR_NO_SUCH_USER) || (dwError == LW_ERROR_NOT_HANDLED))
    {
        LSA_LOG_PAM_WARNING("LsaPamNotifyUserLogoff error [error code:%u]", dwError);
    }
    else
    {
        LSA_LOG_PAM_INFO("LsaPamNotifyUserLogoff error [error code:%u]", dwError);
    }

    goto cleanup;
}


/* Local helper functions used by above */
DWORD
GPInitLibrary(
    )
{
    DWORD dwError = 0;

    /* Test to see if we are already setup */
    if (gbGPLibInitialized == TRUE)
    {
        goto cleanup;
    }

    gbGPLibInitialized = TRUE;

    dlerror();

    gpGPLibHandle = dlopen(LIBDIR "/" GPAPI_DLL_NAME, RTLD_LAZY);
    if (gpGPLibHandle == NULL)
    {
        dwError = LW_ERROR_LOAD_LIBRARY_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gpfnGPPamProcessLogin = (PFN_GP_PAM_PROCESS_LOGIN) dlsym(gpGPLibHandle,
                                                             "gp_pam_process_login");
    if (gpfnGPPamProcessLogin == NULL)
    {
        dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gpfnGPPamProcessLogout = (PFN_GP_PAM_PROCESS_LOGOUT) dlsym(gpGPLibHandle,
                                                               "gp_pam_process_logout");
    if (gpfnGPPamProcessLogout == NULL)
    {
        dwError = LW_ERROR_LOOKUP_SYMBOL_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (dwError)
    {
        GPCloseLibrary();
    }

    goto cleanup;
}

DWORD
GPCloseLibrary(
    )
{
    DWORD dwError = 0;

    if (gpGPLibHandle)
    {
        if (gpfnGPPamProcessLogin)
        {
            gpfnGPPamProcessLogin = NULL;
        }

        if (gpfnGPPamProcessLogout)
        {
            gpfnGPPamProcessLogout = NULL;
        }

        if (dlclose(gpGPLibHandle))
        {
            dwError = LwMapErrnoToLwError(errno);
        }
        gpGPLibHandle = (void*) NULL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
GPNotifyLogin(
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    int ret = 0;

    dwError = GPInitLibrary();
    BAIL_ON_LSA_ERROR(dwError);

    if (gpGPLibHandle && gpfnGPPamProcessLogin)
    {
        ret = gpfnGPPamProcessLogin((void*) NULL,
                                    pszLoginId,
                                    0, // Not cached
                                    GPLogMessageCB,
                                    GPUserMessageCB);
        if (ret != 1)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
GPNotifyLogout(
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    int ret = 0;

    dwError = GPInitLibrary();
    BAIL_ON_LSA_ERROR(dwError);

    if (gpGPLibHandle && gpfnGPPamProcessLogout)
    {
        ret = gpfnGPPamProcessLogout((void*) NULL,
                                     pszLoginId,
                                     0, // Not cached
                                     GPLogMessageCB,
                                     GPUserMessageCB);
        if (ret != 1)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

