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
    char *extended; // os specific
} LwDistroInfo;

LwOSType DJGetOSFromString(const char *str);
LwDistroType DJGetDistroFromString(const char *str);
LwArchType DJGetArchFromString(const char * str);

DWORD DJGetOSString(LwOSType type, char **result);
DWORD DJGetDistroString(LwDistroType type, char **result);
DWORD DJGetArchString(LwArchType type, char **result);

// Fills in fields with correct values
DWORD DJGetDistroInfo(const char *testPrefix, LwDistroInfo *info);

// Safe to call after DJGetDistroInfo has been called, or the structure has
// been zeroed out.
void DJFreeDistroInfo(LwDistroInfo *info);

/**
 * @brief Get Solaris major and minor versions
 *
 * These are populated from the distro info
 * extended field and correspond to the Solaris vesion.
 *
 * @return TRUE/FALSE indicating success
 */
BOOLEAN DJGetSolarisVersion(const LwDistroInfo * const info, int * const major, int * const minor);

/**
 * @brief Returns TRUE is this is Enterprise.
 */
BOOLEAN DJGetIsEnterprise(void);

DWORD
DJGetLikewiseVersion(
    PSTR *product,
    PSTR *version,
    PSTR *build,
    PSTR *revision
    );

#endif // __DJDISTROINFO_H__
