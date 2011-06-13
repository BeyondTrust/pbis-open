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

#include "domainjoin.h"
#include "djdistroinfo.h"
#include "djsystemscripts.h"

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

//Replace this (on Ubuntu):
// PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\007"'
//With this (on Ubuntu):
// PROMPT_COMMAND=$'echo -n "\033]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\007"'
//
//Replace this (on Debian):
// PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME}: ${PWD}\007"'
//With this (on Debian):
// PROMPT_COMMAND=$'echo -n "\033]0;${USER}@${HOSTNAME}: ${PWD}\007"'
static PCSTR ubuntuSedExpr = "s/^\\([ \t]*\\)PROMPT_COMMAND=\'echo -ne \"\\\\033]0;${USER}@${HOSTNAME}: ${PWD\\([^}]*\\)}\\\\007\"\'\\([ \t]*\\)$/"
"\\1PROMPT_COMMAND=$\'echo -n \"\\\\033]0;${USER}@${HOSTNAME}: ${PWD\\2}\\\\007\"\'\\3/";

static QueryResult QueryBash(const JoinProcessOptions *options, LWException **exc)
{
    LwDistroInfo distro;
    BOOLEAN exists;
    BOOLEAN changes;
    QueryResult result = SufficientlyConfigured;

    memset(&distro, 0, sizeof(distro));

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    switch(distro.distro)
    {
        case DISTRO_RHEL:
        case DISTRO_CENTOS:
        case DISTRO_FEDORA:
            LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists("/etc/sysconfig", &exists));
            if(!exists) {
                result = NotApplicable;
                break;
            }
            //If both the bash-prompt-xterm and bash-prompt-screen files exist,
            //then this is fully configured.
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/sysconfig/bash-prompt-xterm", &exists));
            if(!exists)
                break;
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/sysconfig/bash-prompt-screen", &exists));
            if(!exists)
                break;
            result = FullyConfigured;
            break;
        case DISTRO_DEBIAN:
        case DISTRO_UBUNTU:
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/skel/.bashrc", &exists));
            if(!exists)
            {
                //We can't fix it if it doesn't exist
                result = NotApplicable;
                break;
            }
            LW_CLEANUP_CTERR(exc, CTWillSedChangeFile("/etc/skel/.bashrc",
                ubuntuSedExpr, &changes));
            if(!changes)
            {
                //The change was already applied
                result = FullyConfigured;
            }
            break;
        default:
            result = NotApplicable;
    }

cleanup:
    DJFreeDistroInfo(&distro);
    return result;
}

static DWORD WritePromptFile(PCSTR filename, PCSTR contents)
{
    DWORD ceError = ERROR_SUCCESS;
    FILE *file = NULL;

    GCE(CTOpenFile(filename, "w", &file));
    GCE(CTChangePermissions(filename, 0755));
    GCE(CTFileStreamWrite(file, contents, strlen(contents)));

cleanup:
    if (file != NULL)
    {
        CTCloseFile(file);
        file = NULL;
    }
    return ceError;
}

static void DoBash(JoinProcessOptions *options, LWException **exc)
{
    LwDistroInfo distro;
    BOOLEAN exists;

    memset(&distro, 0, sizeof(distro));

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    switch(distro.distro)
    {
        case DISTRO_RHEL:
        case DISTRO_CENTOS:
        case DISTRO_FEDORA:
            LW_CLEANUP_CTERR(exc, CTCheckDirectoryExists("/etc/sysconfig", &exists));
            if(!exists)
            {
                break;
            }
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/sysconfig/bash-prompt-screen", &exists));
            if(!exists)
            {
                //Create the file
                LW_CLEANUP_CTERR(exc, WritePromptFile(
                        "/etc/sysconfig/bash-prompt-screen",
                        "#!/bin/bash\n"
                        "echo -n $'\\033'\"_${USER}@${HOSTNAME%%.*}:${PWD/#$HOME/~}\"$'\\033\\\\'\n"
                        ));
            }
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/sysconfig/bash-prompt-xterm", &exists));
            if(!exists)
            {
                //Create the file
                LW_CLEANUP_CTERR(exc, WritePromptFile(
                        "/etc/sysconfig/bash-prompt-xterm",
                        "#!/bin/bash\n"
                        "echo -n $'\\033'\"]0;${USER}@${HOSTNAME%%.*}:${PWD/#$HOME/~}\"$'\\007'\n"
                        ));
            }
            break;
        case DISTRO_DEBIAN:
        case DISTRO_UBUNTU:
            LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/skel/.bashrc", &exists));
            if(!exists)
            {
                //We can't fix it if it doesn't exist
                break;
            }
            LW_CLEANUP_CTERR(exc, CTRunSedOnFile("/etc/skel/.bashrc",
                "/etc/skel/.bashrc", FALSE, ubuntuSedExpr));
            break;
        default:
            break;
    }

cleanup:
    DJFreeDistroInfo(&distro);
}

static PSTR GetBashDescription(const JoinProcessOptions *options, LWException **exc)
{
    LwDistroInfo distro;
    QueryResult currentStatus;
    PSTR result = NULL;

    memset(&distro, 0, sizeof(distro));
    LW_TRY(exc, currentStatus = QueryBash(options, &LW_EXC));
    if(currentStatus == FullyConfigured)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Fully configured", &result));
        goto cleanup;
    }
    else if(currentStatus == NotApplicable)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Not applicable", &result));
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(NULL, &distro));
    switch(distro.distro)
    {
        case DISTRO_RHEL:
        case DISTRO_CENTOS:
        case DISTRO_FEDORA:
            LW_CLEANUP_CTERR(exc, CTStrdup("On redhat-based systems, the default bash prompt is overwritten by creating /etc/sysconfig/bash-prompt-xterm and /etc/sysconfig/bash-prompt-xterm. This is done so that the prompt is displayed correctly for usernames with a backslash.", &result));
            break;
        case DISTRO_DEBIAN:
        case DISTRO_UBUNTU:
            LW_CLEANUP_CTERR(exc, CTStrdup("On debian and ubuntu based systems, the default bash prompt is changed in /etc/skel/.bashrc. This is done so that the prompt is displayed correctly for usernames with a backslash.", &result));
        default:
            break;
    }

cleanup:
    DJFreeDistroInfo(&distro);
    return result;
}

const JoinModule DJBashPrompt = { TRUE, "bash", "fix bash prompt for backslashes in usernames", QueryBash, DoBash, GetBashDescription };

//Replace this:
// /usr/bin/X11/sessreg -a -w /var/log/wtmp -u none -l $DISPLAY $USER
//With this:
// /usr/bin/X11/sessreg -a -w /var/log/wtmp -u none -l $DISPLAY "$USER"
static PCSTR gdmSedExpr = "s/\\/usr\\/bin\\/X11\\/sessreg\\ -a\\ -w\\ \\/var\\/log\\/wtmp\\ -u\\ none\\ -l\\ $DISPLAY\\ $USER/"
"\\/usr\\/bin\\/X11\\/sessreg\\ -a\\ -w\\ \\/var\\/log\\/wtmp\\ -u\\ none\\ -l\\ $DISPLAY\\ \\\"$USER\\\"/";

static QueryResult QueryGdm(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN exists;
    BOOLEAN changes;
    QueryResult result = NotApplicable;

    if(!options->joiningDomain)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/X11/gdm/PreSession/Default", &exists));
    if(!exists)
    {
        //We can't fix it if it doesn't exist
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, CTWillSedChangeFile(
        "/etc/X11/gdm/PreSession/Default", gdmSedExpr, &changes));
    if(!changes)
    {
        //The change was already applied
        result = FullyConfigured;
    }

cleanup:
    return result;
}

static void DoGdm(JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN exists;
    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists("/etc/X11/gdm/PreSession/Default", &exists));
    if(!exists)
    {
        //We can't fix it if it doesn't exist
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, CTRunSedOnFile(
        "/etc/X11/gdm/PreSession/Default",
        "/etc/X11/gdm/PreSession/Default",
        FALSE,
        gdmSedExpr));

cleanup:
    ;
}

static PSTR GetGdmDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR result = NULL;
    QueryResult currentStatus;

    LW_TRY(exc, currentStatus = QueryGdm(options, &LW_EXC));
    if(currentStatus == FullyConfigured)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Fully configured", &result));
        goto cleanup;
    }
    else if(currentStatus == NotApplicable)
    {
        LW_CLEANUP_CTERR(exc, CTStrdup("Not applicable", &result));
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, CTStrdup("The gdm presession script (/etc/X11/gdm/PreSession/Default) will be edited to allow usernames with spaces to log in. This is done by replacing:\n"
"\t/usr/bin/X11/sessreg -a -w /var/log/wtmp -u none -l $DISPLAY $USER\n"
"With:\n"
"\t/usr/bin/X11/sessreg -a -w /var/log/wtmp -u none -l $DISPLAY \"$USER\"",
        &result));

cleanup:
    return result;
}

const JoinModule DJGdmPresession = { TRUE, "gdm", "fix gdm presession script for spaces in usernames", QueryGdm, DoGdm, GetGdmDescription };
