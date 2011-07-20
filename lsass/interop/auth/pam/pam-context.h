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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-context.h
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module (PAM)
 * 
 *        Likewise Context API
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
    BOOLEAN bPromptGecos;
    BOOLEAN bDisablePasswordChange;
    BOOLEAN bNoRequireMembership;
    
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
