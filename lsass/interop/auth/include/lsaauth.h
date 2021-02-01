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
 *        lsaauth.h
 *
 * Abstract:
 *
 *        Common Authentication Library (BeyondTrust LSASS)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSA_AUTH_H__
#define __LSA_AUTH_H__

#define MODULE_NAME_SPECIFIC "lsass"
#define MODULE_NAME "pam_" MODULE_NAME_SPECIFIC

#define MOTD_FILE __LW_MOTD_FILE__
#define MOTD_MAX_SIZE __LW_MOTD_MAX_SIZE__

/* Custom data keys */
#define PAM_LSASS_OLDAUTHTOK        "PAM_LSASS_OLDAUTHTOK"
#define PAM_LSASS_SMART_CARD_PIN    "PAM_LSASS_SMART_CARD_PIN"
#define PAM_LSASS_SMART_CARD_READER "PAM_LSASS_SMART_CARD_READER"
#define PAM_LSASS_EXPIRE_WARNING_DONE "PAM_LSASS_EXPIRE_WARNING_DONE"

/*
 * AIX declares functions like pam_get_data to take void* pointer
 * as the last argument, while Linux and Mac OS takes const void*.
 * This causes build failures when compiling with -Werror enabled.
 * Use macro just like any other cast e.g.: (PAM_GET_ITEM_TYPE)
 */
#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)

#define PAM_GET_ITEM_TYPE             void**
#define PAM_GET_DATA_TYPE             void**
#define PPCHAR_ARG_CAST               char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         struct pam_message**

#elif defined(__LWI_SOLARIS__)

#if defined(PAM_GET_ITEM_TAKES_CONST_ITEM_ARG)
#define PAM_GET_ITEM_TYPE             const void**
#else
#define PAM_GET_ITEM_TYPE             void**
#endif
#if defined(PAM_GET_DATA_TAKES_CONST_DATA_ARG)
#define PAM_GET_DATA_TYPE             const void**
#else
#define PAM_GET_DATA_TYPE             void**
#endif
#define PPCHAR_ARG_CAST               char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         struct pam_message**

#elif defined(__LWI_FREEBSD__) || defined(__LWI_NETBSD__)

#define PAM_GET_ITEM_TYPE             const void**

#if defined(PAM_GET_DATA_TAKES_CONST_DATA_ARG)
#define PAM_GET_DATA_TYPE             const void**
#else
#define PAM_GET_DATA_TYPE             void**
#endif

#define PPCHAR_ARG_CAST               const char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         const struct pam_message**

#elif defined(__LWI_DARWIN_X64__)

#define PAM_GET_ITEM_TYPE             const void**
#define PAM_GET_DATA_TYPE             const void**
#define PPCHAR_ARG_CAST               const char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         const struct pam_message**

#else

#define PAM_GET_ITEM_TYPE             const void**
#define PAM_GET_DATA_TYPE             const void**
#define PPCHAR_ARG_CAST               const char**
#define PAM_MESSAGE_MSG_TYPE          const char*
#define PAM_CONV_2ND_ARG_TYPE         const struct pam_message**

#endif

extern DWORD gdwLogLevel;

/*
 * Log levels
 */
#if defined(EXPLICIT_OPEN_CLOSE_LOG)

#define LOG_FMT_MODULE_NAME "%s"
#define LOG_MODULE_NAME     ""

#else

#define LOG_FMT_MODULE_NAME "[module:%s]"
#define LOG_MODULE_NAME MODULE_NAME

#endif /* defined(EXPLICIT_OPEN_CLOSE_LOG) */

#define LSA_LOG_PAM_ALWAYS(szFmt, ...) \
    do { \
        LsaPamLogMessage(LSA_PAM_LOG_LEVEL_ALWAYS, \
                         LOG_FMT_MODULE_NAME szFmt, \
                         LOG_MODULE_NAME, \
                         ## __VA_ARGS__); \
    } while(0)

#define LSA_LOG_PAM_ERROR(szFmt, ...) \
    do { \
        if (gdwLogLevel >= LSA_PAM_LOG_LEVEL_ERROR) { \
            LsaPamLogMessage(LSA_PAM_LOG_LEVEL_ERROR, \
                             LOG_FMT_MODULE_NAME szFmt, \
                             LOG_MODULE_NAME, \
                             ## __VA_ARGS__); \
        } \
    } while(0)

#define LSA_LOG_PAM_WARNING(szFmt, ...) \
    do { \
        if (gdwLogLevel >= LSA_PAM_LOG_LEVEL_WARNING) { \
            LsaPamLogMessage(LSA_PAM_LOG_LEVEL_WARNING, \
                             LOG_FMT_MODULE_NAME szFmt, \
                             LOG_MODULE_NAME, \
                             ## __VA_ARGS__); \
        } \
    } while(0)

#define LSA_LOG_PAM_INFO(szFmt, ...) \
    do { \
        if (gdwLogLevel >= LSA_PAM_LOG_LEVEL_INFO)    { \
            LsaPamLogMessage(LSA_PAM_LOG_LEVEL_INFO, \
                             LOG_FMT_MODULE_NAME szFmt, \
                             LOG_MODULE_NAME, \
                             ## __VA_ARGS__); \
        } \
    } while(0)

#define LSA_LOG_PAM_VERBOSE(szFmt, ...) \
    do { \
        if (gdwLogLevel >= LSA_PAM_LOG_LEVEL_VERBOSE) { \
            LsaPamLogMessage(LSA_PAM_LOG_LEVEL_VERBOSE, \
                             LOG_FMT_MODULE_NAME szFmt, \
                             LOG_MODULE_NAME, \
                             ## __VA_ARGS__); \
        } \
    } while(0)

#define LSA_LOG_PAM_DEBUG(szFmt, ...) \
    do { \
        if (gdwLogLevel >= LSA_PAM_LOG_LEVEL_DEBUG) { \
           LsaPamLogMessage(LSA_PAM_LOG_LEVEL_DEBUG, \
                            LOG_FMT_MODULE_NAME "[%s() %s:%d] " szFmt, \
                            LOG_MODULE_NAME, \
                            __FUNCTION__, \
                            __FILE__, \
                            __LINE__, \
                            ## __VA_ARGS__); \
        } \
    } while (0)

#ifdef BAIL_ON_LSA_ERROR
#undef BAIL_ON_LSA_ERROR
#endif

#define BAIL_ON_LSA_ERROR(err)                      \
    do {                                            \
        if ((err))                                  \
        {                                           \
            LSA_LOG_PAM_DEBUG("error %d", (err));   \
            goto error;                             \
        }                                           \
    } while (0)

void
LsaPamLogMessage(
    DWORD dwLogLevel,
    PSTR pszFormat, ...
    );

void
LsaPamSetLogLevel(
    DWORD dwLogLevel
    );

void
LsaPamCloseLog(
    void
    );

DWORD
LsaPamGetConfig(
    PLSA_PAM_CONFIG* ppConfig
    );

VOID
LsaPamFreeConfig(
    PLSA_PAM_CONFIG pConfig
    );

BOOLEAN
LsaShouldIgnoreGroup(
    PCSTR pszName
    );

BOOLEAN
LsaShouldIgnoreUser(
    PCSTR pszName
    );

BOOLEAN
LsaShouldIgnoreUserInfo(
    PVOID        pUserInfo
    );

BOOLEAN
LsaShouldIgnoreGroupInfo(
    PVOID        pGroupInfo
    );

VOID
LsaFreeIgnoreHashes(VOID);

#endif /* __LSA_AUTH_H__ */


