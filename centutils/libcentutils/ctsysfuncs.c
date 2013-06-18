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

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif
#include "lw/types.h"
#include "ctbase.h"
#include <syslog.h>
#include "ctsysfuncs.h"

void
sys_vsyslog(
    int priority,
    const char *format,
    va_list ap
    )
{
#if defined(HAVE_VSYSLOG)
    vsyslog(priority, format, ap);
#else
    DWORD ceError;
    PSTR buffer = NULL;

    ceError = CTAllocateStringPrintfV(&buffer, format, ap);
    if (!ceError)
    {
        syslog(priority, "%s", buffer);
    }

    CT_SAFE_FREE_STRING(buffer);
#endif /* ! HAVE_VSYSLOG */
}

BOOLEAN
IsRoot()
{
    return (getuid() == 0);
}

#ifndef HAVE_MKDTEMP
char *mkdtemp(char *temp)
{
    unsigned int seed = 0;
    int attempt;
    struct timespec curtime;
    size_t templateLen;

    if(temp == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    templateLen = strlen(temp);
    if(templateLen < 6)
    {
        errno = EINVAL;
        return NULL;
    }
    if(strcmp(temp + templateLen - 6, "XXXXXX"))
    {
        errno = EINVAL;
        return NULL;
    }

    for(attempt = 0; attempt < 50; attempt++)
    {
        if(clock_gettime(CLOCK_REALTIME, &curtime) < 0)
            return NULL;
        seed += (unsigned int)curtime.tv_nsec;
        seed += (unsigned int)getpid();

        sprintf(temp + templateLen - 6, "%.6X", rand_r(&seed) & 0xFFFFFF);

        if(mkdir(temp, 0700) < 0)
        {
            if(errno == EEXIST || errno == ELOOP || errno == ENOENT || errno == ENOTDIR)
                continue;
            return NULL;
        }
        return temp;
    }
    return NULL;
}
#endif /* ! HAVE_MKDTEMP */
