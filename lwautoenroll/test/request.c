#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>

#include <openssl/err.h>

#include <lwerror.h>

#include <string.h>
#include <unistd.h>

#define MY_ERROR_NO_TEMPLATE_FOUND  30001

static BOOL
MyTemplateCheck(
    PLW_AUTOENROLL_TEMPLATE pTemplate
    )
{
    return (strcmp(pTemplate->name, "UserSignature") == 0);
}

static VOID
LogFunc(LwLogLevel level, PVOID pUserData, PCSTR pszMessage)
{
    fprintf(stderr, "%s\n", pszMessage);
}

static DWORD
LogInit(void)
{
    return LwSetLogFunction(LW_LOG_LEVEL_DEBUG, LogFunc, NULL);
}

static DWORD
GetCertificate(
        IN OUT EVP_PKEY **ppKeyPair,
        OUT X509 **ppCertificate
    )
{
    PLW_AUTOENROLL_TEMPLATE pTemplates = 0;
    DWORD numTemplates;
    DWORD template;
    DWORD requestID;
    DWORD error = LW_ERROR_SUCCESS;

    error = LwAutoEnrollGetTemplateList(&pTemplates, &numTemplates);
    BAIL_ON_LW_ERROR(error);

    for (template = 0; template < numTemplates; ++template)
    {
        if (MyTemplateCheck(&pTemplates[template]))
        {
            break;
        }
    }

    if (template == numTemplates)
    {
        BAIL_WITH_LW_ERROR(MY_ERROR_NO_TEMPLATE_FOUND);
    }

    error = LwAutoEnrollRequestCertificate(
                &pTemplates[template],
                ppKeyPair,
                ppCertificate,
                &requestID);
    while (error == ERROR_CONTINUE)
    {
        /*
         * A pending status usually means human intervention is
         * required at the CA, so don't poll too frequently.
         */
        sleep(300);
        error = LwAutoEnrollGetRequestStatus(
                    requestID,
                    ppCertificate);
    }

    BAIL_ON_LW_ERROR(error);

cleanup:
    if (pTemplates)
    {
        LwAutoEnrollFreeTemplateList(pTemplates, numTemplates);
    }

    return error;
}

int
main()
{
    EVP_PKEY *pKeyPair = NULL;
    X509 *pCertificate = NULL;
    DWORD error = LW_ERROR_SUCCESS;

    LogInit();
    ERR_load_crypto_strings();

    error = GetCertificate(&pKeyPair, &pCertificate);

    if (pKeyPair)
    {
        EVP_PKEY_free(pKeyPair);
    }

    if (pCertificate)
    {
        X509_free(pCertificate);
    }

    ERR_free_strings();

    return (error != 0);
}
