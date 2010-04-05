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

#include "includes.h"

static
DWORD
LwDomainGetOSType(
    PCSTR           pszPathPrefix,
    LwDomainOSType* pOsType
    );

static
LwDomainOSType
LwDomainGetOSFromString(
    PCSTR pszOSString
    );

static
DWORD
LwDomainGetDistroType(
    PCSTR               pszPathPrefix,
    LwDomainOSType      osType,
    LwDomainDistroType* pDistroType
    );

static
DWORD
LwDomainGetLinuxDistroType(
    PCSTR               pszPathPrefix,
    LwDomainDistroType* pDistroType
    );

static
LwDomainDistroType
LwDomainGetDistroFromString(
    PCSTR pszDistroString
    );

#if 0
static
DWORD
LwDomainGetDistroString(
    LwDomainDistroType distroType,
    PSTR* ppszDistroString
    );
#endif

static
DWORD
LwDomainGetDistroVersion(
    PCSTR              pszPathPrefix,
    LwDomainOSType     osType,
    LwDomainDistroType distroType,
    PSTR*              ppszVersion
    );

static
DWORD
LwDomainGetLinuxVersion(
    PCSTR pszPathPrefix,
    PSTR* ppszVersion
    );

static
DWORD
LwDomainGetDistroArch(
    PCSTR              pszPathPrefix,
    LwDomainOSType     osType,
    LwDomainDistroType distroType,
    LwDomainArchType*  pArchType
    );

static
LwDomainArchType
LwDomainGetArchFromString(
    PCSTR pszArchString
    );

#if 0
static
DWORD
LwDomainGetArchString(
    LwDomainArchType archType,
    PSTR* ppszArchString
    );
#endif

static
DWORD
LwDomainReadFile(
    PCSTR pszPath,
    PSTR* ppszFileContents
    );

static
DWORD
LwDomainCaptureCommandOutput(
    PCSTR pszCommand,
    PSTR* ppszOutput
    );

DWORD
LwGetLikewiseVersion(
    PSTR* ppszVersion,
    PSTR* ppszBuild,
    PSTR* ppszRevision
    )
{
    DWORD dwError = 0;
    PCSTR pszFilePath = LW_DOMAIN_VERSION_FILE_PATH;
    FILE* pVersionFile = NULL;
    PSTR  pszLine = NULL;
    PSTR  pszVersion  = NULL;
    PSTR  pszBuild    = NULL;
    PSTR  pszRevision = NULL;

    pVersionFile = fopen(pszFilePath, "r");
    if (pVersionFile == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (!feof(pVersionFile))
    {
        ssize_t len = 0;
        size_t  bufSize = 0;

        if (pszLine)
        {
            free(pszLine);
            pszLine = NULL;
        }

        len = getline(&pszLine, &bufSize, pVersionFile);
        if (len < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (len > 0)
        {
            LwStripWhitespace(pszLine, TRUE, TRUE);

            if (!strncmp(pszLine, "VERSION=", sizeof("VERSION=") - 1))
            {
                dwError = LwStrDupOrNull(
                            pszLine + sizeof("VERSION=") - 1,
                            &pszVersion);
            }
            else if (!strncmp(pszLine, "BUILD=", sizeof("BUILD=") - 1))
            {
                dwError = LwStrDupOrNull(
                            pszLine + sizeof("BUILD=") - 1,
                            &pszBuild);
            }
            else if (!strncmp(pszLine, "REVISION=", sizeof("REVISION=") - 1))
            {
                dwError = LwStrDupOrNull(
                            pszLine + sizeof("REVISION=") - 1,
                            &pszRevision);
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (!pszVersion)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pszBuild)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszBuild);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pszRevision)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszRevision);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszVersion = pszVersion;
    *ppszBuild = pszBuild;
    *ppszRevision = pszRevision;

cleanup:

    if (pszLine)
    {
        free(pszLine);
    }

    if (pVersionFile)
    {
        fclose(pVersionFile);
    }

    return dwError;

error:

    *ppszVersion = NULL;
    *ppszBuild = NULL;
    *ppszRevision = NULL;

    LW_SAFE_FREE_STRING(pszVersion);
    LW_SAFE_FREE_STRING(pszBuild);
    LW_SAFE_FREE_STRING(pszRevision);

    goto cleanup;
}

DWORD
LwDomainGetDistroInfo(
    PCSTR                   pszTestPrefix,
    PLW_DOMAIN_DISTRO_INFO* ppDistroInfo
    )
{
    DWORD dwError = 0;
    PLW_DOMAIN_DISTRO_INFO pDistroInfo = NULL;

    dwError = LwAllocateMemory(
                sizeof(LW_DOMAIN_DISTRO_INFO),
                (PVOID*)&pDistroInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDomainGetOSType(
                pszTestPrefix,
                &pDistroInfo->osType);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDomainGetDistroType(
                pszTestPrefix,
                pDistroInfo->osType,
                &pDistroInfo->distroType);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDomainGetDistroVersion(
                pszTestPrefix,
                pDistroInfo->osType,
                pDistroInfo->distroType,
                &pDistroInfo->pszVersion);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDomainGetDistroArch(
                pszTestPrefix,
                pDistroInfo->osType,
                pDistroInfo->distroType,
                &pDistroInfo->archType);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDistroInfo = pDistroInfo;

cleanup:

    return dwError;

error:

    *ppDistroInfo = NULL;

    if (pDistroInfo)
    {
        LwFreeDistroInfo(pDistroInfo);
    }

    goto cleanup;
}

static
DWORD
LwDomainGetOSType(
    PCSTR           pszPathPrefix,
    LwDomainOSType* pOsType
    )
{
    DWORD dwError = 0;
    LwDomainOSType osType = OS_UNKNOWN;
    PSTR pszPath         = NULL;
    PSTR pszFileContents = NULL;
    BOOLEAN bExists = FALSE;

    //Check for os override file
    dwError = LwAllocateStringPrintf(
                &pszPath,
                "%s/ostype",
                (pszPathPrefix ? pszPathPrefix : ""));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileOrLinkExists(
                pszPath,
                &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LwDomainReadFile(
                    pszPath,
                    &pszFileContents);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszFileContents)
    {
        LwStripWhitespace(pszFileContents, TRUE, TRUE);

        osType = LwDomainGetOSFromString(pszFileContents);
    }
    else
    {
        struct utsname unameStruct;

        //According to the Solaris man page, any non-negative return value
        //indicates success. In fact, Solaris 8 returns 1, while Linux returns
        //0.
        if (uname(&unameStruct) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        osType = LwDomainGetOSFromString(unameStruct.sysname);
    }

    *pOsType = osType;

cleanup:

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);

    return dwError;

error:

    *pOsType = OS_UNKNOWN;

    goto cleanup;
}

static
LwDomainOSType
LwDomainGetOSFromString(
    PCSTR pszOSString
    )
{
    LwDomainOSType osType = OS_UNKNOWN;
    LW_DOMAIN_OS_LOOKUP osList[] = { LW_DOMAIN_OS_LOOKUPS };
    int i = 0;

    for(; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if (!strcasecmp(pszOSString, osList[i].pszName))
        {
            osType = osList[i].osType;
            break;
        }
    }

    return osType;
}

DWORD
LwDomainGetOSString(
    LwDomainOSType osType,
    PSTR* ppszOSString
    )
{
    DWORD dwError = 0;
    LW_DOMAIN_OS_LOOKUP osList[] = { LW_DOMAIN_OS_LOOKUPS };
    PSTR pszOSString = NULL;
    PCSTR pszChosenOSString = NULL;
    int i = 0;

    for(; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if (osType == osList[i].osType)
        {
            pszChosenOSString = osList[i].pszName;
            break;
        }
    }

    dwError = LwAllocateString(
                (pszChosenOSString ? pszChosenOSString : "unknown"),
                &pszOSString);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszOSString = pszOSString;

cleanup:

    return dwError;

error:

    *ppszOSString = NULL;

    LW_SAFE_FREE_STRING(pszOSString);

    goto cleanup;
}

static
DWORD
LwDomainGetDistroType(
    PCSTR               pszPathPrefix,
    LwDomainOSType      osType,
    LwDomainDistroType* pDistroType
    )
{
    DWORD dwError = 0;
    LwDomainDistroType distroType = DISTRO_UNKNOWN;
    PSTR pszPath = NULL;
    PSTR pszFileContents = NULL;
    PSTR pszDistroString = NULL;
    BOOLEAN bExists = FALSE;

    //Check for distro override file
    dwError = LwAllocateStringPrintf(
                &pszPath,
                "%s/osdistro",
                (pszPathPrefix ? pszPathPrefix : ""));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileOrLinkExists(
                pszPath,
                &bExists);
    if (bExists)
    {
        dwError = LwDomainReadFile(
                    pszPath,
                    &pszDistroString);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszDistroString)
    {
        LwStripWhitespace(pszDistroString, TRUE, TRUE);

        distroType = LwDomainGetDistroFromString(pszDistroString);
    }
    else if (osType == OS_LINUX)
    {
        dwError = LwDomainGetLinuxDistroType(
                        pszPathPrefix,
                        &distroType);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        //It's a UNIX system
        switch(osType)
        {
            case OS_AIX:

                distroType = DISTRO_AIX;
                break;

            case OS_SUNOS:

                distroType = DISTRO_SUNOS;
                break;

            case OS_DARWIN:

                distroType = DISTRO_DARWIN;
                break;

            case OS_HPUX:

                distroType = DISTRO_HPUX;
                break;

            case OS_FREEBSD:

                distroType = DISTRO_FREEBSD;
                break;

            default:

                distroType = DISTRO_UNKNOWN;
                break;
        }
    }

    *pDistroType = distroType;

cleanup:

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);
    LW_SAFE_FREE_STRING(pszDistroString);

    return dwError;

error:

    *pDistroType = DISTRO_UNKNOWN;

    goto cleanup;
}

static
DWORD
LwDomainGetLinuxDistroType(
    PCSTR               pszPathPrefix,
    LwDomainDistroType* pDistroType
    )
{
    DWORD dwError = 0;
    LwDomainDistroType distroType = DISTRO_UNKNOWN;
    // TODO: Define this in a common place
    //       Until then, if you make a change here, also change in the
    //       LwDomainGetLinuxVersion method
    struct
    {
        LwDomainDistroType matchDistro;
        PCSTR              pszSearchFile;
        PCSTR              pszMatchRegex;
        int                versionMatchNum;
        BOOLEAN            bCompareCase;
    } const distroSearch[] =
    {
        {
            DISTRO_RHEL,
            "/etc/redhat-release",
            /*
            # The format of the line is something like:
            #   Red Hat Enterprise Linux ES release 4 (Nahant Update 1)
            #   Red Hat Linux Advanced Server release 2.1AS (Pensacola)
            #   Red Hat Enterprise Linux Client release 5 (Tikanga)
            #   Red Hat Enterprise Linux Server release 5.3 (Tikanga)
            # In addition, Oracle Linux reports itself as:
            #   Enterprise Linux Enterprise Linux AS release 4 (October Update 5)
            */
            //Find a matching distro name
            "^[[:space:]]*((Red Hat)|(Enterprise Linux)) ((Enterprise Linux)|(Linux (Advanced|Enterprise) Server))[[:space:]]+(AS |ES |Client |Server )?"
            //Get the version number, but strip the minor version if it is
            //present (RHEL 2 has one). Also remove the AS or ES
            //suffix if it is present.
            "release ([[:digit:]]+)(\\.[[:digit:]]+)?(AS|ES)?([[:space:]]+\\(.*\\))?[[:space:]]*$",
            9,
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
            1
        },
        {
            DISTRO_CENTOS,
            "/etc/redhat-release",
            /*
            # The format of the line is something like:
            #   CentOS release 4.x (Final)
            */
            "^[[:space:]]*CentOS release ([[:digit:]]+)"
            //Trim off the minor version number
            "(\\.[[:digit:]]+)?",
            1,
            1
        },
        {
            DISTRO_SUSE,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_OPENSUSE,
            "/etc/SuSE-release",
            "^[[:space:]]*openSUSE ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_SLES,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX Enterprise Server ([[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_SLED,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX Enterprise Desktop ([[:digit:]]+)[[:space:]]+",
            1,
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
            1
        },
    };
    int i = 0;
    regmatch_t matches[10];
    BOOLEAN rxAlloced    = FALSE;
    regex_t rx;
    BOOLEAN bExists = FALSE;
    PSTR    pszPath = NULL;
    PSTR    pszFileContents = NULL;

    memset(&rx, 0, sizeof(rx));

    for(; distroType == DISTRO_UNKNOWN; i++)
    {
        if (i == sizeof(distroSearch)/sizeof(distroSearch[0]))
        {
            //We are past the last item in DistroSearch
            break;
        }

        LW_SAFE_FREE_STRING(pszPath);

        dwError = LwAllocateStringPrintf(
                    &pszPath,
                    "%s%s",
                    (pszPathPrefix ? pszPathPrefix : ""),
                    distroSearch[i].pszSearchFile);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCheckFileOrLinkExists(pszPath, &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            int flags = REG_EXTENDED;

            if (!distroSearch[i].bCompareCase)
            {
                flags |= REG_ICASE;
            }

            LW_SAFE_FREE_STRING(pszFileContents);

            dwError = LwDomainReadFile(pszPath, &pszFileContents);
            BAIL_ON_LSA_ERROR(dwError);

            if (rxAlloced)
            {
                regfree(&rx);
                rxAlloced = FALSE;
            }

            if (regcomp(&rx, distroSearch[i].pszMatchRegex, flags) != 0)
            {
                dwError = LW_ERROR_REGEX_COMPILE_FAILED;
                BAIL_ON_LSA_ERROR(dwError);
            }

            rxAlloced = TRUE;

            if (regexec(
                    &rx,
                    pszFileContents,
                    sizeof(matches)/sizeof(matches[0]), matches, 0) == 0 &&
                    matches[distroSearch[i].versionMatchNum].rm_so != -1)
            {
                //This is the correct distro
                distroType = distroSearch[i].matchDistro;

                break;
            }
        }
    }

    if (distroType == DISTRO_DEBIAN)
    {
        LW_SAFE_FREE_STRING(pszPath);

        /*
        #
        # Debian and Ubuntu both have /etc/debian_version,
        # but only Ubuntu has an /etc/lsb-release
        #
        */
        dwError = LwAllocateStringPrintf(
                    &pszPath,
                    "%s/etc/lsb-release",
                    (pszPathPrefix ? pszPathPrefix : ""));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCheckFileOrLinkExists(pszPath, &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            LSA_LOG_ERROR("Unexpected file: %s", pszPath);
            distroType = DISTRO_UNKNOWN;
        }
    }

    *pDistroType = distroType;

cleanup:

    if (rxAlloced)
    {
        regfree(&rx);
        rxAlloced = FALSE;
    }

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);

    return distroType;

error:

    *pDistroType = DISTRO_UNKNOWN;

    goto cleanup;
}

static
LwDomainDistroType
LwDomainGetDistroFromString(
    PCSTR pszDistroString
    )
{
    LwDomainDistroType distroType = DISTRO_UNKNOWN;
    LW_DOMAIN_DISTRO_LOOKUP distroList[] = { LW_DOMAIN_DISTRO_LOOKUPS };
    int i = 0;

    for(; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if (!strcasecmp(pszDistroString, distroList[i].pszName))
        {
            distroType = distroList[i].distroType;

            break;
        }
    }

    return distroType;
}

#if 0
static
DWORD
LwDomainGetDistroString(
    LwDomainDistroType distroType,
    PSTR* ppszDistroString
    )
{
    DWORD dwError = 0;
    LW_DOMAIN_DISTRO_LOOKUP distroList[] = { LW_DOMAIN_DISTRO_LOOKUPS };
    PSTR pszDistroString = NULL;
    PCSTR pszChosenDistroString = NULL;
    int i = 0;

    for(; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if (distroType == distroList[i].distroType)
        {
            pszChosenDistroString = distroList[i].pszName;
            break;
        }
    }

    dwError = LwAllocateString(
                (pszChosenDistroString ? pszChosenDistroString : "unknown"),
                &pszDistroString);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszDistroString = pszDistroString;

cleanup:

    return dwError;

error:

    *ppszDistroString = NULL;

    LW_SAFE_FREE_STRING(pszDistroString);

    goto cleanup;
}
#endif

static
DWORD
LwDomainGetDistroVersion(
    PCSTR              pszPathPrefix,
    LwDomainOSType     osType,
    LwDomainDistroType distroType,
    PSTR*              ppszVersion
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR  pszPath = NULL;
    PSTR  pszFileContents = NULL;
    PSTR  pszVersion = NULL;

    if (distroType == DISTRO_UNKNOWN)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    //Check for version override file
    dwError = LwAllocateStringPrintf(
                    &pszPath,
                    "%s/osver",
                    pszPathPrefix ? pszPathPrefix : "");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileOrLinkExists(
                pszPath,
                &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LwDomainReadFile(
                    pszPath,
                    &pszFileContents);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszFileContents)
    {
        LwStripWhitespace(pszFileContents, TRUE, TRUE);

        pszVersion = pszFileContents;

        pszFileContents = NULL;
    }
    else if (osType == OS_LINUX)
    {
        dwError = LwDomainGetLinuxVersion(
                        pszPathPrefix,
                        &pszVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        struct utsname unameStruct;

        //According to the Solaris man page, any non-negative return value
        //indicates success. In fact, Solaris 8 returns 1, while Linux returns
        //0.
        if (uname(&unameStruct) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        //It's a UNIX system
        switch(osType)
        {
            case OS_AIX:
            /*Uname output from AIX 5.3:
            $ uname -v
            5
            $ uname -r
            3
            */
            dwError = LwAllocateStringPrintf(
                        &pszVersion,
                        "%s.%s",
                        unameStruct.version,
                        unameStruct.release);
            BAIL_ON_LSA_ERROR(dwError);

            break;

            case OS_SUNOS:

                /*Uname output from Solaris 8:
                $ uname -r
                5.8
                */
                dwError = LwAllocateStringPrintf(
                            &pszVersion,
                            "%s",
                            unameStruct.release);
                BAIL_ON_LSA_ERROR(dwError);

                break;

            case OS_DARWIN:

                dwError = LwDomainCaptureCommandOutput(
                            "sw_vers -productVersion",
                            &pszVersion);
                BAIL_ON_LSA_ERROR(dwError);

                LwStripWhitespace(pszVersion, TRUE, TRUE);

                break;

            case OS_HPUX:

                {
                    PCSTR pszTemp = unameStruct.release;

                    while(!isdigit((int)*pszTemp))
                    {
                        pszTemp++;
                    }

                    dwError = LwAllocateString(
                                pszTemp,
                                &pszVersion);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                break;

            case OS_FREEBSD:

                dwError = LwAllocateStringPrintf(
                            &pszVersion,
                            "%s",
                            unameStruct.release);
                BAIL_ON_LSA_ERROR(dwError);

                break;

            default:

                break;
        }
    }

    if (!pszVersion)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszVersion = pszVersion;

cleanup:

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);

    return dwError;

error:

    *ppszVersion = NULL;

    LW_SAFE_FREE_STRING(pszVersion);

    goto cleanup;
}

static
DWORD
LwDomainGetLinuxVersion(
    PCSTR pszPathPrefix,
    PSTR* ppszVersion
    )
{
    DWORD dwError = 0;
    // TODO: Define this in a common place
    //       Until then, if you make a change here, also change in the
    //       LwDomainGetLinuxDistroType method
    struct
    {
        LwDomainDistroType matchDistro;
        PCSTR              pszSearchFile;
        PCSTR              pszMatchRegex;
        int                versionMatchNum;
        BOOLEAN            bCompareCase;
    } const distroSearch[] =
    {
        {
            DISTRO_RHEL,
            "/etc/redhat-release",
            /*
            # The format of the line is something like:
            #   Red Hat Enterprise Linux ES release 4 (Nahant Update 1)
            #   Red Hat Linux Advanced Server release 2.1AS (Pensacola)
            #   Red Hat Enterprise Linux Client release 5 (Tikanga)
            #   Red Hat Enterprise Linux Server release 5.3 (Tikanga)
            # In addition, Oracle Linux reports itself as:
            #   Enterprise Linux Enterprise Linux AS release 4 (October Update 5)
            */
            //Find a matching distro name
            "^[[:space:]]*((Red Hat)|(Enterprise Linux)) ((Enterprise Linux)|(Linux (Advanced|Enterprise) Server))[[:space:]]+(AS |ES |Client |Server )?"
            //Get the version number, but strip the minor version if it is
            //present (RHEL 2 has one). Also remove the AS or ES
            //suffix if it is present.
            "release ([[:digit:]]+)(\\.[[:digit:]]+)?(AS|ES)?([[:space:]]+\\(.*\\))?[[:space:]]*$",
            9,
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
            1
        },
        {
            DISTRO_CENTOS,
            "/etc/redhat-release",
            /*
            # The format of the line is something like:
            #   CentOS release 4.x (Final)
            */
            "^[[:space:]]*CentOS release ([[:digit:]]+)"
            //Trim off the minor version number
            "(\\.[[:digit:]]+)?",
            1,
            1
        },
        {
            DISTRO_SUSE,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_OPENSUSE,
            "/etc/SuSE-release",
            "^[[:space:]]*openSUSE ([[:digit:]]+\\.[[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_SLES,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX Enterprise Server ([[:digit:]]+)[[:space:]]+",
            1,
            0
        },
        {
            DISTRO_SLED,
            "/etc/SuSE-release",
            "^[[:space:]]*SUSE LINUX Enterprise Desktop ([[:digit:]]+)[[:space:]]+",
            1,
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
            1
        },
    };
    int i = 0;
    regmatch_t matches[10];
    BOOLEAN rxAlloced    = FALSE;
    regex_t rx;
    PSTR pszVersion = NULL;
    PSTR pszPath = NULL;
    PSTR pszFileContents = NULL;
    LwDomainDistroType distroType = DISTRO_UNKNOWN;
    BOOLEAN bExists = FALSE;

    memset(&rx, 0, sizeof(rx));

    for(; distroType == DISTRO_UNKNOWN; i++)
    {
        if (i == sizeof(distroSearch)/sizeof(distroSearch[0]))
        {
            //We are past the last item in DistroSearch
            break;
        }

        LW_SAFE_FREE_STRING(pszPath);

        dwError = LwAllocateStringPrintf(
                    &pszPath,
                    "%s%s",
                    (pszPathPrefix ? pszPathPrefix : ""),
                    distroSearch[i].pszSearchFile);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCheckFileOrLinkExists(pszPath, &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            int flags = REG_EXTENDED;

            if (!distroSearch[i].bCompareCase)
            {
                flags |= REG_ICASE;
            }

            LW_SAFE_FREE_STRING(pszFileContents);

            dwError = LwDomainReadFile(pszPath, &pszFileContents);
            BAIL_ON_LSA_ERROR(dwError);

            if (rxAlloced)
            {
                regfree(&rx);
                rxAlloced = FALSE;
            }

            if (regcomp(&rx, distroSearch[i].pszMatchRegex, flags) != 0)
            {
                dwError = LW_ERROR_REGEX_COMPILE_FAILED;
                BAIL_ON_LSA_ERROR(dwError);
            }

            rxAlloced = TRUE;

            if (regexec(
                    &rx,
                    pszFileContents,
                    sizeof(matches)/sizeof(matches[0]), matches, 0) == 0 &&
                    matches[distroSearch[i].versionMatchNum].rm_so != -1)
            {
                //This is the correct distro
                regmatch_t * pVer = &matches[distroSearch[i].versionMatchNum];

                dwError = LwStrndup(
                            pszFileContents + pVer->rm_so,
                            pVer->rm_eo - pVer->rm_so,
                            &pszVersion);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }
        }
    }

    if (!pszVersion)
    {
        dwError = LwAllocateString(
                        "unknown",
                        &pszVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszVersion = pszVersion;

cleanup:

    if (rxAlloced)
    {
        regfree(&rx);
        rxAlloced = FALSE;
    }

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);

    return distroType;

error:

    *ppszVersion = NULL;

    LW_SAFE_FREE_STRING(pszVersion);

    goto cleanup;
}

static
DWORD
LwDomainGetDistroArch(
    PCSTR              pszPathPrefix,
    LwDomainOSType     osType,
    LwDomainDistroType distroType,
    LwDomainArchType*  pArchType
    )
{
    DWORD dwError = 0;
#if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
    CHAR szArchBuffer[100];
#endif
    LwDomainArchType archType = ARCH_UNKNOWN;
    BOOLEAN bExists = FALSE;
    PSTR    pszPath = NULL;
    PSTR    pszFileContents = NULL;
    PSTR pszDistroString = NULL;

    //Check for arch override file
    dwError = LwAllocateStringPrintf(
                &pszPath,
                "%s/osarch",
                pszPathPrefix ? pszPathPrefix : "");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileOrLinkExists(
                pszPath,
                &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LwDomainReadFile(
                    pszPath,
                    &pszFileContents);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszFileContents)
    {
        archType = LwDomainGetArchFromString(pszFileContents);
    }
    else
    {
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

    #if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
        //Solaris has this
        if (archType == ARCH_UNKNOWN &&
            sysinfo(SI_ARCHITECTURE, archBuffer, sizeof(archBuffer)) != -1)
        {
            archType = LwDomainGetArchFromString(archBuffer);
        }
    #endif

    #if defined(HAVE_SYSCONF) && defined(_SC_CPU_VERSION)
        //HPUX uses this
        if (archType == ARCH_UNKNOWN)
        {
            switch(sysconf(_SC_CPU_VERSION))
            {
                case CPU_PA_RISC1_0:
                case CPU_PA_RISC1_1:
                case CPU_PA_RISC1_2:
                case CPU_PA_RISC2_0:
                case CPU_PA_RISC_MAX:

                    archType = ARCH_HPPA;
                    break;

    #ifdef CPU_HP_INTEL_EM_1_0
                case CPU_HP_INTEL_EM_1_0:
    #endif

    #ifdef CPU_IA64_ARCHREV_0
                case CPU_IA64_ARCHREV_0:
    #endif

                    archType = ARCH_IA64;
                    break;
                //If it's not any of the previous values, let another test figure
                //it out.
            }
        }
    #endif

        if (archType == ARCH_UNKNOWN)
        {
            struct utsname unameStruct;

            //According to the Solaris man page, any non-negative return value
            //indicates success. In fact, Solaris 8 returns 1, while Linux returns
            //0.
            if (uname(&unameStruct) < 0)
            {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }

            //Linux uses this, and sometimes Darwin does too. If 'uname -m' doesn't
            //return something meaningful on this platform, then the arch will stay
            //as unknown.
            archType = LwDomainGetArchFromString(unameStruct.machine);
        }

        if (archType == ARCH_UNKNOWN)
        {
            //AIX and sometimes Darwin use this
            dwError = LwDomainCaptureCommandOutput(
                        "uname -p",
                        &pszDistroString);
            BAIL_ON_LSA_ERROR(dwError);

            LwStripWhitespace(pszDistroString, TRUE, TRUE);

            archType = LwDomainGetArchFromString(pszDistroString);
        }
    }

    *pArchType = archType;

cleanup:

    LW_SAFE_FREE_STRING(pszPath);
    LW_SAFE_FREE_STRING(pszFileContents);

    return dwError;

error:

    *pArchType = ARCH_UNKNOWN;

    goto cleanup;
}

static
LwDomainArchType
LwDomainGetArchFromString(
    PCSTR pszArchString
    )
{
    LW_DOMAIN_ARCH_LOOKUP archLookups[] = { LW_DOMAIN_ARCH_LOOKUPS };
    LwDomainArchType archType = ARCH_UNKNOWN;
    int i = 0;

    for(; i < sizeof(archLookups)/sizeof(archLookups[0]); i++)
    {
        if (!strcasecmp(pszArchString, archLookups[i].pszName))
        {
            archType = archLookups[i].archType;
            break;
        }
    }

    return archType;
}

#if 0
static
DWORD
LwDomainGetArchString(
    LwDomainArchType archType,
    PSTR* ppszArchString
    )
{
    DWORD dwError = 0;
    LW_DOMAIN_ARCH_LOOKUP archLookups[] = { LW_DOMAIN_ARCH_LOOKUPS };
    int i = 0;
    PSTR pszArchString = NULL;
    PCSTR pszChosenArchString = NULL;

    for(; i < sizeof(archLookups)/sizeof(archLookups[0]); i++)
    {
        if (archType == archLookups[i].archType)
        {
            pszChosenArchString = archLookups[i].pszName;

            break;
        }
    }

    dwError = LwAllocateString(
                 (pszChosenArchString ? pszChosenArchString : "unknown"),
                 &pszArchString);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszArchString = pszArchString;

cleanup:

    return dwError;

error:

    *ppszArchString = NULL;

    LW_SAFE_FREE_STRING(pszArchString);

    goto cleanup;
}
#endif

VOID
LwFreeDistroInfo(
    PLW_DOMAIN_DISTRO_INFO pDistroInfo
    )
{
    LwFreeDistroInfoContents(pDistroInfo);

    LwFreeMemory(pDistroInfo);
}

VOID
LwFreeDistroInfoContents(
    PLW_DOMAIN_DISTRO_INFO pDistroInfo
    )
{
    LW_SAFE_FREE_MEMORY(pDistroInfo->pszVersion);
}

static
DWORD
LwDomainReadFile(
    PCSTR pszPath,
    PSTR* ppszFileContents
    )
{
    DWORD dwError = 0;
    struct stat statbuf;
    FILE* pFp = NULL;
    PSTR pszFileContents = NULL;

    memset(&statbuf, 0, sizeof(statbuf));

    if (stat(pszPath, &statbuf) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (statbuf.st_size > 0)
    {
        dwError = LwAllocateMemory(
                    statbuf.st_size + 1,
                    (PVOID*)&pszFileContents);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pFp = fopen(pszPath, "r");
    if (pFp == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (fread(pszFileContents, statbuf.st_size, 1, pFp) != 1)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszFileContents = pszFileContents;

cleanup:

    if (pFp)
    {
        fclose(pFp);
    }

    return dwError;

error:

    *ppszFileContents = pszFileContents;

    LW_SAFE_FREE_MEMORY(pszFileContents);

    goto cleanup;
}

static
DWORD
LwDomainCaptureCommandOutput(
    PCSTR pszCommand,
    PSTR* ppszOutput
    )
{
    DWORD dwError = 0;

    // TODO:

    return dwError;
}
