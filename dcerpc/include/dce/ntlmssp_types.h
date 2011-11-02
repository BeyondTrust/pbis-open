#ifndef _NTLMSSP_TYPES_H
#define _NTLMSSP_TYPES_H	1

/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a"
#define GSS_MECH_NTLM_LEN   10

/* 1.3.6.1.4.1.27433.3.1 */
#define GSS_CRED_OPT_PW     "\x2b\x06\x01\x04\x01\x81\xd6\x29\x03\x01"
#define GSS_CRED_OPT_PW_LEN 10

#define RPC_CN_PKT_SIZEOF_NTLMSSP_AUTH_TLR   (16)

#define SEC_WINNT_AUTH_IDENTITY_ANSI     1

/*
 * NTLM Credentials to pass to gss routines.
 * This is a copy (to avoid cirular dependency) of SEC_WINNT_AUTH_IDENTITY
 * type from lsass/include/ntlm/sspintlm.h
 */
typedef struct rpc_ntlmssp_auth_ident
{
    PCHAR User;
    DWORD UserLength;
    PCHAR Domain;
    DWORD DomainLength;
    PCHAR Password;
    DWORD PasswordLength;
    DWORD Flags;
} rpc_ntlmssp_auth_ident_t, *rpc_ntlmssp_auth_ident_t_p;

#endif /* _NTLMSSP_TYPES_H */
