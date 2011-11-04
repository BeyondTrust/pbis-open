/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        backtrace.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Service Entry API
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 */
#include "config.h"
#include "lsassd.h"
#include "lsasrvutils.h"

#ifdef HAVE_BACKTRACE
static
void
LsaSrvCrashHandler(
    int iSignal,
    siginfo_t* pSigInfo,
    void* pvContext
    );
#endif

#ifdef HAVE_BACKTRACE
static
void
LsaSrvCrashHandler(
    int iSignal,
    siginfo_t* pSigInfo,
    void* pvContext
    )
{
    void *ppFunctions[100];
    int dwFunctionsSize = sizeof(ppFunctions)/sizeof(ppFunctions[0]);
    PSTR* ppszSymbols = NULL;
    int iIndex = 0;
    ucontext_t* pContext = (ucontext_t*)pvContext;

    LSA_LOG_ALWAYS("Lsass crashed with signal %d", iSignal);

    // This gets an array of the function return pointers for all of the
    // functions on the stack.
    dwFunctionsSize = backtrace(ppFunctions, dwFunctionsSize);
    if (dwFunctionsSize >= 2)
    {
        // When signal handler is run, a return pointer to the function that
        // caused the signal is not stored on the stack. However, the EIP/RIP
        // register is backed up in the ucontext. The function pointer second
        // from the top is not useful, so this code replaces that function
        // pointer with where the exception was thrown.
#ifdef REG_nPC
        // On Sparc, nPC is program counter for the next instruction
        ppFunctions[1] = (void *)pContext->uc_mcontext.gregs[REG_nPC];
#elif defined(REG_RIP)
        // Linux 64bit instruction pointer
        ppFunctions[1] = (void *)pContext->uc_mcontext.gregs[REG_RIP];
#elif defined(HAVE_UCONTEXT_T_UC_MCONTEXT____SS___RIP)
        // Mac 64bit instruction pointer
        ppFunctions[1] = (void *)pContext->uc_mcontext->__ss.__rip;
#elif defined(HAVE_UCONTEXT_T_UC_MCONTEXT____SS___EIP)
        // Mac 32bit instruction pointer
        ppFunctions[1] = (void *)pContext->uc_mcontext->__ss.__eip;
#else
        // Linux 32bit instruction pointer
        ppFunctions[1] = (void *)pContext->uc_mcontext.gregs[REG_EIP];
#endif
    }

    // Converts the list of function pointers into a list of function names
    ppszSymbols = backtrace_symbols(ppFunctions, dwFunctionsSize);

    if (ppszSymbols)
    {
        for (iIndex = 0; iIndex < dwFunctionsSize; iIndex++)
        {
            LSA_LOG_ALWAYS("%d - %s", iIndex, ppszSymbols[iIndex]);
        }
        LW_SAFE_FREE_MEMORY(ppszSymbols);
    }
}
#endif

DWORD
LsaSrvRegisterCrashHandler(
    VOID
    )
{
#ifdef HAVE_BACKTRACE
    DWORD dwError = 0;
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    // Have the signal handler only one once (SA_RESETHAND). This way when a
    // segfault occurs, the signal handler will get run. When the signal
    // handler finishes, the program will resume where it left off (and crash
    // exactly the same way again). On the second crash, no handler will be
    // registered so the program will coredump.
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    action.sa_sigaction = LsaSrvCrashHandler;
    if (sigaction(SIGILL, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (sigaction(SIGFPE, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (sigaction(SIGSEGV, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (sigaction(SIGBUS, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (sigaction(SIGABRT, &action, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;

#else
    return 0;
#endif
}
