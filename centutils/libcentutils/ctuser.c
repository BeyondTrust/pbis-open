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

#include "config.h"
#include "ctbase.h"
#if HAVE_UTMPS_H
#include <utmps.h>
#elif HAVE_UTMPX_H
#include <utmpx.h>
#elif HAVE_UTMP_H
#include <utmp.h>
#endif

#ifdef __LWI_FREEBSD__
#define UT_ID ut_name
/* Small implementation of utmp functions */

FILE* utmp_file = NULL;
struct utmp utmp_struct;

static void
utmp_init(void)
{
    if (!utmp_file)
        utmp_file = fopen(_PATH_UTMP, "r");
}

static void
setutent(void)
{
    utmp_init();
    rewind(utmp_file);
}

static struct utmp*
getutent(void)
{
    utmp_init();
    if (fread(&utmp_struct, sizeof(utmp_struct), 1, utmp_file) == 1)
        return &utmp_struct;
    else
        return NULL;
}

static void
endutent(void)
{
    if (utmp_file)
    {
        fclose(utmp_file);
        utmp_file = NULL;
    }
}

#else
#define UT_ID ut_id
#endif

DWORD
CTIsUserInX(BOOLEAN *inX)
{
#if HAVE_UTMPS_H
    struct utmps *ent;
#elif HAVE_UTMPX_H
    struct utmpx *ent;
#elif HAVE_UTMP_H
    struct utmp *ent;
#else
#error no utmp support found
#endif

    *inX = FALSE;

#if HAVE_UTMPS_H
    setutsent();
#elif HAVE_UTMPX_H
    setutxent();
#elif HAVE_UTMP_H
    setutent();
#endif
    while(1)
    {
#if HAVE_UTMPS_H
        ent = GETUTSENT();
#elif HAVE_UTMPX_H
        ent = getutxent();
#elif HAVE_UTMP_H
        ent = getutent();
#endif
        if(ent == NULL)
            break;
#ifdef USER_PROCESS
        if(ent->ut_type != USER_PROCESS)
            continue;
#endif
        if(
            /* Linux uses this */
            ent->ut_line[0] == ':' || ent->UT_ID[0] == ':' ||
            /* AIX uses this */
            (!strncmp(ent->ut_line, "lft", 3) && !strcmp(ent->UT_ID, "dt")) ||
            /* Solaris and HP-UX use this */
            (!strcmp(ent->ut_line, "console") && !strcmp(ent->UT_ID, "dt")))
        {
            *inX = TRUE;
            goto cleanup;
        }
    }

cleanup:
#if HAVE_UTMPS_H
    endutsent();
#elif HAVE_UTMPX_H
    endutxent();
#elif HAVE_UTMP_H
    endutent();
#endif

    return ERROR_SUCCESS;
}
