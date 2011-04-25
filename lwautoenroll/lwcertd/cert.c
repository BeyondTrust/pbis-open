/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#include "lwcertd.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <ldap.h>

#include <lw/types.h>

#include <bail.h>
#include <ldap_util.h>

#include <lwerror.h>
#include <lwldap.h>
#include <lwkrb5.h>
#include <lwstr.h>

#include <lwhash.h>

#include <lsa/ad.h>
#include <lsa/lsa.h>

#define LDAP_QUERY "(objectClass=certificationAuthority)"
#define LDAP_BASE "cn=NTAuthCertificates,cn=Public Key Services,cn=Services,cn=Configuration"
#define TRUSTED_CERT_DIR            CACHEDIR "/trusted_certs"
#define KRB5_CACHE                  "MEMORY:lwcertd_krb5_cc"
#define TRUSTED_CERT_PATH_MAX       (sizeof(TRUSTED_CERT_DIR) + NAME_MAX + 2)

/* Markers for cert files that are found/not found in AD. */
static int found, notfound;

static PSTR attributeList[] =
{
    "caCertificate",
    NULL
};

static int
PEMPasswordCallBack(char *buf, int size, int rwflag, void *data)
{
    /*
     * None of the PEM operations we use should require a password,
     * and we wouldn't know what it is if they did, so just return
     * false.
     */
    return 0;
}

DWORD
GetTrustedCertificates(
        void
        )
{
    HANDLE lsaConnection = (HANDLE) NULL;
    HANDLE ldapConnection = (HANDLE) NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    PSTR machineUPN = NULL;
    PSTR domainDn = NULL;
    krb5_context krb5context = NULL;
    krb5_ccache krb5ccache = NULL;
    krb5_principal krb5principal = NULL;
    LDAP *pLdap = NULL;
    LDAPMessage *pLdapResults = NULL;
    LDAPMessage *pLdapResult = NULL;
    X509 *cert = NULL;
    const EVP_MD *pDigestAlgorithm = EVP_sha1();
    static LW_BOOL initialized = LW_FALSE;
    LW_HASH_TABLE* pCertFiles = NULL;
    DIR* pDir = NULL;
    struct dirent*  pDirEntry = NULL;
    LW_BOOL downloadedCerts = LW_FALSE;
    int ret;
    DWORD error = LW_ERROR_SUCCESS;

    if (!initialized)
    {
        /* Use lsass' kerberos credentials cache (machine credentials). */
        error = LwKrb5SetProcessDefaultCachePath(KRB5_CACHE);
        BAIL_ON_LW_ERROR(error);

        /* Make sure the trusted certificate directory exists. */
        ret = mkdir(TRUSTED_CERT_DIR, 0755);
        BAIL_ON_UNIX_ERROR(ret == -1 && errno != EEXIST);

        initialized = LW_TRUE;
    }

    pDir = opendir(TRUSTED_CERT_DIR);
    BAIL_ON_UNIX_ERROR(pDir == NULL);

    error = LwHashCreate(
                    42,
                    LwHashStringCompare,
                    LwHashStringHash,
                    LwHashFreeStringKey,
                    NULL,
                    &pCertFiles);
    BAIL_ON_LW_ERROR(error);

    while ((pDirEntry = readdir(pDir)) != NULL)
    {
        if (pDirEntry->d_name[0] == '.' &&
            (pDirEntry->d_name[1] == '\0' ||
             (pDirEntry->d_name[1] == '.' &&
                  pDirEntry->d_name[2] == '\0')))
        {
            continue;
        }

        LwHashSetValue(pCertFiles, strdup(pDirEntry->d_name), &notfound);
    }

    error = LsaOpenServer(&lsaConnection);
    BAIL_ON_LW_ERROR(error);

    error = LsaAdGetMachinePasswordInfo(lsaConnection, NULL, &pPasswordInfo);
    BAIL_ON_LW_ERROR(error);

    error = LwLdapConvertDomainToDN(
                pPasswordInfo->Account.DnsDomainName,
                &domainDn);
    BAIL_ON_LW_ERROR(error);

    error = LwAllocateStringPrintf(
                &machineUPN,
                "%s@%s",
                pPasswordInfo->Account.SamAccountName,
                pPasswordInfo->Account.DnsDomainName);
    BAIL_ON_LW_ERROR(error);

    LwStrToUpper(machineUPN);

    ret = krb5_init_context(&krb5context);
    BAIL_ON_KRB_ERROR(ret, krb5context);

    ret = krb5_parse_name(krb5context, machineUPN, &krb5principal);
    BAIL_ON_KRB_ERROR(ret, krb5context);

    ret = krb5_cc_resolve(krb5context, KRB5_CACHE, &krb5ccache);
    BAIL_ON_KRB_ERROR(ret, krb5context);

    ret = krb5_cc_initialize(krb5context, krb5ccache, krb5principal);
    BAIL_ON_KRB_ERROR(ret, krb5context);

    error = LwKrb5InitializeCredentials(
                machineUPN,
                pPasswordInfo->Password,
                KRB5_CACHE,
                NULL);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapConnect(
                pPasswordInfo->Account.DnsDomainName,
                &ldapConnection);
    BAIL_ON_LW_ERROR(error);

    error = LwAutoEnrollLdapSearch(
                ldapConnection,
                domainDn,
                LDAP_BASE,
                LDAP_SCOPE_SUBTREE,
                LDAP_QUERY,
                attributeList,
                &pLdapResults);
    BAIL_ON_LW_ERROR(error);

    pLdap = LwLdapGetSession(ldapConnection);

    for (pLdapResult = LwLdapFirstEntry(ldapConnection, pLdapResults);
            pLdapResult != NULL;
            pLdapResult = LwLdapNextEntry(ldapConnection, pLdapResult))
    {
        struct berval **ppValues;
        int numValues;
        int value;

        ppValues = ldap_get_values_len(pLdap, pLdapResult, attributeList[0]);
        numValues = ldap_count_values_len(ppValues);

        for (value = 0; value < numValues; ++value)
        {
            const unsigned char *pCertData;
            DWORD hash;
            int sequence = 0;
            char path[TRUSTED_CERT_PATH_MAX];
            const char *file;
            unsigned int uCertDigestLen = 0;
            unsigned char certDigest[EVP_MAX_MD_SIZE];

            pCertData = (const unsigned char *)
                ppValues[value]->bv_val;
            cert = d2i_X509(NULL, &pCertData,
                    ppValues[value]->bv_len);
            hash = X509_subject_name_hash(cert);

            /*
             * Look for matching filenames:
             *     hash.0, hash.1, ...
             *
             * For any matches, compare the certs to see
             * if they're the same.  If so, go to the next
             * cert.
             *
             * If no existing cert file is found, create
             * it as hash.N where N is the first number
             * that doesn't already exist.
             */
            while (1)
            {
                unsigned int uFileDigestLen;
                unsigned char fileDigest[EVP_MAX_MD_SIZE];
                X509 *fileCert;
                FILE *fp;
                int result;

                snprintf(path, sizeof(path), "%s/%08x.%d",
                        TRUSTED_CERT_DIR, hash, sequence);
                file = strrchr(path, '/') + 1;

                if (LwHashGetValue(
                        pCertFiles,
                        file,
                        NULL) != LW_ERROR_SUCCESS)
                {
                    /* File does not exist. */
                    break;
                }

                fp = fopen(path, "r");
                if (fp == NULL)
                {
                    LW_RTL_LOG_ERROR("fopen(%s) failed: %s",
                            path, strerror(errno));
                    continue;
                }

                fileCert = PEM_read_X509(fp, NULL,
                        PEMPasswordCallBack, NULL);
                fclose(fp);
                if (fileCert == NULL)
                {
                    continue;
                }

                result = X509_digest(fileCert, pDigestAlgorithm,
                            fileDigest, &uFileDigestLen);
                X509_free(fileCert);
                if (!result)
                {
                    continue;
                }

                if (uCertDigestLen == 0)
                {
                    if (!X509_digest(cert, pDigestAlgorithm,
                                certDigest, &uCertDigestLen))
                    {
                        continue;
                    }
                }

                if (uFileDigestLen == uCertDigestLen &&
                        memcmp(fileDigest, certDigest,
                            uCertDigestLen) == 0)
                {
                    /*
                     * The existing file already has
                     * the cert.
                     */
                    LwHashSetValue(
                            pCertFiles,
                            strdup(file),
                            &found);
                    sequence = -1;
                    break;
                }

                ++sequence;
            }

            if (sequence != -1)
            {
                FILE *fp;

                fp = fopen(path, "w");
                if (fp == NULL)
                {
                    LW_RTL_LOG_ERROR("Cannot create %s", path);
                }
                else
                {
                    if (PEM_write_X509(fp, cert))
                    {
                        LwHashSetValue(
                                pCertFiles,
                                strdup(file),
                                &found);
                    }
                    else
                    {
                        /*
                         * Write failed - don't leave a
                         * bogus file around.
                         */
                        LW_RTL_LOG_ERROR(
                            "PEM_write_X509 Write failed to %s",
                            path);
                        unlink(path);
                    }

                    fclose(fp);
                }
            }

            X509_free(cert);
            cert = NULL;
        }
    }

    if (pCertFiles && downloadedCerts)
    {
        LW_HASH_ITERATOR it;
        LW_HASH_ENTRY *pCertFile;

        error = LwHashGetIterator(pCertFiles, &it);
        BAIL_ON_LW_ERROR(error);

        while ((pCertFile = LwHashNext(&it)) != NULL)
        {
            if (pCertFile->pValue == &notfound)
            {
                char path[TRUSTED_CERT_PATH_MAX];

                if (snprintf(
                        path,
                        sizeof(path),
                        "%s/%s",
                        TRUSTED_CERT_DIR,
                        (const char *) pCertFile->pKey)
                    >= sizeof(path))
                {
                    LW_RTL_LOG_ERROR(
                        "path %s/%s too long",
                        TRUSTED_CERT_DIR,
                        (const char *) pCertFile->pKey);
                }
                else if (unlink(path) == -1)
                {
                    LW_RTL_LOG_ERROR(
                        "Could not unlink %s: %s",
                        path,
                        strerror(errno));
                }
            }
        }
    }

cleanup:
    if (lsaConnection)
    {
        LsaCloseServer(lsaConnection);
    }

    if (pPasswordInfo)
    {
        LsaAdFreeMachinePasswordInfo(pPasswordInfo);
    }

    if (ldapConnection != (HANDLE) NULL)
    {
        LwLdapCloseDirectory(ldapConnection);
    }

    if (cert)
    {
        X509_free(cert);
    }

    if (pLdapResults)
    {
        ldap_msgfree(pLdapResults);
    }

    if (pLdapResult)
    {
        ldap_msgfree(pLdapResult);
    }

    if (pDir)
    {
        closedir(pDir);
    }

    if (pCertFiles)
    {
        LwHashSafeFree(&pCertFiles);
    }

    if (krb5context)
    {
        if (krb5principal)
        {
            krb5_free_principal(krb5context, krb5principal);
        }

        if (krb5ccache)
        {
            krb5_cc_close(krb5context, krb5ccache);
        }

        krb5_free_context(krb5context);
    }

    LW_SAFE_FREE_STRING(domainDn);
    LW_SAFE_FREE_STRING(machineUPN);

    return error;
}
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
