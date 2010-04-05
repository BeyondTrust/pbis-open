/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Group Policy
 *
 *        LSASS Authentication Service Interface
 * 
 * 	      External Variables
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __EXTERNS_H__
#define __EXTERNS_H__

#define GP_PID_DIR                   LOCALSTATEDIR "/run"
#define GP_LSASS_DAEMON_NAME         "lsassd"
#define GP_LSASS_PID_FILE GP_PID_DIR "/" GP_LSASS_DAEMON_NAME ".pid"

#ifndef BAIL_ON_KRB_ERROR

#define BAIL_ON_KRB_ERROR(ctx, ret)                                                      \
    if (ret) {                                                                           \
        if (ctx)  {                                                                      \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);                        \
            if (pszKrb5Error) {                                                          \
                GPA_LOG_VERBOSE("KRB5 Error at %s:%d: %s",__FILE__,__LINE__,pszKrb5Error); \
                krb5_free_error_message(ctx, pszKrb5Error);                              \
            }                                                                            \
        } else {                                                                         \
            GPA_LOG_VERBOSE("KRB5 Error at %s:%d [Code:%d]",__FILE__,__LINE__,ret);      \
        }                                                                                \
        dwError = CENTERROR_GP_KRB5_CALL_FAILED;                                         \
        goto error;                                                                      \
    }

#endif

extern time_t gdwKrbTicketExpiryTime;

extern double gdwExpiryGraceSeconds;

#endif /* __EXTERNS_H__ */

