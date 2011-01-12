#ifndef __LWAUTOENROLL_H__
#define __LWAUTOENROLL_H__

#include <openssl/x509v3.h>

#include <lw/attrs.h>
#include <lw/types.h>

typedef struct _LW_AUTOENROLL_TEMPLATE
{
        PWSTR                   name;
        PWSTR                   csp;
        DWORD                   keyUsage;
        EXTENDED_KEY_USAGE      *extendedKeyUsage;
        STACK_OF(ASN1_OBJECT)   *criticalExtensions;
        unsigned int            minimumKeyBits;
} LW_AUTOENROLL_TEMPLATE, *PLW_AUTOENROLL_TEMPLATE;

LW_BEGIN_EXTERN_C

DWORD
LwAutoEnrollGetTemplateList(
        OUT PLW_AUTOENROLL_TEMPLATE *ppTemplateList,
        OUT PDWORD pNumTemplates
        );

VOID
LwAutoEnrollFreeTemplateList(
        IN PLW_AUTOENROLL_TEMPLATE pTemplateList,
        IN DWORD NumTemplates
        );

DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        );

DWORD
LwAutoEnrollGetRequestStatus(
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        );

LW_END_EXTERN_C

#endif /* __LWAUTOENROLL_H */
