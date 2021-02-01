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
