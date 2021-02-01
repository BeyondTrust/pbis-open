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

#ifndef __CTDISTROINFO_H__
#define __CTDISTROINFO_H__

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
} CtOSType;

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
} CtDistroType;

typedef enum
{
    ARCH_UNKNOWN,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_HPPA,
    ARCH_IA64,
    ARCH_SPARC,
    ARCH_POWERPC,
} CtArchType;

typedef struct
{
    CtOSType os;
    CtDistroType distro;
    CtArchType arch;
    char *version;
} CtDistroInfo;

CtOSType CTGetOSFromString(const char *str);
CtDistroType CTGetDistroFromString(const char *str);
CtArchType CTGetArchFromString(const char * str);

DWORD CTGetOSString(CtOSType type, char **result);
DWORD CTGetDistroString(CtDistroType type, char **result);
DWORD CTGetArchString(CtArchType type, char **result);

//Fills in fields with correct values
DWORD
CTGetDistroInfo(const char *testPrefix, CtDistroInfo *info);

//Safe to call after DJGetDistroInfo has been called, or the structure has
//been zeroed out.
void CTFreeDistroInfo(CtDistroInfo *info);

DWORD
CTGetLikewiseVersion(
    PSTR *product,
    PSTR *version,
    PSTR *build,
    PSTR *revision
    );

#endif // __CTDISTROINFO_H__
