#ifndef __LWAUTOENROLL_H__
#define __LWAUTOENROLL_H__

#include <openssl/x509v3.h>
#include <lw/attrs.h>
#include <lw/types.h>

typedef struct _LW_AUTOENROLL_TEMPLATE
{
        PSTR                    name;
        DWORD                   keyUsage;
        EXTENDED_KEY_USAGE      *extendedKeyUsage;
} LW_AUTOENROLL_TEMPLATE, *PLW_AUTOENROLL_TEMPLATE;

LW_BEGIN_EXTERN_C

DWORD
LwAutoEnrollGetTemplateList(
        OUT PLW_AUTOENROLL_TEMPLATE *ppTemplateList,
        OUT PDWORD pNumTemplates
        );

DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        );

DWORD
LwAutoEnrollGetRequestStatus(
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        );

VOID
LwAutoEnrollFreeTemplateList(
        IN PLW_AUTOENROLL_TEMPLATE pTemplateList,
        IN DWORD NumTemplates
        );

LW_END_EXTERN_C

#endif /* __LWAUTOENROLL_H */
