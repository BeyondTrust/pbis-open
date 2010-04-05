/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        credcontext.h
 *
 * Abstract:
 *
 *       lw-gp-admin
 *
 *       AD Utility API (Private Header)
 *
 * Author: Glenn Curtis (glennc@likewisesoftware.com)
 *         Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __CREDCONTEXT_H__
#define __CREDCONTEXT_H__

DWORD
ADUBuildCredContext(
    PCSTR              pszDomain,
    PCSTR              pszUserUPN,
    PADU_CRED_CONTEXT* ppCredContext
    );

DWORD
ADUActivateCredContext(
    PADU_CRED_CONTEXT pCredContext
    );

DWORD
ADUDeactivateCredContext(
    PADU_CRED_CONTEXT pCredContext
    );

VOID
ADUFreeCredContext(
    PADU_CRED_CONTEXT pCredContext
    );

#endif /* __CREDCONTEXT_H__ */
