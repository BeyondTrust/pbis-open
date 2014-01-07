#ifndef _DSRSRVDEFS_H_
#define _DSRSRVDEFS_H_


#define LSA_RPC_DIR                            CACHEDIR "/rpc"
#define LSA_DEFAULT_LPC_SOCKET_PATH            LSA_RPC_DIR "/lsass"


#define BAIL_ON_NTSTATUS_ERROR(status)                   \
    do {                                                 \
        if ((status) != STATUS_SUCCESS) {                \
            goto error;                                  \
        }                                                \
    } while (0)



#define BAIL_ON_NO_MEMORY(ptr)                           \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            ntStatus = STATUS_NO_MEMORY;                 \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PTR(ptr)                         \
    do {                                                 \
        if (ptr == NULL) {                               \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid pointer");     \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PARAMETER(cond)                  \
    do {                                                 \
        if (!(cond)) {                                   \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid parameter");   \
            goto error;                                  \
        }                                                \
    } while (0)


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gDsrSrvDataMutex);     \
        if (ret) {                                       \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 1;                                \
        }                                                \
    } while (0)


#define GLOBAL_DATA_UNLOCK(locked)                       \
    do {                                                 \
        int ret = 0;                                     \
        if (!locked) break;                              \
        ret = pthread_mutex_unlock(&gDsrSrvDataMutex);   \
        if (ret && dwError == STATUS_SUCCESS) {          \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)


#endif /* _DSRSRVDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
