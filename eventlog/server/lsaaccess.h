/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaaccess.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LSAACCESS_H__
#define __LSAACCESS_H__

typedef DWORD (*PFNLSAACCESSGETDATA)(
    PCSTR * pczConfigData,
    PVOID * ppAccessData
    );

typedef DWORD (*PFNLSAACCESSCHECKDATA)(
    PCSTR pczUserName,
    PCVOID pAccessData
    );

typedef DWORD (*PFNLSAACCESSFREEDATA)(
    PVOID pAccessData
    );

#define LSA_SYMBOL_NAME_ACCESS_GET_DATA "LsaAccessGetData"
#define LSA_SYMBOL_NAME_ACCESS_CHECK_DATA "LsaAccessCheckData"
#define LSA_SYMBOL_NAME_ACCESS_FREE_DATA "LsaAccessFreeData"

#endif /* __LSAACCESS_H__ */
