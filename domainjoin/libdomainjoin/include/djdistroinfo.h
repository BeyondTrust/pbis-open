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

#ifndef __DJDISTROINFO_H__
#define __DJDISTROINFO_H__

#include <lw/types.h>

typedef enum
{
    OS_UNKNOWN,
    OS_AIX,
    OS_SUNOS,
    OS_DARWIN,
    OS_HPUX,
    OS_LINUX,
    OS_FREEBSD,
} LwOSType;

typedef enum
{
    DISTRO_UNKNOWN = OS_UNKNOWN,
    DISTRO_AIX = OS_AIX,
    DISTRO_SUNOS = OS_SUNOS,
    DISTRO_DARWIN = OS_DARWIN,
    DISTRO_HPUX = OS_HPUX,
    DISTRO_RHEL,
    DISTRO_REDHAT,
    DISTRO_FEDORA,
    DISTRO_CENTOS,
    DISTRO_SUSE,
    DISTRO_OPENSUSE,
    DISTRO_SLES,
    DISTRO_SLED,
    DISTRO_UBUNTU,
    DISTRO_DEBIAN,
    DISTRO_FREEBSD,
    DISTRO_ESX,
    DISTRO_SCIENTIFIC
} LwDistroType;

typedef enum
{
    ARCH_UNKNOWN,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_HPPA,
    ARCH_IA64,
    ARCH_SPARC,
    ARCH_POWERPC,
} LwArchType;

typedef struct
{
    LwOSType os;
    LwDistroType distro;
    LwArchType arch;
    char *version;
} LwDistroInfo;

LwOSType DJGetOSFromString(const char *str);
LwDistroType DJGetDistroFromString(const char *str);
LwArchType DJGetArchFromString(const char * str);

DWORD DJGetOSString(LwOSType type, char **result);
DWORD DJGetDistroString(LwDistroType type, char **result);
DWORD DJGetArchString(LwArchType type, char **result);

//Fills in fields with correct values
DWORD
DJGetDistroInfo(const char *testPrefix, LwDistroInfo *info);

//Safe to call after DJGetDistroInfo has been called, or the structure has
//been zeroed out.
void DJFreeDistroInfo(LwDistroInfo *info);

DWORD
DJGetLikewiseVersion(
    PSTR *product,
    PSTR *version,
    PSTR *build,
    PSTR *revision
    );

#endif // __DJDISTROINFO_H__
