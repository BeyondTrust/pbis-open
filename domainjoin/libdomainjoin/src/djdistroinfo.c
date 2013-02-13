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
#include <sys/utsname.h>
#include <sys/types.h>
#include <regex.h>
#ifdef HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <lwstr.h>
#include <lwmem.h>
#include <lwdef.h>
#ifdef HAVE_COREFOUNDATION_COREFOUNDATION_H
#include <CoreFoundation/CoreFoundation.h>
#endif

#define GCE(x) GOTO_CLEANUP_ON_DWORD((x))

#ifdef __LWI_DARWIN__

static
DWORD
GetPstrFromStringRef(
    CFStringRef pIn,
    PSTR *ppOut
    )
{
    size_t len = 0;
    PSTR pOut = NULL;
    DWORD error = 0;

    len = CFStringGetMaximumSizeForEncoding(
                CFStringGetLength(pIn),
                CFStringGetSystemEncoding());

    error = LwAllocateMemory(
                len + 1,
                (PVOID*)&pOut);
    GCE(error);

    if (!CFStringGetCString(
                pIn,
                pOut,
                len + 1,
                CFStringGetSystemEncoding()))
    {
        error = ERROR_ILLEGAL_CHARACTER;
        GCE(error);
    }

    *ppOut = pOut;

cleanup:
    if (error)
    {
        LW_SAFE_FREE_STRING(pOut);
    }
    return error;
}

static
DWORD
UrlErrorToLwError(
    SInt32 urlError
    )
{
    switch(urlError)
    {
        default:
        case kCFURLUnknownError:
            return ERROR_GEN_FAILURE;
        case kCFURLUnknownSchemeError:
            return ERROR_PROTOCOL_UNREACHABLE;
        case kCFURLResourceNotFoundError:
            return ERROR_NOT_FOUND;
        case kCFURLResourceAccessViolationError:
            return ERROR_ACCESS_DENIED;
        case kCFURLRemoteHostUnavailableError:
            return ERROR_HOST_UNREACHABLE;
        case kCFURLImproperArgumentsError:
            return ERROR_BAD_ARGUMENTS;
        case kCFURLUnknownPropertyKeyError:
            return ERROR_UNKNOWN_PROPERTY;
        case kCFURLPropertyKeyUnavailableError:
            return ERROR_INVALID_ACCESS;
        case kCFURLTimeoutError:
            return ERROR_TIMEOUT;
    }
}

static
DWORD
DJGetPListVersion(
    PSTR* ppVersion
    )
{
    DWORD error = 0;
    CFURLRef pURL = NULL;
    CFDataRef pContents = NULL;
    SInt32 urlError = 0;
    // Do not free. This is returned by a mac function following the "get"
    // rule.
    CFStringRef pVers = NULL;
    CFStringRef pError = NULL;
    PSTR pVersionString = NULL;
    CFPropertyListRef pPList = NULL;
    PSTR pErrorString = NULL;

    pURL = CFURLCreateWithFileSystemPath(
                    kCFAllocatorDefault,
                    CFSTR("/System/Library/CoreServices/SystemVersion.plist"),
                    kCFURLPOSIXPathStyle,
                    false);
    
    if (!pURL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        GCE(error);
    }

    if (!CFURLCreateDataAndPropertiesFromResource(
            kCFAllocatorDefault,
            pURL,
            &pContents,
            NULL,
            NULL,
            &urlError))
    {
        error = UrlErrorToLwError(urlError);
        GCE(error);
    }

    pPList = CFPropertyListCreateFromXMLData(
                    kCFAllocatorDefault,
                    pContents,
                    kCFPropertyListImmutable,
                    &pError);
    if (!pPList)
    {
        GetPstrFromStringRef(pError, &pErrorString);
        DJ_LOG_ERROR("Error '%s' parsing OS X version file",
                LW_SAFE_LOG_STRING(pErrorString));
        error = ERROR_PRODUCT_VERSION;
        GCE(error);
    }

    pVers = (CFStringRef)CFDictionaryGetValue(
                (CFDictionaryRef)pPList,
                CFSTR("ProductVersion"));
    if (!pVers)
    {
        error = ERROR_PRODUCT_VERSION;
        GCE(error);
    }

    error = GetPstrFromStringRef(
                pVers,
                &pVersionString);
    GCE(error);

    *ppVersion = pVersionString;

cleanup:
    if (error)
    {
        LW_SAFE_FREE_STRING(pVersionString);
    }
    if (pError)
    {
        CFRelease(pError);
    }
    LW_SAFE_FREE_STRING(pErrorString);
    if (pURL)
    {
        CFRelease(pURL);
    }
    if (pContents)
    {
        CFRelease(pContents);
    }
    if (pPList)
    {
        CFRelease(pPList);
    }
    return error;
}
#else
static
DWORD
DJGetPListVersion(
    PSTR* ppVersion
    )
{
    return LwAllocateString("Unknown", ppVersion);
}
#endif

DWORD DJGetDistroInfo(const char *testPrefix, LwDistroInfo *info)
{
    BOOLEAN exists;
    DWORD ceError = ERROR_SUCCESS;
    struct utsname unameStruct;
    char *path = NULL;
    PSTR fileContents = NULL;
    PSTR distroString = NULL;
    BOOLEAN rxAlloced = FALSE;
    regex_t rx;
    memset(info, 0, sizeof(*info));
#if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
    char archBuffer[100];
#endif

    //According to the Solaris man page, any non-negative return value
    //indicates success. In fact, Solaris 8 returns 1, while Linux returns
    //0.
    if(uname(&unameStruct) < 0)
        return LwMapErrnoToLwError(errno);

    //Check for os override file
    if(testPrefix == NULL)
        testPrefix = "";
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/ostype", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, SIZE_MAX, &fileContents, NULL));
    }
    if(fileContents != NULL)
    {
        CTStripWhitespace(fileContents);
        info->os = DJGetOSFromString(fileContents);
    }
    else
        info->os = DJGetOSFromString(unameStruct.sysname);
    CT_SAFE_FREE_STRING(fileContents);
    CT_SAFE_FREE_STRING(path);

    //Check for distro override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osdistro", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, SIZE_MAX, &distroString, NULL));
    }
    CT_SAFE_FREE_STRING(path);

    if(distroString != NULL)
    {
        CTStripWhitespace(distroString);
        info->distro = DJGetDistroFromString(distroString);
        CT_SAFE_FREE_STRING(distroString);
        GCE(ceError = CTStrdup("unknown", &info->version));
    }
    else if(info->os == OS_LINUX)
    {
        struct
        {
            LwDistroType matchDistro;
            const char *searchFile;
            const char *matchRegex;
            int versionMatchNum;
            int updateMatchNum;
            BOOLEAN compareCase;
        } const distroSearch[] = {
            {
                DISTRO_ESX,
                "/etc/vmware-release",
                /*
                # The format of the line is something like:
                #   VMware ESX 4.0 (Kandinsky)
                */
                "^[[:space:]]*VMware ESX ([[:digit:]]+(\\.[[:digit:]]+)?)"
                "( \\(\\S+\\))?",
                1,
                -1,
                1
            },
            {
                DISTRO_RHEL,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Red Hat Enterprise Linux ES release 4 (Nahant Update 1)
                #   Red Hat Linux Advanced Server release 2.1AS (Pensacola)
                #   Red Hat Enterprise Linux Client release 5 (Tikanga)
                #   Red Hat Enterprise Linux Server release 5.3 (Tikanga)
                #   Red Hat Enterprise Linux ES release 4 (Nahant Update 8)
                #   Red Hat Enterprise Linux Workstation release 6.0 (Santiago)
                # In addition, Oracle Linux reports itself as:
                #   Enterprise Linux Enterprise Linux AS release 4 (October Update 5)
                */
                //Find a matching distro name
                "^[[:space:]]*((Red Hat)|(Enterprise Linux)) ((Enterprise Linux)|(Linux (Advanced|Enterprise) Server))[[:space:]]+(AS |ES |Client |Server |Workstation )?"
                "release ([[:digit:]]+(\\.[[:digit:]]+)?(AS|ES)?) (\\(\\S+ Update ([[:digit:]]+)\\))?",
                9,
                13,
                1
            },
            {
                DISTRO_REDHAT,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Red Hat Linux release 7.3 (Valhala)
                */
                "^[[:space:]]*Red Hat Linux release ([[:digit:]]+(\\.[[:digit:]]+)?)",
                1,
                -1,
                1
            },
            {
                DISTRO_FEDORA,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Fedora Core release 4 (Stentz)
                */
                "^[[:space:]]*Fedora (Core )?release (\\S+)",
                2,
                -1,
                1
            },
            {
                DISTRO_CENTOS,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   CentOS release 4.x (Final)
                */
                "^[[:space:]]*(CentOS|CentOS Linux) release ([[:digit:]]+(\\.[[:digit:]]+)?)",
                2,
                -1,
                1
            },
            {
                DISTRO_SCIENTIFIC,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Scientific Linux release 6.1 (Carbon)
                */
                "^[[:space:]]*(Scientific|Scientific Linux) release ([[:digit:]]+(\\.[[:digit:]]+)?)",
                2,
                -1,
                1
            },
            {
                DISTRO_SUSE,
                "/etc/SuSE-release",
                "^[[:space:]]*SUSE LINUX ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
                1,
                -1,
                0
            },
            {
                DISTRO_OPENSUSE,
                "/etc/SuSE-release",
                "^[[:space:]]*openSUSE ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
                1,
                -1,
                0
            },
            {
                DISTRO_SLES,
                "/etc/SuSE-release",
                "^[[:space:]]*SUSE LINUX Enterprise Server ([[:digit:]]+( SP[[:digit:]]+)?)",
                1,
                -1,
                0
            },
            {
                DISTRO_SLED,
                "/etc/SuSE-release",
                "^[[:space:]]*SUSE LINUX Enterprise Desktop ([[:digit:]]+( SP[[:digit:]]+)?)",
                1,
                -1,
                0
            },
            {
                DISTRO_UBUNTU,
                "/etc/lsb-release",
                /*
                # The file will have lines that include:
                #   DISTRIB_ID=Ubuntu
                #   DISTRIB_RELEASE=6.06
                */
                "^[[:space:]]*DISTRIB_ID[[:space:]]*=[[:space:]]*Ubuntu[[:space:]]*\n"
                "(.*\n)?DISTRIB_RELEASE[[:space:]]*=[[:space:]]*(\\S+)[[:space:]]*(\n.*)?$",
                2,
                -1,
                1
            },
            {
                DISTRO_DEBIAN,
                "/etc/debian_version",
                /*
                # The format of the entire file is a single line like:
                # 3.1
                # and nothing else, so that is the version
                */
                "^[[:space:]]*(\\S+)[[:space:]]*$",
                1,
                -1,
                1
            },
        };
        int i;
        regmatch_t matches[20];
        info->distro = DISTRO_UNKNOWN;
        for(i = 0; info->distro == DISTRO_UNKNOWN; i++)
        {
            if(i == sizeof(distroSearch)/sizeof(distroSearch[0]))
            {
                //We past the last item in DistroSearch
                break;
            }
            GCE(ceError = CTAllocateStringPrintf(
                    &path, "%s%s", testPrefix, distroSearch[i].searchFile));
            GCE(ceError = CTCheckFileOrLinkExists(path, &exists));
            if(exists)
            {
                int flags = REG_EXTENDED;
                if(!distroSearch[i].compareCase)
                    flags |= REG_ICASE;

                GCE(ceError = CTReadFile(path, SIZE_MAX, &fileContents, NULL));
                if(regcomp(&rx, distroSearch[i].matchRegex, flags) != 0)
                {
                    GCE(ceError = LW_ERROR_REGEX_COMPILE_FAILED);
                }
                rxAlloced = TRUE;
                if(regexec(&rx, fileContents,
                        sizeof(matches)/sizeof(matches[0]), matches, 0) == 0 &&
                        matches[distroSearch[i].versionMatchNum].rm_so != -1)
                {
                    //This is the correct distro
                    regmatch_t *ver = &matches[distroSearch[i].versionMatchNum];
                    regmatch_t *update = &matches[distroSearch[i].updateMatchNum];
                    info->distro = distroSearch[i].matchDistro;
                    if (distroSearch[i].updateMatchNum == -1 ||
                            update->rm_so == -1)
                    {
                        GCE(ceError = CTStrndup(fileContents + ver->rm_so,
                                    ver->rm_eo - ver->rm_so,
                                    &info->version));
                    }
                    else
                    {
                        GCE(ceError = CTAllocateStringPrintf(
                                &info->version,
                                "%.*s.%.*s",
                                ver->rm_eo - ver->rm_so,
                                fileContents + ver->rm_so,
                                update->rm_eo - update->rm_so,
                                fileContents + update->rm_so));
                    }
                }
                regfree(&rx);
                rxAlloced = FALSE;
                CT_SAFE_FREE_STRING(fileContents);
            }
            CT_SAFE_FREE_STRING(path);
        }
        if(info->distro == DISTRO_DEBIAN)
        {
            /*
            #
            # Debian and Ubuntu both have /etc/debian_version,
            # but only Ubuntu has an /etc/lsb-release
            #
            */
            GCE(ceError = CTAllocateStringPrintf(
                    &path, "%s/etc/lsb-release", testPrefix));
            GCE(ceError = CTCheckFileOrLinkExists(path, &exists));
            if(exists)
            {
                DJ_LOG_ERROR("Unexpected file: %s", path);
                info->distro = DISTRO_UNKNOWN;
            }
            CT_SAFE_FREE_STRING(path);
        }
    }
    else
    {
        //It's a UNIX system
        switch(info->os)
        {
        case OS_AIX:
            info->distro = DISTRO_AIX;
            /*Uname output from AIX 5.3:
            $ uname -v
            5
            $ uname -r
            3
            */
            GCE(ceError = CTAllocateStringPrintf(&info->version,
                        "%s.%s", unameStruct.version, unameStruct.release));
            break;
        case OS_SUNOS:
            info->distro = DISTRO_SUNOS;
            /*Uname output from Solaris 8:
            $ uname -r
            5.8
            */
            GCE(ceError = CTAllocateStringPrintf(&info->version,
                        "%s", unameStruct.release));
            break;
        case OS_DARWIN:
            info->distro = DISTRO_DARWIN;
            ceError = DJGetPListVersion(&info->version);
            GCE(ceError);
            CTStripWhitespace(info->version);
            break;
        case OS_HPUX:
            info->distro = DISTRO_HPUX;
            {
                const char *temp = unameStruct.release;
                while(!isdigit((int)*temp)) temp++;
                GCE(ceError = CTStrdup(temp, &info->version));
            }
            break;
        case OS_FREEBSD:
            info->distro = DISTRO_FREEBSD;
            GCE(ceError = CTAllocateStringPrintf(&info->version,
                        "%s", unameStruct.release));
            break;
        default:
            info->distro = DISTRO_UNKNOWN;
        }
    }

    if(info->distro == DISTRO_UNKNOWN)
    {
        CT_SAFE_FREE_STRING(info->version);
        GCE(ceError = CTStrdup("unknown", &info->version));
    }

    //Check for version override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osver", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, SIZE_MAX, &fileContents, NULL));
    }
    if(fileContents != NULL)
    {
        CTStripWhitespace(fileContents);
        CT_SAFE_FREE_STRING(info->version);
        info->version = fileContents;
        fileContents = NULL;
    }
    CT_SAFE_FREE_STRING(path);

    /*
    uname -m output:
    Linux: x86_64
    Linux: i386
    Linux: i686
    AIX: 00CBE1DD4C00
    Solaris: sun4u
    Solaris: i86pc
    Darwin: i386
    Darwin: Power Macintosh
    HPUX: 9000/785

    uname -i output:
    Linux: x86_64
    Linux: i386
    RHEL21: not recogn
    AIX: not recogn
    Darwin: not recogn
    Solaris: SUNW,Ultra-4
    Solaris: i86pc
    HPUX: 2000365584

    uname -p output:
    Linux reads /proc/cpuinfo
    Linux: x86_64
    Linux: i686
    Linux: athlon
    Darwin: i386
    Darwin: powerpc
    AIX has the value hard coded in uname
    AIX: powerpc
    Solaris uses sysinfo(SI_ARCHITECTURE, buff, sizeof(buff)
    Solaris: sparc
    Solaris: i386
    HPUX: not recogn
    */
    info->arch = ARCH_UNKNOWN;
#if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
    //Solaris has this
    if(info->arch == ARCH_UNKNOWN &&
            sysinfo(SI_ARCHITECTURE, archBuffer, sizeof(archBuffer)) != -1)
    {
        info->arch = DJGetArchFromString(archBuffer);
    }
#endif
#if defined(HAVE_SYSCONF) && defined(_SC_CPU_VERSION)
    //HPUX uses this
    if(info->arch == ARCH_UNKNOWN)
    {
        switch(sysconf(_SC_CPU_VERSION))
        {
            case CPU_PA_RISC1_0:
            case CPU_PA_RISC1_1:
            case CPU_PA_RISC1_2:
            case CPU_PA_RISC2_0:
            case CPU_PA_RISC_MAX:
                info->arch = ARCH_HPPA;
                break;
#ifdef CPU_HP_INTEL_EM_1_0
            case CPU_HP_INTEL_EM_1_0:
#endif
#ifdef CPU_IA64_ARCHREV_0
            case CPU_IA64_ARCHREV_0:
#endif
                info->arch = ARCH_IA64;
                break;
            //If it's not any of the previous values, let another test figure
            //it out.
        }
    }
#endif
    if(info->arch == ARCH_UNKNOWN)
    {
        //Linux uses this, and sometimes Darwin does too. If 'uname -m' doesn't
        //return something meaningful on this platform, then the arch will stay
        //as unknown.
        info->arch = DJGetArchFromString(unameStruct.machine);
    }
    if(info->arch == ARCH_UNKNOWN)
    {
        //AIX and sometimes Darwin use this
        GCE(ceError = CTCaptureOutput("uname -p", &distroString));
        CTStripWhitespace(distroString);
        info->arch = DJGetArchFromString(distroString);
        CT_SAFE_FREE_STRING(distroString);
    }

    //Check for arch override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osarch", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, SIZE_MAX, &fileContents, NULL));
        info->arch = DJGetArchFromString(fileContents);
        CT_SAFE_FREE_STRING(fileContents);
    }
    CT_SAFE_FREE_STRING(path);

cleanup:
    CT_SAFE_FREE_STRING(path);
    CT_SAFE_FREE_STRING(fileContents);
    CT_SAFE_FREE_STRING(distroString);
    if(rxAlloced)
        regfree(&rx);
    if(ceError)
    {
        DJFreeDistroInfo(info);
    }
    return ceError;
}

struct
{
    LwOSType value;
    const char *name;
} static const osList[] = 
{
    { OS_AIX, "AIX" },
    { OS_SUNOS, "SunOS" },
    { OS_SUNOS, "Solaris" },
    { OS_DARWIN, "Mac OS X"},
    { OS_DARWIN, "Darwin"},
    { OS_DARWIN, "OsX" },
    { OS_HPUX, "HP-UX"},
    { OS_LINUX, "Linux" },
    { OS_FREEBSD, "FreeBSD" },
};

LwOSType DJGetOSFromString(const char *str)
{
    int i;
    for(i = 0; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if(!strcasecmp(str, osList[i].name))
            return osList[i].value;
    }
    return OS_UNKNOWN;
}

DWORD DJGetOSString(LwOSType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if(type == osList[i].value)
            return CTStrdup(osList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

struct
{
    LwDistroType value;
    const char *name;
} static const distroList[] = 
{
    { DISTRO_AIX, "AIX" },
    { DISTRO_SUNOS, "Solaris" },
    { DISTRO_SUNOS, "SunOS" },
    { DISTRO_DARWIN, "Darwin"},
    { DISTRO_HPUX, "HP-UX"},
    { DISTRO_RHEL, "RHEL" },
    { DISTRO_REDHAT, "Redhat" },
    { DISTRO_FEDORA, "Fedora" },
    { DISTRO_CENTOS, "CentOS" },
    { DISTRO_SCIENTIFIC, "Scientific" },
    { DISTRO_SUSE, "SuSE" },
    { DISTRO_OPENSUSE, "OpenSuSE" },
    { DISTRO_SLES, "SLES" },
    { DISTRO_SLED, "SLED" },
    { DISTRO_UBUNTU, "Ubuntu" },
    { DISTRO_DEBIAN, "Debian" },
    { DISTRO_FREEBSD, "FreeBSD" },
    { DISTRO_ESX, "VMware ESX" },
};

LwDistroType DJGetDistroFromString(const char *str)
{
    int i;
    for(i = 0; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if(!strcasecmp(str, distroList[i].name))
            return distroList[i].value;
    }
    return DISTRO_UNKNOWN;
}

DWORD DJGetDistroString(LwDistroType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if(type == distroList[i].value)
            return CTStrdup(distroList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

struct
{
    LwArchType value;
    const char *name;
} static const archList[] = 
{
    { ARCH_X86_32, "x86_32" },
    { ARCH_X86_32, "i386" },
    { ARCH_X86_32, "i486" },
    { ARCH_X86_32, "i586" },
    { ARCH_X86_32, "i686" },
    { ARCH_X86_64, "x86_64" },
    { ARCH_X86_64, "amd64" },
    { ARCH_HPPA, "hppa" },
    { ARCH_IA64, "ia64" },
    { ARCH_IA64, "itanium" },
    { ARCH_SPARC, "sparc" },
    { ARCH_POWERPC, "powerpc" },
    { ARCH_POWERPC, "ppc" },
};

LwArchType DJGetArchFromString(const char * str)
{
    int i;
    for(i = 0; i < sizeof(archList)/sizeof(archList[0]); i++)
    {
        if(!strcasecmp(str, archList[i].name))
            return archList[i].value;
    }
    return ARCH_UNKNOWN;
}

DWORD DJGetArchString(LwArchType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(archList)/sizeof(archList[0]); i++)
    {
        if(type == archList[i].value)
            return CTStrdup(archList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

void DJFreeDistroInfo(LwDistroInfo *info)
{
    if(info != NULL)
        CT_SAFE_FREE_STRING(info->version);
}

DWORD
DJGetLikewiseVersion(
    PSTR *product,
    PSTR *version,
    PSTR *build,
    PSTR *revision
    )
{
    FILE *versionFile = NULL;
    PSTR line = NULL; 
    BOOLEAN isEndOfFile = FALSE;
    DWORD ceError = ERROR_SUCCESS;
    BOOLEAN bIsEnterprise = FALSE;

    PSTR _product = NULL;
    PSTR _version = NULL;
    PSTR _build = NULL;
    PSTR _revision = NULL;

    *version = NULL;
    *build = NULL;
    *revision = NULL;

#ifdef MINIMAL_JOIN
    GCE(ceError = CTOpenFile(LOCALSTATEDIR "/VERSION", "r", &versionFile));
#else
    ceError = CTOpenFile(PREFIXDIR "/data/ENTERPRISE_VERSION", "r", &versionFile);
    if (ceError == 0)
    {
        bIsEnterprise = TRUE;
    }
    else
    {
        GCE(ceError = CTOpenFile(PREFIXDIR "/data/VERSION", "r", &versionFile));
    }
#endif

    while (TRUE)
    {
        GCE(ceError = CTReadNextLine(versionFile, &line, &isEndOfFile));
        if (isEndOfFile)
            break;
        CTStripWhitespace(line);
        if (!strncmp(line, "VERSION=", sizeof("VERSION=") - 1))
        {
            GCE(ceError = CTStrdup(line + sizeof("VERSION=") - 1,
                        &_version));
        }
        else if (!strncmp(line, "BUILD=", sizeof("BUILD=") - 1))
        {
            GCE(ceError = CTStrdup(line + sizeof("BUILD=") - 1,
                    &_build));
        }
        else if (!strncmp(line, "REVISION=", sizeof("REVISION=") - 1))
        {
            GCE(ceError = CTStrdup(line + sizeof("REVISION=") - 1,
                    &_revision));
        }
    }

    if (bIsEnterprise)
    {
        GCE(ceError = CTStrdup("Enterprise", &_product));
    }
    else
    {
        GCE(ceError = CTStrdup("Open", &_product));
    }
    if (_version == NULL)
    {
        GCE(ceError = CTStrdup("unknown", &_version));
    }
    if (_build == NULL)
    {
        GCE(ceError = CTStrdup("unknown", &_build));
    }
    if (_revision == NULL)
    {
        GCE(ceError = CTStrdup("unknown", &_revision));
    }

    GCE(ceError = CTSafeCloseFile(&versionFile));

    *product = _product;
    *version = _version;
    *build = _build;
    *revision = _revision;
    _product = NULL;
    _version = NULL;
    _build = NULL;
    _revision = NULL;
    
cleanup:

    CTSafeCloseFile(&versionFile);
    CT_SAFE_FREE_STRING(line);
    CT_SAFE_FREE_STRING(_version);
    CT_SAFE_FREE_STRING(_build);
    CT_SAFE_FREE_STRING(_revision);

    return ceError;
}
