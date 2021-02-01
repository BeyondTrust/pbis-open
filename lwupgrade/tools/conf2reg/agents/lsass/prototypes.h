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

