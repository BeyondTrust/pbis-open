/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog module error codes
 *
 */
#ifndef __EVTERROR_H__
#define __EVTERROR_H__

/* ERRORS */
#define CLTR_ERROR_SUCCESS                   0x0000
#define CLTR_ERROR_INVALID_CONFIG_PATH       0x9001 // 36865
#define CLTR_ERROR_INVALID_PREFIX_PATH       0x9002 // 36866
#define CLTR_ERROR_INSUFFICIENT_BUFFER       0x9003 // 36867
#define CLTR_ERROR_OUT_OF_MEMORY             0x9004 // 36868
#define CLTR_ERROR_INVALID_MESSAGE           0x9005 // 36869
#define CLTR_ERROR_UNEXPECTED_MESSAGE        0x9006 // 36870
#define CLTR_ERROR_NO_SUCH_USER              0x9007 // 36871
#define CLTR_ERROR_DATA_ERROR                0x9008 // 36872
#define CLTR_ERROR_NOT_IMPLEMENTED           0x9009 // 36873
#define CLTR_ERROR_NO_CONTEXT_ITEM           0x900A // 36874
#define CLTR_ERROR_NO_SUCH_GROUP             0x900B // 36875
#define CLTR_ERROR_REGEX_COMPILE_FAILED      0x900C // 36876
#define CLTR_ERROR_NSS_EDIT_FAILED           0x900D // 36877
#define CLTR_ERROR_NO_HANDLER                0x900E // 36878
#define CLTR_ERROR_INTERNAL                  0x900F // 36879
#define CLTR_ERROR_NOT_HANDLED               0x9010 // 36880
#define CLTR_ERROR_UNEXPECTED_DB_RESULT      0x9011 // 36881
#define CLTR_ERROR_INVALID_PARAMETER         0x9012 // 36882
#define CLTR_ERROR_LOAD_LIBRARY_FAILED       0x9013 // 36883
#define CLTR_ERROR_LOOKUP_SYMBOL_FAILED      0x9014 // 36884
#define CLTR_ERROR_INVALID_EVENTLOG          0x9015 // 36885
#define CLTR_ERROR_INVALID_CONFIG            0x9016 // 36886
#define CLTR_ERROR_STRING_CONV_FAILED        0x9017 // 36887
#define CLTR_ERROR_INVALID_DB_HANDLE         0x9018 // 36888
#define CLTR_ERROR_FAILED_CONVERT_TIME       0x9019 // 36889
#define CLTR_ERROR_RPC_EXCEPTION_UPON_OPEN   0x901A // 36890
#define CLTR_ERROR_RPC_EXCEPTION_UPON_CLOSE  0x901B // 36891
#define CLTR_ERROR_OUTOF_MEMORY              0x901C // 36892

#define CLTR_ERROR_MASK(_e_)             (_e_ & 0x9000)

/* WARNINGS */



#define BAIL_ON_CLTR_ERROR(dwError) \
    if (dwError) {                 \
      CLTR_LOG_ERROR("Error at %s:%d. Error [code:%d]", __FILE__, __LINE__, dwError); \
      goto error;                    \
    }

#define BAIL_ON_INVALID_STRING(pszParam)             \
        if ((pszParam) == NULL || (pszParam)[0] == 0) {         \
           dwError = EINVAL; \
           BAIL_ON_CLTR_ERROR(dwError);            \
        }

#define BAIL_ON_SQLITE3_ERROR(dwError, pszError) \
    do { \
        if (dwError) \
        { \
           CLTR_LOG_DEBUG("Sqlite3 error '%hhs' (code = %d)", \
                         CLTR_SAFE_LOG_STRING(pszError), dwError); \
           goto error;                               \
        } \
    } while (0)

#define BAIL_ON_DCE_ERROR(dwError)                         \
    if (dwError != ERROR_SUCCESS)                        \
    {                                                      \
    CLTR_LOG_ERROR("DCE Error at %s:%d. Error [code:%d]", __FILE__, __LINE__, dwError); \
        goto error;                                         \
    }

#endif /* __EVTERROR_H__ */

