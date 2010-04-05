#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _NET_SHARE_INFO0
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO0, *PNET_SHARE_INFO0;

typedef struct _NET_SHARE_INFO1
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO1, *PNET_SHARE_INFO1;

typedef struct _NET_SHARE_INFO2
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO2, *PNET_SHARE_INFO2;

typedef struct _NET_SHARE_INFO501
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO501, *PNET_SHARE_INFO501;

typedef struct _NET_SHARE_INFO502
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO502, *PNET_SHARE_INFO502;

typedef struct _NET_SHARE_INFO503
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} NET_SHARE_INFO503, *PNET_SHARE_INFO503;

typedef struct _NFSSVC_CONFIG
{
    pthread_mutex_t mutex;

    /* path to lsarpc server socket for local procedure calls */
    CHAR szLsaLpcSocketPath[PATH_MAX + 1];

} NFSSVC_CONFIG, *PNFSSVC_CONFIG;

typedef struct _NFSSVC_RUNTIME_GLOBALS
{
    pthread_mutex_t mutex;

    NFSSVC_CONFIG   config;                      /* configuration settings */

    dcethread*      pRpcListenerThread;

    rpc_binding_vector_p_t pServerBinding;
    rpc_binding_vector_p_t pWkstaBinding;
    rpc_binding_vector_p_t pRegistryBinding;

    DWORD           dwStartAsDaemon;             /* Should start as daemon */

    LWIO_LOG_TARGET logTarget;                   /* where are we logging   */

    LWIO_LOG_LEVEL  maxAllowedLogLevel;          /* how much logging ?     */

    CHAR            szLogFilePath[PATH_MAX + 1]; /* log file path */

    BOOLEAN         bProcessShouldExit;          /* Process termination flag */

    DWORD           dwExitCode;                  /* Process Exit Code */

    PSECURITY_DESCRIPTOR_ABSOLUTE pServerSecDesc;

    GENERIC_MAPPING genericMapping;

} NFSSVC_RUNTIME_GLOBALS, *PNFSSVC_RUNTIME_GLOBALS;

#endif /* __STRUCTS_H__ */
