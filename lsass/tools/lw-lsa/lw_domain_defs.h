/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Definitions
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#define LW_DOMAIN_VERSION_FILE_PATH PREFIXDIR "/data/VERSION"

typedef enum
{
    OS_UNKNOWN,
    OS_AIX,
    OS_SUNOS,
    OS_DARWIN,
    OS_HPUX,
    OS_LINUX,
    OS_FREEBSD,
} LwDomainOSType;

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
} LwDomainDistroType;

typedef enum
{
    ARCH_UNKNOWN,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_HPPA,
    ARCH_IA64,
    ARCH_SPARC,
    ARCH_POWERPC,
} LwDomainArchType;

#define LW_DOMAIN_ARCH_LOOKUPS \
    { ARCH_X86_32,  "x86_32"  }, \
    { ARCH_X86_32,  "i386"    }, \
    { ARCH_X86_32,  "i486"    }, \
    { ARCH_X86_32,  "i586"    }, \
    { ARCH_X86_32,  "i686"    }, \
    { ARCH_X86_64,  "x86_64"  }, \
    { ARCH_HPPA,    "hppa"    }, \
    { ARCH_IA64,    "ia64"    }, \
    { ARCH_IA64,    "itanium" }, \
    { ARCH_SPARC,   "sparc"   }, \
    { ARCH_POWERPC, "powerpc" }, \
    { ARCH_POWERPC, "ppc"     }

#define LW_DOMAIN_DISTRO_LOOKUPS \
    { DISTRO_AIX,      "AIX"      }, \
    { DISTRO_SUNOS,    "Solaris"  }, \
    { DISTRO_SUNOS,    "SunOS"    }, \
    { DISTRO_DARWIN,   "Darwin"   }, \
    { DISTRO_DARWIN,   "OsX"      }, \
    { DISTRO_HPUX,     "HP-UX"    }, \
    { DISTRO_HPUX,     "HPUX"     }, \
    { DISTRO_RHEL,     "RHEL"     }, \
    { DISTRO_REDHAT,   "Redhat"   }, \
    { DISTRO_FEDORA,   "Fedora"   }, \
    { DISTRO_CENTOS,   "CentOS"   }, \
    { DISTRO_SUSE,     "SuSE"     }, \
    { DISTRO_OPENSUSE, "OpenSuSE" }, \
    { DISTRO_SLES,     "SLES"     }, \
    { DISTRO_SLED,     "SLED"     }, \
    { DISTRO_UBUNTU,   "Ubuntu"   }, \
    { DISTRO_DEBIAN,   "Debian"   }, \
    { DISTRO_FREEBSD,  "FreeBSD"  }

#define LW_DOMAIN_OS_LOOKUPS \
    { OS_AIX,     "AIX"     }, \
    { OS_SUNOS,   "SunOS"   }, \
    { OS_SUNOS,   "Solaris" }, \
    { OS_DARWIN,  "Darwin"  }, \
    { OS_DARWIN,  "OsX"     }, \
    { OS_HPUX,    "HP-UX"   }, \
    { OS_HPUX,    "HPUX"    }, \
    { OS_LINUX,   "Linux"   }, \
    { OS_FREEBSD, "FreeBSD" }

#endif /* __DEFS_H__ */
