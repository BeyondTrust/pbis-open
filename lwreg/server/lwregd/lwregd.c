/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lwregd.c
 *
 * Abstract:
 *        Stand-alone registry service
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

/*
 * Simple driver main() program for lwregd. This allows lwregd
 * to easily be run as a stand-alone program. Useful for debugging
 * of the registry service, especially when a bug in the registry
 * prevents lwsmd from start/restarting lwregd.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#define _LWREG_SO "/opt/likewise/lib/lw-svcm/lwreg.so"

unsigned long 
    (*pfRegSvcmStart)(
        void *dummy1,
        unsigned long argc,
        char **argv,
        unsigned long fd,
        int *fds);

unsigned long (*pfRegSvcmStop)
    (void *dummy);


int main(int argc, char *argv[])
{
    sigset_t sigs;
    sigset_t cursigs;
    int caught = 0;
    char *libname = _LWREG_SO;

    sigemptyset(&sigs);
    sigemptyset(&cursigs);
    sigaddset(&sigs, SIGTERM);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGHUP);
    sigprocmask(SIG_BLOCK, &sigs, &cursigs);
    if (argc > 1)
    {
        libname = argv[1];
    }

    void *dlregistry = dlopen(libname, RTLD_LAZY);
    if (!dlregistry)
    {
        printf("dlopen(%s)\n", dlerror());
        return 1;
    }

    pfRegSvcmStart = dlsym(dlregistry, "RegSvcmStart");
    if (!dlregistry)
    {
        printf("dlsym(%s)\n", dlerror());
        return 1;
    }

    pfRegSvcmStop = dlsym(dlregistry, "RegSvcmStop");
    if (!dlregistry)
    {
        printf("dlsym(%s)\n", dlerror());
        return 1;
    }

    pfRegSvcmStart(NULL, 0, NULL, 0, NULL);
    sigwait(&sigs, &caught);

    printf("Shutting down lwregd (sig=%d)\n", caught);
    pfRegSvcmStop(NULL);
    dlclose(dlregistry);
    return 0;
}
