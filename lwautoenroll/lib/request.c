#include <lwautoenroll/lwautoenroll.h>

DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        )
{
    return 0;
}

DWORD
LwAutoEnrollGetRequestStatus(
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        )
{
    return 0;
}
