/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include <lwautoenroll/lwautoenroll.h>
#include <bail.h>
#include <ssl_util.h>

#include <openssl/err.h>
#include <openssl/pem.h>

#include <lwerror.h>
#include <lwstr.h>

#include <string.h>
#include <unistd.h>

#define MY_ERROR_BASE                   30000
#define MY_ERROR_NO_TEMPLATE_FOUND      MY_ERROR_BASE+0
#define MY_ERROR_INVALID_SUBJECT_NAME   MY_ERROR_BASE+1
#define MY_ERROR_MAX                    MY_ERROR_BASE+2

static const char *MyErrorNames[] = {
    "Template not found",
    "Invalid subject name",
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

static
VOID
LogFunc(
    LW_IN LW_OPTIONAL LW_PVOID Context,
    LW_IN LW_RTL_LOG_LEVEL Level,
    LW_IN LW_OPTIONAL LW_PCSTR ComponentName,
    LW_IN LW_PCSTR FunctionName,
    LW_IN LW_PCSTR FileName,
    LW_IN LW_ULONG LineNumber,
    LW_IN LW_PCSTR Format,
    LW_IN ...
    )
{
    va_list args;

    va_start(args, Format);
    vfprintf(stderr, Format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

static DWORD
LogInit(void)
{
    LwRtlLogSetLevel(LW_RTL_LOG_LEVEL_DEBUG);
    LwRtlLogSetCallback(LogFunc, NULL);
    return 0;
}

static DWORD
ParseSubjectName(
        IN PSTR nameString,
        OUT X509_NAME **ppSubjectName
        )
{
    PSTR type = NULL;
    PSTR value = NULL;
    PSTR savePtr = NULL;
    X509_NAME *pSubjectName = NULL;
    int nid;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    pSubjectName = X509_NAME_new();
    BAIL_ON_SSL_ERROR(pSubjectName == NULL);

    while ((type = strtok_r(nameString, "=", &savePtr)) != NULL)
    {
        nameString = NULL; // For next call to strtok();
        nid = OBJ_txt2nid(type);
        BAIL_ON_SSL_ERROR(
            nid == NID_undef,
            ": DN component type '%s' not found",
            type);

        value = strtok_r(NULL, ",", &savePtr);
        if (value == NULL)
        {
            value = savePtr + 1;
            if (*value == '\0')
            {
                BAIL_WITH_MY_ERROR(
                    MY_ERROR_INVALID_SUBJECT_NAME,
                    ": No value specified for DN component '%s'",
                    type);
            }
        }

        sslResult = X509_NAME_add_entry_by_NID(
                        pSubjectName,
                        nid,
                        MBSTRING_ASC,
                        (unsigned char *) value,
                        -1,
                        -1,
                        0);
        BAIL_ON_SSL_ERROR(sslResult == 0);
    }

cleanup:
    if (error)
    {
        if (pSubjectName)
        {
            X509_NAME_free(pSubjectName);
            pSubjectName = NULL;
        }
    }

    *ppSubjectName = pSubjectName;
    return error;
}

static DWORD
GetCertificate(
        IN PCSTR templateName,
        IN OPTIONAL X509_NAME *pSubjectName,
        IN OUT EVP_PKEY **ppKeyPair,
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
                &pTemplates[template],
                pSubjectName,
                NULL,
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
                    url,
                    requestId,
                    NULL,
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
    X509_NAME *pSubjectName = NULL;
    X509 *pCertificate = NULL;
    int sslResult = 0;
    DWORD error = LW_ERROR_SUCCESS;

    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: %s template [subject_name]\n", argv[0]);
        exit(1);
    }

    LogInit();
    ERR_load_crypto_strings();

    if (argc == 3)
    {
        error = ParseSubjectName(argv[2], &pSubjectName);
        BAIL_ON_LW_ERROR(error);
    }

    error = GetCertificate(argv[1], pSubjectName, &pKeyPair, &pCertificate);
    BAIL_ON_LW_ERROR(error);

    pStdoutBio = BIO_new(BIO_s_file());
    BAIL_ON_SSL_ERROR(pStdoutBio == NULL);

    sslResult = BIO_set_fp(pStdoutBio, stdout, BIO_NOCLOSE);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = PEM_write_bio_PrivateKey(
                    pStdoutBio,
                    pKeyPair,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL);
    BAIL_ON_SSL_ERROR(sslResult == 0);

    sslResult = PEM_write_bio_X509(pStdoutBio, pCertificate);
    BAIL_ON_SSL_ERROR(sslResult == 0);

cleanup:
    if (pSubjectName)
    {
        X509_NAME_free(pSubjectName);
        pSubjectName = NULL;
    }

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
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
