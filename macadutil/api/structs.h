#ifndef __GPLDAPSTRUCT_H__
#define __GPLDAPSTRUCT_H__

typedef struct _ADU_DIRECTORY_CONTEXT
{
    LDAP *ld;

} ADU_DIRECTORY_CONTEXT, *PADU_DIRECTORY_CONTEXT;

typedef struct _ADU_CRED_CONTEXT
{
    PSTR                pszPrincipalName;
    PSTR                pszCachePath;
    BOOLEAN             bDestroyCachePath;

    LW_PIO_CREDS pAccessToken;

} ADU_CRED_CONTEXT, *PADU_CRED_CONTEXT;

#endif /* __GPLDAPSTRUCT_H__ */
