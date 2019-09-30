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
#ifndef __UPGRADEUTILS_H__
#define __UPGRADEUTILS_H__

#include <lw/types.h>
#include <lw/attrs.h>

#include <lwerror.h>

#include <lsa/lsapstore-types.h>

typedef DWORD (*UpParseConfigSectionHandler)(
    BOOLEAN bStartOfSection,
    PCSTR pszSectionName, 
    PVOID pData,
    PBOOLEAN pbContinue
    );

typedef DWORD (*UpParseConfigNameValueHandler)(
    PCSTR pszSectionName, 
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    );

typedef DWORD (*UPCONVPFN)(PCSTR pszConfFilename, PCSTR pszRegFilename);


typedef struct _LWPS_PASSWORD_INFOA
{
    PSTR pszDomainName;
    PSTR pszDnsDomainName;
    PSTR pszSid;
    PSTR pszHostname;
    PSTR pszHostDnsDomain;
    PSTR pszMachineAccount;
    PSTR pszMachinePassword;
    time_t last_change_time;
    DWORD dwSchannelType;
} LWPS_PASSWORD_INFOA, *PLWPS_PASSWORD_INFOA;

DWORD
UpParseConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
    );

DWORD
UpParseSambaConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
    );


VOID
UpFreeString(
    PSTR pszStr
    );

DWORD
UpStringToMultiString(
    PCSTR pszIn,
    PCSTR pszDelims,
    PSTR *ppszOut
    );

DWORD
UpFormatBoolean(
    PCSTR pszName,
    BOOLEAN bValue,
    PSTR *ppszBoolean
    );

DWORD
UpFormatDword(
    PCSTR pszName,
    DWORD dwValue,
    PSTR *ppszDword
    );

DWORD
UpFormatString(
    PCSTR pszName,
    PCSTR pszValue,
    PSTR *ppszString
    );

DWORD
UpFormatMultiString(
    PCSTR pszName,
    PCSTR pszValue,
    PSTR *ppszMultiString
    );

DWORD
UpPrintBoolean(
    FILE *fp,
    PCSTR pszName,
    BOOLEAN bValue
    );

DWORD
UpPrintDword(
    FILE *fp,
    PCSTR pszName,
    DWORD dwValue
    );

DWORD
UpPrintString(
    FILE *fp,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
UpPrintMultiString(
    FILE *fp,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
UpParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

LSA_MACHINE_ACCOUNT_FLAGS
UpConvertSchannelTypeToMachineAccountFlags(
    IN DWORD SchannelType
    );

DWORD
UpConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    );

#if !defined(HAVE_STRTOLL)
long long int
strtoll(
    const char *nptr,
    char** endptr,
    int base
    );

#endif /* defined(HAVE_STRTOLL) */

#endif
