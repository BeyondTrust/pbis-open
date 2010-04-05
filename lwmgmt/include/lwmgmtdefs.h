/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmgmt.h
 *
 * Abstract:
 *
 *        Likewise Management Service (LWMGMT)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWMGMTDEFS_H__
#define __LWMGMTDEFS_H__

#ifdef _WIN32

#define RPC_SS_ALLOCATE(dwSize) RpcSsAllocate(dwSize)

#define RPC_SS_FREE(node) RpcSsFree(node)

#define RPC_STRING_BINDING_COMPOSE(protocol, hostname, pBindingString, pStatus) \
    *pStatus = RpcStringBindingCompose(NULL, (RPC_CSTR)(protocol), (RPC_CSTR)(hostname), NULL, NULL, (RPC_CSTR*)(pBindingString))

#define RPC_BINDING_FROM_STRING_BINDING(bindingString, pBindingHandle, pStatus) \
    *pStatus = RpcBindingFromStringBinding((RPC_CSTR)(bindingString), (pBindingHandle))

#define RPC_STRING_FREE(pString, pStatus) \
    *pStatus = RpcStringFree((RPC_CSTR*)pString)

#define RPC_BINDING_FREE(pBindingHandle, pStatus) \
    *pStatus = RpcBindingFree(pBindingHandle)

#else

#define RPC_SS_ALLOCATE(dwSzie) rpc_ss_allocate(dwSize)

#define RPC_SS_FREE(node) rpc_ss_free(node)

#define RPC_STRING_BINDING_COMPOSE(protocol, hostname, pBindingString, pStatus) \
    rpc_string_binding_compose(NULL, (unsigned_char_p_t)(protocol), (unsigned_char_p_t)(hostname), NULL, NULL, (unsigned_char_p_t*)(pBindingString), (unsigned32*)pStatus)

#define RPC_BINDING_FROM_STRING_BINDING(bindingString, pBindingHandle, pStatus) \
    rpc_binding_from_string_binding((unsigned_char_p_t)(bindingString), (pBindingHandle), (unsigned32*)pStatus)

#define RPC_BINDING_SET_AUTH_INFO(BindingHandle, srv_principal, prot_level, \
                  auth_mech, IdHandle, auth_svc, pStatus) \
    rpc_binding_set_auth_info((BindingHandle), (srv_principal), (prot_level), \
                  (auth_mech), (IdHandle), (auth_svc), ((unsigned32*)pStatus));

#define RPC_STRING_FREE(pString, pStatus) \
    rpc_string_free((unsigned_char_p_t*)(pString), ((unsigned32*)pStatus))

#define RPC_BINDING_FREE(pBindingHandle, pStatus) \
    rpc_binding_free((pBindingHandle), ((unsigned32*)pStatus))

#endif

#define TRY       DCETHREAD_TRY
#define CATCH_ALL DCETHREAD_CATCH_ALL(THIS_CATCH)
#define ENDTRY    DCETHREAD_ENDTRY

#define BAIL_ON_LWMGMT_ERROR(dwError) \
    if (dwError) {                    \
        LWMGMT_LOG_ERROR("Error at %s:%d. Error [code:%d]", __FILE__, __LINE__, dwError); \
        goto error;                    \
    }

#define BAIL_ON_DCE_ERROR(dwError) \
    if (dwError != error_status_ok)             \
    {                                           \
        dce_error_string_t errstr;              \
        int error_status;                       \
        dce_error_inq_text(                     \
                        dwError,                \
                        (unsigned char*)errstr, \
                        &error_status);         \
        if (error_status == error_status_ok)    \
        {                                       \
            LWMGMT_LOG_ERROR("DCE Error [0x%8x] Reason [%s]", dwError, errstr);\
        }                                                   \
        else                                                \
        {                                                   \
            LWMGMT_LOG_ERROR("DCE Error [0x%8x]", dwError); \
        }                                                   \
        goto error;                                         \
    }

#define LWMGMT_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              LWMGMTFreeMemory(mem); \
           }                      \
        } while(0);

#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define LWMGMT_SAFE_FREE_STRING(str) \
    do { \
        if (str) { \
            LWMGMTFreeString(str); \
            (str) = NULL; \
        } \
    } while(0)

/*
 * Log levels
 */
#define LOG_LEVEL_ALWAYS  0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_VERBOSE 4
#define LOG_LEVEL_DEBUG   5

 /*
  * Logging targets
  */
 #define LOG_TO_SYSLOG 0
 #define LOG_TO_FILE   1
 #define LOG_TO_CONSOLE 2
 #define LOG_DISABLED  3

#ifdef _WIN32

#define LOG_PRINT(...) ((gBasicLogStreamFD == NULL) ? fprintf(stdout, __VA_ARGS__) : fprintf(gBasicLogStreamFD, __VA_ARGS__))

#define LWMGMT_LOG_ALWAYS(...)                     \
     LOG_PRINT(__VA_ARGS__);

#define LWMGMT_LOG_ERROR(...)                      \
     if (gLogLevel >= LOG_LEVEL_ERROR) {    \
         LOG_PRINT(__VA_ARGS__);  \
     }

#define LWMGMT_LOG_WARNING(...)                    \
     if (gLogLevel >= LOG_LEVEL_WARNING) {  \
        LOG_PRINT(__VA_ARGS__);\
     }

#define LWMGMT_LOG_INFO(...)                       \
     if (gLogLevel >= LOG_LEVEL_INFO)    {  \
         LOG_PRINT(__VA_ARGS__);   \
     }

#define LWMGMT_LOG_VERBOSE(...)                    \
     if (gLogLevel >= LOG_LEVEL_VERBOSE) {  \
         LOG_PRINT(__VA_ARGS__);\
     }

#define LWMGMT_LOG_DEBUG(...)                    \
     if (gLogLevel >= LOG_LEVEL_VERBOSE) {  \
         LOG_PRINT(__VA_ARGS__);\
     }

 #else

 #define LWMGMT_LOG_ALWAYS(szFmt...)                     \
     LWMGMTLogMessage(LOG_LEVEL_ALWAYS, ## szFmt);

 #define LWMGMT_LOG_ERROR(szFmt...)                      \
     if (gLWMGMTLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {    \
         LWMGMTLogMessage(LOG_LEVEL_ERROR, ## szFmt);  \
     }

 #define LWMGMT_LOG_WARNING(szFmt...)                    \
     if (gLWMGMTLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {  \
         LWMGMTLogMessage(LOG_LEVEL_WARNING, ## szFmt);\
     }

 #define LWMGMT_LOG_INFO(szFmt...)                       \
     if (gLWMGMTLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {  \
         LWMGMTLogMessage(LOG_LEVEL_INFO, ## szFmt);   \
     }

 #define LWMGMT_LOG_VERBOSE(szFmt...)                    \
     if (gLWMGMTLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
         LWMGMTLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);\
     }

 #define LWMGMT_LOG_DEBUG(szFmt...)                    \
     if (gLWMGMTLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
         LWMGMTLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);\
     }
 #endif //non-_WIN32

#endif /* __LWMGMTDEFS_H__ */
