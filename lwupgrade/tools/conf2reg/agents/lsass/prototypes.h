/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

DWORD
LsaParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    );

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    );

DWORD
LsaSrvApiConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

DWORD
LsassGlobalSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue);

DWORD
PrintLsaConfig(
    FILE *fp,
    PLSA_SRV_API_CONFIG pConfig
    );


DWORD
LsaPamInitializeConfig(
    PLSA_PAM_CONFIG pConfig
    );

VOID
LsaPamFreeConfig(
    PLSA_PAM_CONFIG pConfig
    );

VOID
LsaPamFreeConfigContents(
    PLSA_PAM_CONFIG pConfig
    );


DWORD
LsaPamConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

DWORD
PrintPamConfig(
    FILE *fp,
    PLSA_PAM_CONFIG pConfig
    );


DWORD
UpAdInitializeConfig(
    PLSA_AD_CONFIG pConfig
    );

VOID
UpAdFreeConfigContents(
    PLSA_AD_CONFIG pConfig
    );

DWORD
UpAdSectionHandler(
    BOOLEAN         bStartOfSection,
    PCSTR           pszSectionName,
    PLSA_AD_CONFIG  pConfig,
    PBOOLEAN        pbContinue
    );

DWORD
UpAdConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PLSA_AD_CONFIG pConfig,
    PBOOLEAN pbContinue
    );

DWORD
UpAdPrintConfig(
    FILE *fp,
    PLSA_AD_CONFIG pConfig
    );



DWORD
UpLocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    );

VOID
UpLocalCfgFreeContents(
    PLOCAL_CONFIG pConfig
    );

DWORD
UpLocalCfgNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

DWORD
UpLocalPrintConfig(
    FILE *fp,
    PLOCAL_CONFIG pConfig
    );

