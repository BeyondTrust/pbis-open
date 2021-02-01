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

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-context.h
 *
 * Abstract:
 * 
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module (PAM)
 * 
 *        BeyondTrust Context API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __PAM_CONTEXT_H__
#define __PAM_CONTEXT_H__

typedef struct _PAMOPTIONS
{
    
    BOOLEAN bTryFirstPass;
    BOOLEAN bUseFirstPass;
    BOOLEAN bUseAuthTok;
    BOOLEAN bRememberChPass;
    BOOLEAN bSetDefaultRepository;
    BOOLEAN bLsassUsersOnly;
    BOOLEAN bUnknownOK;
    BOOLEAN bSmartCardPrompt;
    BOOLEAN bSmartCardAuth;
    BOOLEAN bPromptGecos;
    BOOLEAN bDisablePasswordChange;
    BOOLEAN bNoRequireMembership;
    BOOLEAN bDebug;
    
} PAMOPTIONS, *PPAMOPTIONS;

typedef struct _PAMCONTEXT
{
    PSTR       pszLoginName;
    PAMOPTIONS pamOptions;
    BOOLEAN    bPasswordExpired;
    BOOLEAN    bPasswordMessageShown;
    BOOLEAN    bPasswordChangeFailed;
    BOOLEAN    bPasswordChangeSuceeded;
    BOOLEAN    bOnlineLogon;
    
} PAMCONTEXT, *PPAMCONTEXT;

DWORD
LsaPamGetContext(
    pam_handle_t* pamh, 
    int           flags, 
    int           argc, 
    const char**  argv,
    PPAMCONTEXT*  ppContext
    );

DWORD
LsaPamGetOptions(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv,
    PPAMOPTIONS   pPamOptions
    );

DWORD
LsaPamGetLoginId(
    pam_handle_t* pamh,
    PPAMCONTEXT   pContext,
    PSTR*         ppszLoginId,
    BOOLEAN       bAllowPrompt
    );

void
LsaPamCleanupContext(
    pam_handle_t* pamh,
    void*         pData,
    int           error_status
    );

void
LsaPamFreeContext(
    PPAMCONTEXT pContext
    );

DWORD
LsaPamSetDataString(
    pam_handle_t* pamh,
    PCSTR pszKey,
    PCSTR pszStr
    );

#endif /* __PAM_CONTEXT_H__ */
