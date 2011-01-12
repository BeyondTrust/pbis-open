#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <lwerror.h>

DWORD
LwAutoEnrollRequestCertificate(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate,
        OUT PDWORD pRequestID
        )
{
    X509_REQ *pRequest = NULL;
    X509* pCertificate = NULL;
    DWORD requestID = 0;
    RSA *pRsaKey = NULL;
    int sslResult;
    DWORD error = LW_ERROR_SUCCESS;

    if (*ppKeyPair == NULL)
    {
        *ppKeyPair = EVP_PKEY_new();

        if (*ppKeyPair == NULL)
        {
            BAIL_WITH_LW_ERROR(LW_ERROR_OUT_OF_MEMORY);
        }

        pRsaKey = RSA_generate_key(pTemplate->minimumKeyBits,
                                65537,
                                NULL,
                                NULL);
        BAIL_ON_SSL_ERROR(pRsaKey == NULL);

        sslResult = EVP_PKEY_assign_RSA(*ppKeyPair, pRsaKey);
        BAIL_ON_SSL_ERROR(!sslResult);
    }

    pRequest = X509_REQ_new();
    BAIL_ON_SSL_ERROR(pRequest == NULL);

    sslResult = X509_REQ_set_version(pRequest, 0L);
    BAIL_ON_SSL_ERROR(!sslResult);

    sslResult = X509_REQ_set_pubkey(pRequest, *ppKeyPair);
    BAIL_ON_SSL_ERROR(!sslResult);

    sslResult = X509_REQ_sign(pRequest, *ppKeyPair, EVP_sha1());
    BAIL_ON_SSL_ERROR(!sslResult);

cleanup:
    if (error)
    {
        if (pRequest)
        {
            X509_REQ_free(pRequest);
            pRequest = NULL;
        }

        if (pCertificate)
        {
            X509_free(pCertificate);
            pCertificate = NULL;
        }
    }

    if (pRsaKey)
    {
        RSA_free(pRsaKey);
    }

    *ppCertificate = pCertificate;
    *pRequestID = requestID;

    return error;
}

DWORD
LwAutoEnrollGetRequestStatus(
        IN DWORD RequestID,
        OUT X509 **ppCertificate
        )
{
    return 0;
}
