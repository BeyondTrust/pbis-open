#ifndef __DEFS_H__
#define __DEFS_H__

#define DAEMON_NAME "nfssvcd"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

#define DEFAULT_LSALPC_SOCKET_PATH CACHEDIR "/rpc/lsass"

#define NFS_DRIVER_NAME_W { '\\', 's', 'r', 'v', 0 }

#define NFS_IOCTL_ADD_SHARE 0
#define NFS_IOCTL_SET_SHARE 1

#define ERROR_NOT_SUPPORTED 50

/* Work around lack of proper Win32 error codes for now */

#define WIN32_ERROR_SUCCESS                     0x00000000
#define WIN32_ERROR_INVALID_FUNCTION            0x00000001
#define WIN32_ERROR_FILE_NOT_FOUND              0x00000002
#define WIN32_ERROR_UNKNOWN_LEVEL               0x0000007c

#define SECURITY_UNMAPPED_UNIX_AUTHORITY    { 0, 0, 0, 0, 0, 22 }
#define SECURITY_UNMAPPED_UNIX_UID_RID      1
#define SECURITY_UNMAPPED_UNIX_RID_COUNT    2


#endif /* __DEFS_H__ */

