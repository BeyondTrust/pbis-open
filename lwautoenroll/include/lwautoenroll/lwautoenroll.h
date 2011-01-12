#ifndef __LWAUTOENROLL_H__
#define __LWAUTOENROLL_H__

#include <openssl/x509v3.h>

#include <lw/attrs.h>
#include <lw/types.h>

typedef struct _LW_AUTOENROLL_TEMPLATE
{
        PCSTR                       name;
        PCSTR                       displayName;
        PCSTR                       csp;
        unsigned int                keySize;
        DWORD                       keyUsage;
        DWORD                       enrollmentFlags;
        EXTENDED_KEY_USAGE          *extendedKeyUsage;
        STACK_OF(ASN1_OBJECT)       *criticalExtensions;
	STACK_OF(X509_ATTRIBUTE)    *attributes;
        STACK_OF(X509_EXTENSION)    *extensions;
} LW_AUTOENROLL_TEMPLATE, *PLW_AUTOENROLL_TEMPLATE;

LW_BEGIN_EXTERN_C

DWORD
LwAutoEnrollGetTemplateList(
        IN OPTIONAL PCSTR credentialsCache,
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
        IN OPTIONAL PCSTR credentialsCache,
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        );

DWORD
LwAutoEnrollGetRequestStatus(
        IN OPTIONAL PCSTR credentialsCache,
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        );

LW_END_EXTERN_C

#endif /* __LWAUTOENROLL_H */
