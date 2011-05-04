/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2011
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
