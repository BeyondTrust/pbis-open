#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>
#include <ssl_util.h>

#include <openssl/err.h>
#include <openssl/pem.h>

#include <lwerror.h>
#include <lwstr.h>

#include <string.h>
#include <unistd.h>

#define MY_ERROR_BASE               30000
#define MY_ERROR_NO_TEMPLATE_FOUND  MY_ERROR_BASE+0
#define MY_ERROR_MAX                MY_ERROR_BASE+1

static const char *MyErrorNames[] = {
    "Template not found",
};

static const char *
MyErrorToName(DWORD error)
{
    if (error >= MY_ERROR_BASE && error < MY_ERROR_MAX)
    {
        return MyErrorNames[error - MY_ERROR_BASE];
    }
    else
    {
        return "Unknown Error";
    }
}

#define BAIL_WITH_MY_ERROR(_error, ...) \
    do { \
        error = _error; \
        BAIL(" Error code: %d (%s)" _BAIL_FORMAT_STRING(__VA_ARGS__), \
            _error, MyErrorToName(_error), _BAIL_FORMAT_ARGS(__VA_ARGS__)); \
    } while(0)

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
        IN PCSTR templateName,
        OUT X509 **ppCertificate
    )
{
    PLW_AUTOENROLL_TEMPLATE pTemplates = 0;
    DWORD numTemplates;
    DWORD template;
    PSTR url = NULL;
    DWORD requestId;
    DWORD error = LW_ERROR_SUCCESS;

    error = LwAutoEnrollGetTemplateList(NULL, &pTemplates, &numTemplates);
    BAIL_ON_LW_ERROR(error);

    for (template = 0; template < numTemplates; ++template)
    {
        if (!strcmp(pTemplates[template].name, templateName))
        {
            break;
        }
    }

    if (template == numTemplates)
    {
        BAIL_WITH_MY_ERROR(MY_ERROR_NO_TEMPLATE_FOUND);
    }

    error = LwAutoEnrollRequestCertificate(
                NULL,
                &pTemplates[template],
                &url,
                ppKeyPair,
                ppCertificate,
                &requestId);
    while (error == ERROR_CONTINUE)
    {
        /*
         * A pending status usually means human intervention is
         * required at the CA, so don't poll too frequently.
         */
        sleep(300);
        error = LwAutoEnrollGetRequestStatus(
                    NULL,
                    url,
                    requestId,
                    ppCertificate);
    }

    BAIL_ON_LW_ERROR(error);

cleanup:
    if (pTemplates)
    {
        LwAutoEnrollFreeTemplateList(pTemplates, numTemplates);
    }

    LW_SAFE_FREE_STRING(url);

    return error;
}

int
main(int argc, char * const *argv)
{
    EVP_PKEY *pKeyPair = NULL;
    BIO *pStdoutBio = NULL;
    X509 *pCertificate = NULL;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s template\n", argv[0]);
        exit(1);
    }

    LogInit();
    ERR_load_crypto_strings();

    error = GetCertificate(&pKeyPair, argv[1], &pCertificate);
    BAIL_ON_LW_ERROR(error);

    pStdoutBio = BIO_new(BIO_s_file());
    BAIL_ON_SSL_ERROR(pStdoutBio == NULL);

    sslResult = BIO_set_fp(pStdoutBio, stdout, BIO_NOCLOSE);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = PEM_write_bio_X509(pStdoutBio, pCertificate);
    BAIL_ON_SSL_ERROR(sslResult == 0);

cleanup:
    if (pKeyPair)
    {
        EVP_PKEY_free(pKeyPair);
    }

    if (pCertificate)
    {
        X509_free(pCertificate);
    }

    if (pStdoutBio)
    {
        BIO_free_all(pStdoutBio);
    }

    ERR_free_strings();

    return (error != 0);
}
