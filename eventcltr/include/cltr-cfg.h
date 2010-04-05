/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        evtcfg.h
 *
 * Abstract:
 *
 *        Likewise Eventlog
 * 
  *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __CLTR_CFG_H__
#define __CLTR_CFG_H__

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNCONFIG_START_SECTION)(
                        PCWSTR    pszSectionName,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_COMMENT)(
                        PCWSTR    pszComment,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_NAME_VALUE_PAIR)(
                        PCWSTR    pszName,
                        PCWSTR    pszValue,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_END_SECTION)(
                        PCWSTR pszSectionName,
                        PBOOLEAN pbContinue
                        );

DWORD
CltrParseConfigFile(
    PCWSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    );

#endif /* __CLTR_CFG_H__ */
