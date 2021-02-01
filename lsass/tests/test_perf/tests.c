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
 *
 *        tests.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) 
 *        
 *        Test helper functions
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 *
 */

#include "types.h"
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

BOOL
RunGrabUid(
    IN PVOID arg
    )
{
    uid_t userid = (uid_t)(size_t)arg;
    struct passwd *result = getpwuid(userid);

    if (result == NULL)
    {
        perror(__FUNCTION__);
        return FALSE;
    }
    return TRUE;
}

BOOL
SetupGrabUid(
    IN PVOID username,
    OUT PVOID *uid
    )
{
    struct passwd *result = getpwnam((PSTR)username);
    if (result == NULL)
    {
        perror(__FUNCTION__);
        return FALSE;
    }

    *uid = (PVOID)(size_t)result->pw_uid;
    return TRUE;
}

BOOL
RunGrabGid(
    IN PVOID arg
    )
{
    gid_t groupid = (gid_t)(size_t)arg;
    struct group *result = getgrgid(groupid);

    if (result == NULL)
    {
        perror(__FUNCTION__);
        return FALSE;
    }
    return TRUE;
}

BOOL
SetupGrabGid(
    IN PVOID groupname,
    OUT PVOID *gid
    )
{
    struct group *result = getgrnam((PSTR)groupname);
    if (result == NULL)
    {
        perror(__FUNCTION__);
        return FALSE;
    }

    *gid = (PVOID)(size_t)result->gr_gid;
    return TRUE;
}

BOOL
RunGrabUsers1_500(
    IN PVOID prefix
    )
{
    int i;
    char name[100];
    struct passwd *result;

    for (i = 1; i <= 500; i++)
    {
        snprintf(name, sizeof(name), "%s%04d", (PSTR)prefix, i);
        result = getpwnam(name);
        if (result == NULL)
        {
            perror(__FUNCTION__);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
RunGrabGroups1_500(
    IN PVOID prefix
    )
{
    int i;
    char name[100];
    struct group *result;

    for (i = 1; i <= 500; i++)
    {
        snprintf(name, sizeof(name), "%s%04d", (PSTR)prefix, i);
        result = getgrnam(name);
        if (result == NULL)
        {
            perror(__FUNCTION__);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
SetupClearCache(
    IN PVOID prefix,
    OUT PVOID *outPrefix
    )
{
    int status;

    status = system("/etc/init.d/likewise-open stop 2>/dev/null || /etc/init.d/lsassd stop 2>/dev/null");
    if (WEXITSTATUS(status) != 0)
    {
        printf("Unable to stop authentication daemon using init script\n");
        return FALSE;
    }
    
    status = system("rm -rf "
                    "/var/lib/lwidentity/idmap_cache.tdb "
                    "/var/lib/lwidentity/netsamlogon_cache.tdb "
                    "/var/lib/lwidentity/winbindd_cache.tdb "
                    "/var/lib/likewise/db/lsass-adcache.db "
                    "/var/lib/likewise/db/lsass-adcache.filedb");
    if (WEXITSTATUS(status) != 0)
    {
        printf("Unable to clear caches\n");
        return FALSE;
    }
    
    status = system("/etc/init.d/likewise-open start 2>/dev/null || /etc/init.d/lsassd start 2>/dev/null");

    if (WEXITSTATUS(status) != 0)
    {
        printf("Unable to start authentication daemon\n");
        return FALSE;
    }

    // Give the daemon a little time to prep
    sleep(1);

    *outPrefix = prefix;
    return TRUE;
}
