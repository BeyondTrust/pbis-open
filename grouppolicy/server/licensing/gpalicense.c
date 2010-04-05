#include "gpalicense2.h"
#include "gpapcntl.h"
#include "gpagent.h"
#include "gpalogger.h"
#include "gpagss.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/err.h>

#include <assert.h>

static
void
freeDatastruct(
    Datastruct* pDataStruct
    )
{
    if (pDataStruct) {
        if (pDataStruct->data) {
            free(pDataStruct->data);
        }
        free(pDataStruct);
    }
}

static
u_int32_t
getInodeTimeStamp()
{
    int status;
    char filename[LICENSE_BUFSIZE];
    int tsfile = -1;
    u_int32_t retval = 0;

    memset(filename, 0, LICENSE_BUFSIZE);

    sprintf(filename, "/etc/samba/ts.tdb");

    tsfile = open(filename, O_RDONLY);
    if(tsfile < 0)
    {
        tsfile = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
        if(tsfile < 0)
            return 0;
        retval = getTimeStamp();
        status = write(tsfile, &retval, sizeof(retval));
    }
    else
    {
        status = read(tsfile, &retval, sizeof(retval));
    }
    close(tsfile);

    return retval;
}

static
u_int16_t
crc(Datastruct *payload)
{
    int i, j;
    u_int16_t crc = 0xFFFF, ch, /*u,*/ v, xor_flag, mask;

    mask = 0x1021;

    /* compute the CRC-CCIT of the buffer --
       http://www.joegeluso.com/software/articles/ccitt.htm */
    for(i = 0; i < payload->size; i++)
    {
        ch = payload->data[i];
        v = 0x80;
        for(j = 0; j < 8; j++)
        {
            if(crc & 0x8000)
                xor_flag = 1;
            else
                xor_flag = 0;
            crc = crc << 1;
            if(ch & v)
                crc += 1;
            if(xor_flag)
                crc = crc ^ mask;

            v = v >> 1;
        }
    }
    for(i = 0; i < 16; i++)
    {
        if(crc & 0x8000)
            xor_flag = 1;
        else
            xor_flag = 0;
        crc = crc << 1;

        if(xor_flag)
            crc = crc ^ mask;
    }

    return crc;
}

DWORD
requireServerOrWorkstationLicense()
{
    /* TODO -- this will have logic to figure out whether we
       should require a server or a workstation license -- based
       on the distro, for example. There will be other factors
       as well. For now, per Manny, just always require a server */

    return LICENSE_SERVER;
}

static
Datastruct *
LicenseToCanonicalForm(Datastruct *licenseKey)
{
    Datastruct *retval = NULL;
    int i, j, len;
    char buf[LICENSE_BUFSIZE];

    memset(buf, 0, LICENSE_BUFSIZE);

    /* first strip out any dashes */
    for(i = 0, j = 0; i < licenseKey->size; i++)
    {
        if(isalnum(licenseKey->data[i]))
        {
            buf[j++] = licenseKey->data[i];
        }
    }

    retval = calloc(1, sizeof(*retval));
    retval->size = 30;
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    len = j;
    for(i = 0, j = 0; i < len; i++)
    {
        retval->data[j++] = buf[i];
        if(((i+1)%5 == 0) && i < (len-5))
        {
            retval->data[j++] = '-';
        }
    }

    return retval;
}

static
Datastruct *
base32_encode(Datastruct *payload)
{
    Datastruct *retval, *tmp;
    u_int32_t padding, numbits, src, dest;
    u_int16_t i, j;
    u_int8_t BASE32_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    u_int8_t freeTmp = 0;

    numbits = payload->size * 8;
    padding = (numbits % 5) ? (40 - (numbits % 5)) : 0;
    if(padding)
    {
        /* zero-fill at the tail */
        padding /= 8;
        tmp = calloc(1, sizeof(*tmp));
        tmp->size = payload->size + padding;
        tmp->data = calloc(tmp->size, sizeof(*(tmp->data)));
        memcpy(tmp->data, payload->data, payload->size);
        freeTmp = 1;
    }
    else
    {
        tmp = payload;
    }
    retval = calloc(1, sizeof(*retval));
    retval->size = ((tmp->size * 8) / 5);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));
    /* fill buffer with pad characters */
    memset(retval->data, '=', retval->size);

    for(src = 0, dest = 0; src < numbits; src += 5, dest++)
    {
        j = ((u_int8_t)tmp->data[src/8]) << 8;
        if(src+5< numbits)
            j+= (u_int8_t)tmp->data[(src/8)+1];
        /* mask off anything other than the 5 bits we care about */
        i = (j >> (11 - (src % 8))) & 0x1F;
        retval->data[dest] = BASE32_ALPHABET[i];
    }
    retval->size = dest;

    if(freeTmp == 1)
    {
        free(tmp->data);
        free(tmp);
    }

    return retval;
}

static
Datastruct *
base32_decode(Datastruct *payload)
{
    Datastruct *retval, *tmp;
    u_int32_t numbits, src, dest, shift, padding;
    u_int16_t i, j, r;
    u_int8_t freeTmp = 0;
//    u_int8_t BASE32_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    dest = src = shift = 0;
    numbits = payload->size * 8;
    padding = (numbits % 5) ? (40 - (numbits % 5)) : 0;
    if(padding)
    {
        /* pad the tail with '=' */
        padding /= 8;
        tmp = calloc(1, sizeof(*tmp));
        tmp->size = payload->size + padding;
        tmp->data = calloc(tmp->size, sizeof(*(tmp->data)));
        memset(tmp->data, '=', tmp->size);
        memcpy(tmp->data, payload->data, payload->size);
        freeTmp = 1;
    }
    else
    {
        tmp = payload;
    }
    retval = calloc(1, sizeof(*retval));
    retval->size = (tmp->size * 5)/8;
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    numbits = retval->size * 8;

    for(src = 0, dest = 0; src < tmp->size; src++, dest += 5)
    {
        i = ((u_int8_t)retval->data[dest/8]) << 8;
        if(dest + 5 < numbits)
        {
            i += (u_int8_t)retval->data[(dest/8)+1];
        }
        if(isalpha(tmp->data[src]))
        {
            j = (u_int8_t)toupper(tmp->data[src]) - 'A';
        }
        else if(isdigit(tmp->data[src]))
        {
            j = (u_int8_t)tmp->data[src] - '2' + 26;
        }
        else if(tmp->data[src] == '=')
        {
            retval->size = (src * 5) / 8;
            break;
        }
        else
        {
            dest -= 5;
            continue;
        }
        shift = 11 - (dest % 8);
        j = j << shift;
        r = i | j;
        retval->data[dest/8] = r >> 8;
        retval->data[(dest/8)+1] = r & 0xFF;
    }

    if(freeTmp)
    {
        free(tmp->data);
        free(tmp);
    }

    return retval;
}

static
Datastruct *
rc2_encrypt(Datastruct *payload, Datastruct *keybuf, Datastruct *ivbuf)
{
    int status;
    u_int8_t tmp[LICENSE_BUFSIZE];
    int32_t tmplen;
    int numbytes = 0;
    EVP_CIPHER_CTX ctx;
    Datastruct *retval;
    /*unsigned char key[EVP_MAX_KEY_LENGTH];*/
    /*unsigned char iv[EVP_MAX_IV_LENGTH];*/

    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit(&ctx, EVP_rc2_cbc(), keybuf->data, ivbuf->data);
    EVP_CIPHER_CTX_set_key_length(&ctx, keybuf->size);

    memset(tmp, 0, LICENSE_BUFSIZE);

    status = EVP_EncryptUpdate(&ctx, tmp, &numbytes, payload->data, payload->size);
    if(status < 0)
    {
        GPA_LOG_ERROR("rc2_encrypt: %s\n", ERR_error_string(ERR_get_error(), 0));
        /* TODO -- we should propagate the error, not exit. OK for now per Manny */
        exit(1);
    }

#if defined(HAVE_EVP_ENCRYPTFINAL_EX)
    status = EVP_EncryptFinal_ex(&ctx, tmp + numbytes, &tmplen);
#elif defined(HAVE_EVP_ENCRYPTFINAL)
    status = EVP_EncryptFinal(&ctx, tmp + numbytes, &tmplen);
#else
#error EVP_ENCRYPTFINAL_UNAVAILABLE
#endif
    if(status < 0)
    {
        GPA_LOG_ERROR("rc2_encrypt: %s\n", ERR_error_string(ERR_get_error(), 0));
        /* TODO -- propagate the error, don't exit */
        exit(1);
    }
    numbytes += tmplen;

    EVP_CIPHER_CTX_cleanup(&ctx);

    retval = calloc(1, sizeof(*retval));
    retval->size = numbytes;
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    memcpy(retval->data, tmp, retval->size);

    return retval;
}

static
Datastruct *
makeCrcPayload(LicensePayload license, Datastruct *domain)
{
    /*int i;*/
    Datastruct *retval;
    u_int32_t numBytes = 0, netLong;
    u_int16_t netShort;

    retval = calloc(1, sizeof(*retval));
    retval->size = sizeof(license) + domain->size - sizeof(license.crc16);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    memcpy(retval->data, &(license.licenseVersion), sizeof(license.licenseVersion));
    numBytes += sizeof(license.licenseVersion);

    memcpy(retval->data + numBytes, &(license.productId), sizeof(license.productId));
    numBytes += sizeof(license.productId);

    netShort= htons(license.variable);
    memcpy(retval->data + numBytes, &(netShort), sizeof(netShort));
    numBytes += sizeof(netShort);

    netLong = htonl(license.licenseExpiration);
    memcpy(retval->data + numBytes, &(netLong), sizeof(netLong));
    numBytes += sizeof(netLong);

    netLong = htonl(license.licenseMagic);
    memcpy(retval->data + numBytes, &(netLong), sizeof(netLong));
    numBytes += sizeof(netLong);

    memcpy(retval->data + numBytes, domain->data, domain->size);
    numBytes += domain->size;

    // force the size (to avoid packing length issues)
    retval->size = numBytes;

    return retval;
}

static
Datastruct *
makeEncryptionPayload(u_int16_t crc16, LicensePayload *license)
{
    /* int i;*/
    Datastruct *retval;
    u_int16_t netShort;
    u_int32_t numBytes = 0, netLong;

    retval = calloc(1, sizeof(*retval));
    retval->size = sizeof(*license);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    netShort = htons(crc16);
    memcpy(retval->data, &netShort, sizeof(netShort));
    numBytes += sizeof(netShort);

    memcpy(retval->data + numBytes, &(license->licenseVersion), sizeof(license->licenseVersion));
    numBytes += sizeof(license->licenseVersion);

    memcpy(retval->data + numBytes, &(license->productId), sizeof(license->productId));
    numBytes += sizeof(license->productId);

    netShort= htons(license->variable);
    memcpy(retval->data + numBytes, &(netShort), sizeof(netShort));
    numBytes += sizeof(netShort);

    netLong = htonl(license->licenseExpiration);
    memcpy(retval->data + numBytes, &(netLong), sizeof(netLong));
    numBytes += sizeof(netLong);

    netLong = htonl(license->licenseMagic);
    memcpy(retval->data + numBytes, &(netLong), sizeof(netLong));
    numBytes += sizeof(netLong);

    // force the size (to avoid packing length issues)
    retval->size = numBytes;

    return retval;
}

static
Datastruct *
rc2_decrypt(Datastruct *payload, Datastruct *keybuf, Datastruct *ivbuf)
{
    int status;
    u_int8_t tmp[LICENSE_BUFSIZE];
    int32_t tmplen;
    int32_t numbytes;
    EVP_CIPHER_CTX ctx;
    Datastruct *retval;
    /*unsigned char key[EVP_MAX_KEY_LENGTH];*/
    /*unsigned char iv[EVP_MAX_IV_LENGTH];*/

    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit(&ctx, EVP_rc2_cbc(), keybuf->data, ivbuf->data);
    EVP_CIPHER_CTX_set_key_length(&ctx, keybuf->size);

    memset(tmp, 0, LICENSE_BUFSIZE);

    status = EVP_DecryptUpdate(&ctx, tmp, &numbytes, payload->data, payload->size);
    if(status < 0)
    {
        GPA_LOG_ERROR("rc2_decrypt: %s\n", ERR_error_string(ERR_get_error(), 0));
        /* propagate the error, don't exit */
        exit(1);
    }
#if defined(HAVE_EVP_DECRYPTFINAL_EX)
    status = EVP_DecryptFinal_ex(&ctx, tmp + numbytes, &tmplen);
#elif defined(HAVE_EVP_DECRYPTFINAL)
    status = EVP_DecryptFinal(&ctx, tmp + numbytes, &tmplen);
#else
#error EVP_DECRYPT_UNAVAILABLE
#endif
    if(status < 0)
    {
        GPA_LOG_ERROR("rc2_decrypt: %s\n", ERR_error_string(ERR_get_error(), 0));
        /* propagate the error, don't exit */
        exit(1);
    }
    numbytes += tmplen;

    EVP_CIPHER_CTX_cleanup(&ctx);

    retval = calloc(1, sizeof(*retval));
    retval->size = numbytes;
    retval->data = calloc(retval->size, sizeof(*(retval->data)));

    memcpy(retval->data, tmp, retval->size);

    return retval;
}

static
LicensePayload *
makeLicenseFromDecryptionPayload(Datastruct *payload)
{
    LicensePayload *license;
    u_int32_t netLong, numBytes = 0/*, ts*/;
    u_int16_t netShort;

    license = calloc(1, sizeof(*license));

    memcpy(&netShort, payload->data, sizeof(netShort));
    license->crc16 = ntohs(netShort);
    numBytes += sizeof(netShort);

    memcpy(&(license->licenseVersion), payload->data + numBytes, sizeof(license->licenseVersion));
    numBytes += sizeof(license->licenseVersion);

    memcpy(&(license->productId), payload->data + numBytes, sizeof(license->productId));
    numBytes += sizeof(license->productId);

    memcpy(&netShort, payload->data + numBytes, sizeof(netShort));
    license->variable = ntohs(netShort);
    numBytes += sizeof(netShort);

    memcpy(&netLong, payload->data + numBytes, sizeof(netLong));
    license->licenseExpiration = ntohl(netLong);
    numBytes += sizeof(netLong);

    memcpy(&netLong, payload->data + numBytes, sizeof(netLong));
    license->licenseMagic = ntohl(netLong);
    numBytes += sizeof(netLong);

    return license;
}

static
BOOLEAN
isEvalKey(LicensePayload license)
{
    if(license.licenseMagic & LICENSE_EVAL)
    {
        return TRUE;
    }

    return FALSE;
}

static
BOOLEAN
isSiteLicenseKey(LicensePayload license)
{
    if(license.licenseMagic & LICENSE_SITE)
    {
        return TRUE;
    }

    return FALSE;
}

static
CENTERROR
getLicenseKeyFromDisk(Datastruct **key)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned char buf[LICENSE_BUFSIZE];
    struct stat statstruct;
    int keyfile, status;
    Datastruct *raw = NULL, *canon = NULL;
    PSTR keyPath = NULL;

    memset(buf, 0, LICENSE_BUFSIZE);

    ceError = CTAllocateStringPrintf(&keyPath, "%s/key", CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    stat(keyPath, &statstruct);
    keyfile = open(keyPath, O_RDONLY);
    if(keyfile < 0)
    {
        ceError = CTMapSystemError(errno);
        /* no key file present -- that's OK */
        return ceError;
    }
    status = read(keyfile, buf, (statstruct.st_size < LICENSE_BUFSIZE) ? statstruct.st_size : LICENSE_BUFSIZE);
    if(status < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    close(keyfile);
    raw = calloc(1, sizeof(*raw));
    raw->size = status;
    raw->data = calloc(raw->size, sizeof(*(raw->data)));
    memcpy(raw->data, buf, raw->size);

    canon = LicenseToCanonicalForm(raw);

    *key = canon;
error:
    if(raw != NULL)
    {
        free(raw->data);
        free(raw);
    }
    CT_SAFE_FREE_STRING(keyPath);

    return ceError;
}

static
CENTERROR
putLicenseKeyToDisk(Datastruct *key)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned char buf[LICENSE_BUFSIZE];
    int status, keyfile;
    Datastruct *canon = NULL;
    PSTR keyPath = NULL;

    memset(buf, 0, LICENSE_BUFSIZE);

    ceError = CTAllocateStringPrintf(&keyPath, "%s/key", CACHEDIR);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(keyPath, "%s/key", CACHEDIR);

    keyfile = open(keyPath, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
    if(keyfile < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    canon = LicenseToCanonicalForm(key);

    status = write(keyfile, canon->data, canon->size);
    if(status != canon->size)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    close(keyfile);

error:
    if(canon != NULL)
    {
        free(canon->data);
        free(canon);
    }
    CT_SAFE_FREE_STRING(keyPath);

    return ceError;
}

static
Datastruct *
get_enc_key()
{
    Datastruct *retval;
    u_int8_t key[] = {
        0x54,
        0xD1,
        0x64,
        0x0,
        0x25,
        0xF1,
        0x64,
        0xC2,
        0x7C,
        0xB0,
        0xD3,
        0x1F,
        0xCC,
        0xE4,
        0xDF,
        0x4B
    };

    retval = calloc(1, sizeof(*retval));
    retval->size = sizeof(key);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));
    memcpy(retval->data, key, retval->size);

    return retval;
}

static
Datastruct *
get_enc_iv()
{
    Datastruct *retval;
    u_int8_t iv[] = {
        0x8C,
        0x6E,
        0x6B,
        0x3E,
        0xE8,
        0x6F,
        0xAD,
        0x25
    };

    retval = calloc(1, sizeof(*retval));
    retval->size = sizeof(iv);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));
    memcpy(retval->data, iv, retval->size);

    return retval;
}

static
LicensePayload *
crackLicense(Datastruct *licenseKey)
{
    Datastruct *b32result = NULL;
    Datastruct *decresult = NULL;
    Datastruct *enckey = NULL;
    Datastruct *iv = NULL;
    LicensePayload *retval = NULL;

    enckey = get_enc_key();
    iv = get_enc_iv();
    b32result = base32_decode(licenseKey);
    decresult = rc2_decrypt(b32result, enckey, iv);

    if(b32result)
    {
        free(b32result->data);
        free(b32result);
    }

    retval = makeLicenseFromDecryptionPayload(decresult);

    if (enckey) {
        freeDatastruct(enckey);
    }

    if (iv) {
        freeDatastruct(iv);
    }

    if (decresult) {
        freeDatastruct(decresult);
    }

    return retval;
}

static
void
PopulateEvalLicense(LicensePayload *license)
{
    license->licenseVersion = LICENSE_VERSION;
    license->productId = LICENSE_PRODUCT_IDENTITY;
    license->variable = 0;
    license->licenseExpiration = getInodeTimeStamp() + EVAL_DURATION;
    license->licenseMagic = requireServerOrWorkstationLicense() | LICENSE_EVAL;

    return;
}

static
CENTERROR
GetDomainName(Datastruct **ppDomain, DWORD licenseMagic)
{
    char *pszDomain = NULL, *tmp/*, *p1, *p2*/;
    Datastruct *retval = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    char buf[LICENSE_BUFSIZE];
    u_int32_t numToks = 0/*, pos = 0*/;

    memset(buf, 0, LICENSE_BUFSIZE);

    ceError = GPOGetADDomain(&pszDomain);
    if(ceError != CENTERROR_SUCCESS)
    {
        sprintf(buf, "%s", "UNKNOWN");
    }
    else
    {
        memcpy(buf, pszDomain, strlen(pszDomain));
    }

    tmp = buf;
    /* If a site license, get the TLD -- so for CORP.FOO.BAR.COM, just return BAR.COM */
    if(licenseMagic & LICENSE_SITE)
    {
        tmp = strtok(buf, ".");
        while(tmp != NULL)
        {
            tmp = strtok(NULL, ".");
            numToks++;
        }

        memset(buf, 0, LICENSE_BUFSIZE);
        memcpy(buf, pszDomain, strlen(pszDomain));

        tmp = buf;

        if(numToks > 2)
        {
            for(; numToks > 2; numToks--)
            {
                tmp = strchr(tmp, '.');
                if(tmp != NULL)
                    tmp++;
            }
        }
    }

    GPA_LOG_INFO("gpalicense.c:GetDomainName:domain is [%s]\n", tmp);

    retval = calloc(1, sizeof(*retval));
    retval->size = strlen(tmp);
    retval->data = calloc(retval->size, sizeof(*(retval->data)));
    memcpy(retval->data, tmp, retval->size);
    for(numToks = 0; numToks < retval->size; numToks++)
    {
        retval->data[numToks] = toupper(retval->data[numToks]);
    }

    *ppDomain = retval;
    retval = NULL;

    if(pszDomain != NULL)
        CTFreeString(pszDomain);

    if(retval != NULL)
    {
        free(retval->data);
        free(retval);
    }

    return ceError;
}

static
Datastruct *
GenerateEvalLicense(
    PGPALICENSEINFO pLicenseInfo
    )
{
    Datastruct *retval = NULL;
    Datastruct *crcPayload = NULL;
    Datastruct *enckey = NULL;
    Datastruct *iv = NULL;
    Datastruct *encpayload = NULL;
    Datastruct *encresult = NULL;
    Datastruct *domain = NULL;
    /*DWORD licenseMagic = 0;*/
    u_int16_t crc16;
    LicensePayload license;

    PopulateEvalLicense(&license);
    GetDomainName(&domain, requireServerOrWorkstationLicense());
    crcPayload = makeCrcPayload(license, domain);
    free(domain->data);
    free(domain);

    pLicenseInfo->dwLicenseExpiration = license.licenseExpiration;
    pLicenseInfo->dwLicenseMagic = license.licenseMagic;

    crc16 = crc(crcPayload);
    free(crcPayload->data);
    free(crcPayload);

    license.crc16 = crc16;
    encpayload = makeEncryptionPayload(crc16, &license);

    enckey = get_enc_key();
    iv = get_enc_iv();
    encresult = rc2_encrypt(encpayload, enckey, iv);
    free(encpayload->data);
    free(encpayload);
    free(enckey->data);
    free(enckey);
    free(iv->data);
    free(iv);

    retval = base32_encode(encresult);
    free(encresult->data);
    free(encresult);

    return retval;
}

/* checks for a valid CRC, timestamp, version, product Id, and license type */
static
CENTERROR
ValidateLicense(
    LicensePayload *crackedLicense,
    PGPALICENSEINFO pLicenseInfo
    )
{
    u_int32_t ts;
    u_int16_t crc16, genCrc;
    CENTERROR ceError = CENTERROR_SUCCESS;
    LicensePayload generatedLicense;
    Datastruct *crcPayload = NULL;
    Datastruct *domain = NULL;

    /* verify whatever key we have now */
    /* First, verify that the license is not expired */
    ts = getTimeStamp();

    pLicenseInfo->dwLicenseExpiration = crackedLicense->licenseExpiration;
    if(crackedLicense->licenseExpiration != 0 && ts > crackedLicense->licenseExpiration)
    {
        GPA_LOG_ERROR("Likewise Identity license is expired.");
        ceError = CENTERROR_LICENSE_EXPIRED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* check if the product id matches that of likewise identity */
    if(crackedLicense->productId != LICENSE_PRODUCT_IDENTITY)
    {
        GPA_LOG_ERROR("Likewise Identity license from other product.");
        ceError = CENTERROR_LICENSE_INCORRECT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* license version check */
    if(crackedLicense->licenseVersion != LICENSE_VERSION)
    {
        GPA_LOG_ERROR("Likewise Identity license version mismatch.");
        ceError = CENTERROR_LICENSE_INCORRECT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* verify that license type matches */
    if(!(pLicenseInfo->dwLicenseMagic & requireServerOrWorkstationLicense()))
    {
        GPA_LOG_ERROR("Likewise Identity license does not match machine role. Got [%s], expected [%s].",
                      (pLicenseInfo->dwLicenseMagic & LICENSE_SERVER) ? "LICENSE_SERVER" : "LICENSE_WORKSTATION",
                      (pLicenseInfo->dwLicenseMagic & LICENSE_SERVER) ? "LICENSE_WORKSTATION" : "LICENSE_SERVER");
        ceError = CENTERROR_LICENSE_INCORRECT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* verify the CRC */
    if(isEvalKey(*crackedLicense) == TRUE)
    {
        PopulateEvalLicense(&generatedLicense);
    }
    else
    {
        /* Assemble our notion of a valid license */
        generatedLicense.licenseVersion = LICENSE_VERSION;
        generatedLicense.productId = LICENSE_PRODUCT_IDENTITY;
        generatedLicense.variable = crackedLicense->variable;
        generatedLicense.licenseExpiration = crackedLicense->licenseExpiration;
        generatedLicense.licenseMagic = crackedLicense->licenseMagic;
    }

    GetDomainName(&domain, generatedLicense.licenseMagic);
    crcPayload = makeCrcPayload(generatedLicense, domain);
    crc16 = crackedLicense->crc16;
    genCrc = crc(crcPayload);
    free(crcPayload->data);
    free(crcPayload);
    crcPayload = NULL;
    free(domain->data);
    free(domain);
    domain = NULL;
    if(crc16 != genCrc)
    {
        /* Most likely cause is domain mismatch */
        ceError = CENTERROR_LICENSE_INCORRECT;
        GPA_LOG_ERROR("Likewise Identity license checksum does not match.");
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* validate is good -- update the global license settings in serverinfo */
    pLicenseInfo->dwLicenseMagic = crackedLicense->licenseMagic;
    pLicenseInfo->bLicenseValid = TRUE;
    pLicenseInfo->dwLicenseExpiration = crackedLicense->licenseExpiration;

error:
    if(domain != NULL)
    {
        free(domain->data);
        free(domain);
    }

    if(crcPayload != NULL)
    {
        free(crcPayload->data);
        free(crcPayload);
    }

    return ceError;
}

GroupPolicyMessageType
handle_license_request(
    PDWORD licenseMagic,
    u_int32_t *licenseExpiration
    )
{
  GPALICENSEINFO licenseInfo;

  memset(&licenseInfo, 0, sizeof(GPALICENSEINFO));

  GetLicenseInfo(&licenseInfo);

  *licenseMagic = licenseInfo.dwLicenseMagic;
  *licenseExpiration = licenseInfo.dwLicenseExpiration;

  return (licenseInfo.bLicenseValid ? LICENSE_VALID : LICENSE_ERROR);
}

CENTERROR
CheckProvisioningState(
    HANDLE hDirectory,
    PSTR pszMachineName,
    PSTR pszADDomain
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDirectoryRoot = NULL;
    PSTR pszLicenseKeyDN = NULL;
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PGPO_DIRECTORY_CONTEXT pDirectory = NULL;

    ceError = GPAGetLicenseDN(pszADDomain, &pszLicenseKeyDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPODirectorySearch(hDirectory,
                                 pszLicenseKeyDN,
                                 LDAP_SCOPE_BASE,
                                 "(objectClass=*)",
                                 NULL,
                                 &pMessage);
    if (ceError != CENTERROR_SUCCESS) {
        ceError = CENTERROR_LICENSE_AD_NOT_PROVISIONED;
        /* bail instead of processing the error normally -- we don't
           want to log this */
        goto error;
    }

    pDirectory = (PGPO_DIRECTORY_CONTEXT)hDirectory;
    dwCount = ldap_count_entries(pDirectory->ld, pMessage);
    if(dwCount != 1)
    {
        ceError = CENTERROR_LICENSE_AD_NOT_PROVISIONED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if(pszDirectoryRoot)
        CTFreeString(pszDirectoryRoot);
    if(pszLicenseKeyDN)
        CTFreeMemory(pszLicenseKeyDN);
    if(pMessage)
        ldap_msgfree(pMessage);

    return ceError;
}

static
CENTERROR
VerifyKeyAssignmentInAD(
    Datastruct *licenseKey
    )
{
    int32_t i;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszLicenseRoot = NULL;
    PSTR pszLicenseKeyDN = NULL;
    PSTR pszMachineName = NULL;
    PSTR pszADDomain = NULL;
    PSTR pszGUID = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    DWORD dwCount = 0;
    LicensePayload *crackedLicense = NULL;
    PSTR *ppLicenses = NULL;
    char buf[LICENSE_BUFSIZE];

    memset(buf, 0, LICENSE_BUFSIZE);
    memcpy(buf, licenseKey->data, licenseKey->size);
    /* strip off any trailing spaces or newlines */
    for(i = licenseKey->size-1; isspace((int)buf[i]); i--)
    {
        buf[i] = '\0';
    }

    ceError = GPOOpenDirectory2(&hDirectory, &pszMachineName, &pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetHostGUID(hDirectory, pszMachineName, pszADDomain, &pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLicenseDN(pszADDomain, &pszLicenseRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateMemory((strlen(pszLicenseRoot) + strlen("CN=") + licenseKey->size + 2) * sizeof(char),
                               (PVOID *)&pszLicenseKeyDN);
    BAIL_ON_CENTERIS_ERROR(ceError);
    sprintf(pszLicenseKeyDN, "CN=%s,%s", buf, pszLicenseRoot);

    crackedLicense = crackLicense(licenseKey);

    if(isSiteLicenseKey(*crackedLicense) == FALSE)
    {
        /* check assignment for single node licenses */
        ceError = GPAGetAssignedLicense(hDirectory, pszLicenseKeyDN, &ppLicenses, &dwCount, FALSE, pszGUID);
    }
    else
    {
        /* check if the site license has changed */
        ceError = GPAGetAssignedLicense(hDirectory, pszLicenseKeyDN, &ppLicenses, &dwCount, TRUE, NULL);
    }
    BAIL_ON_CENTERIS_ERROR(ceError);

    if(dwCount < 1)
    {
        ceError = CENTERROR_LICENSE_NOT_AVAILABLE_FOR_USE;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else if(dwCount > 1)
    {
        ceError = CENTERROR_LICENSE_DUPLICATE_PROVISIONED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if(ppLicenses)
    {
        for(i = 0; i < dwCount; i++)
        {
            CTFreeString(ppLicenses[i]);
        }
        CTFreeMemory(ppLicenses);
    }

    if(pszLicenseKeyDN)
        CTFreeString(pszLicenseKeyDN);
    if(crackedLicense)
        free(crackedLicense);
    if(hDirectory)
        GPOCloseDirectory(hDirectory);
    if(pszMachineName)
        CTFreeString(pszMachineName);
    if(pszADDomain)
        CTFreeString(pszADDomain);
    if(pszLicenseRoot)
        CTFreeString(pszLicenseRoot);
    if(pszGUID)
        CTFreeString(pszGUID);

    return ceError;
}

static
BOOLEAN
HaveValidPermanentLicense(
    PGPALICENSEINFO pLicenseInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    Datastruct *licenseKey = NULL;
    LicensePayload *crackedLicense = NULL;
    BOOLEAN bLicenseValid = FALSE;

    /* check for existing license on disk */
    ceError = getLicenseKeyFromDisk(&licenseKey);
    if(ceError != CENTERROR_SUCCESS)
    {
        /* no license key on disk, just bail */
        pLicenseInfo->bLicenseValid = bLicenseValid;
        return bLicenseValid;
    }

    /* see that it decrypts correctly */
    crackedLicense = crackLicense(licenseKey);
    if(crackedLicense == NULL)
    {
        ceError = CENTERROR_LICENSE_INCORRECT;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    pLicenseInfo->dwLicenseMagic = crackedLicense->licenseMagic;

    /* validate the key contents */
    ceError = ValidateLicense(crackedLicense, pLicenseInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* check that it is not an eval key */
    if(isEvalKey(*crackedLicense))
    {
        ceError = CENTERROR_LICENSE_INCORRECT;
        /* not really an error. Don't want to log this, so bail
           using straight goto, instead of BAIL_ON_CENTERIS_ERROR */
        goto error;
    }

    /* check that the key belongs to us in AD */
    if(pLicenseInfo->bLicensesProvisioned)
    {
        ceError = VerifyKeyAssignmentInAD(licenseKey);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    free(licenseKey->data);
    free(licenseKey);
    licenseKey = NULL;
    free(crackedLicense);
    crackedLicense = NULL;

    bLicenseValid = TRUE;

error:
    if(licenseKey != NULL)
    {
        free(licenseKey->data);
        free(licenseKey);
    }

    if(crackedLicense != NULL)
    {
        free(crackedLicense);
    }

    pLicenseInfo->bLicenseValid = bLicenseValid;

    return bLicenseValid;
}

static
CENTERROR
useEvalLicense(
    PGPALICENSEINFO pLicenseInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    Datastruct *licenseKey = NULL;
    /*DWORD licenseMagic = 0;*/
    LicensePayload *crackedLicense = NULL;
    BOOLEAN bGenerated = FALSE;

    /* check for generated eval license first */
    ceError = getLicenseKeyFromDisk(&licenseKey);
    if(ceError == CENTERROR_SUCCESS)
    {
        /* Check that someone hasn't placed a stolen permanent key on the machine */
        crackedLicense = crackLicense(licenseKey);
        if(!(crackedLicense->licenseMagic & LICENSE_EVAL))
        {
            free(licenseKey->data);
            free(licenseKey);
            licenseKey = NULL;
        }
        ceError = ValidateLicense(crackedLicense, pLicenseInfo);
        if(ceError != CENTERROR_SUCCESS && ceError != CENTERROR_LICENSE_EXPIRED)
        {
            /* some other error failed -- we want to trigger
               generation of a new license anyway */
            if(licenseKey != NULL)
            {
                free(licenseKey->data);
                free(licenseKey);
                licenseKey = NULL;
            }
        }
        free(crackedLicense);
        crackedLicense = NULL;
    }
    if(licenseKey == NULL)
    {
        /* No key found on disk -- generate an eval */
        licenseKey = GenerateEvalLicense(pLicenseInfo);
        bGenerated = TRUE;
    }

    crackedLicense = crackLicense(licenseKey);
    ceError = ValidateLicense(crackedLicense, pLicenseInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(bGenerated == TRUE)
    {
        ceError = putLicenseKeyToDisk(licenseKey);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
error:
    if(licenseKey)
    {
        free(licenseKey->data);
        free(licenseKey);
    }
    if(crackedLicense)
        free(crackedLicense);
    return ceError;
}

PSTR ExtractLicenseKeyText(PSTR pszLicenseKeyDN)
{
    PSTR start = NULL;
    PSTR stop = NULL;
    PSTR pszLicenseKeyText = NULL;

    start = strchr(pszLicenseKeyDN, '=');
    if(start != NULL)
    {
        start++;
        stop = strchr(pszLicenseKeyDN, ',');
        if(stop != NULL)
        {
            CTAllocateMemory((stop - start + 1) * sizeof(char),
                             (PVOID *)&pszLicenseKeyText);
            memcpy(pszLicenseKeyText, start, stop - start);
        }
    }

    return pszLicenseKeyText;
}

CENTERROR
ReleaseLicense()
{
    int32_t i = 0;
    /*BOOLEAN bLicensesProvisioned = FALSE;*/
    CENTERROR ceError = CENTERROR_SUCCESS;
    HANDLE hDirectory= (HANDLE)NULL;
    PSTR pszMachineName = NULL;
    PSTR pszADDomain = NULL;
    PSTR pszLicenseRoot = NULL;
    PSTR pszLicenseKeyDN = NULL;
    PSTR pszGUID;
    /*DWORD dwCount = 0;*/
    Datastruct *licenseKey = NULL;
    LicensePayload *crackedLicense = NULL;
    char buf[LICENSE_BUFSIZE];
    GPALICENSEINFO licenseInfo;

    memset(buf, 0, LICENSE_BUFSIZE);

    memset(&licenseInfo, 0, sizeof(GPALICENSEINFO));

    GetLicenseInfo(&licenseInfo);

    if(!licenseInfo.bLicensesProvisioned)
    {
        /* nothing to see here, move along */
        return ceError;
    }

    ceError = getLicenseKeyFromDisk(&licenseKey);
    if(ceError != CENTERROR_SUCCESS)
    {
        /* We have no license file, so there's nothing to release */
        return CENTERROR_SUCCESS;
    }

    crackedLicense = crackLicense(licenseKey);
    {
        if(isSiteLicenseKey(*crackedLicense) == TRUE)
        {
            /* this is a site license, so there's nothing to release */
            ceError = CENTERROR_SUCCESS;
            goto error;
        }
    }

    ceError = GPOOpenDirectory2(&hDirectory, &pszMachineName, &pszADDomain);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetHostGUID(hDirectory, pszMachineName, pszADDomain, &pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetLicenseDN(pszADDomain, &pszLicenseRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);
    memcpy(buf, licenseKey->data, licenseKey->size);
    for(i = licenseKey->size -1; isspace((int)buf[i]); i--)
    {
        buf[i] = '\0';
    }

    ceError = CTAllocateMemory((strlen(pszLicenseRoot) + strlen("CN=") + licenseKey->size + 2) * sizeof(char),
                               (PVOID *)&pszLicenseKeyDN);
    BAIL_ON_CENTERIS_ERROR(ceError);
    sprintf(pszLicenseKeyDN, "CN=%s,%s", buf, pszLicenseRoot);

    /* just make sure we're freeing our own key */
    ceError = VerifyKeyAssignmentInAD(licenseKey);
    if(ceError != CENTERROR_SUCCESS)
    {
        /* not our key, so just bail out */
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    /* pass in an empty GUID to clear the license assignment */
    ceError = GPAAssignLicense(hDirectory, pszLicenseKeyDN, "");
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    if(hDirectory)
        GPOCloseDirectory(hDirectory);
    if(pszMachineName)
        CTFreeString(pszMachineName);
    if(pszADDomain)
        CTFreeString(pszADDomain);
    if(pszLicenseRoot)
        CTFreeString(pszLicenseRoot);
    if(pszLicenseKeyDN)
        CTFreeString(pszLicenseKeyDN);
    if(pszGUID)
        CTFreeString(pszGUID);
    if(licenseKey)
    {
        free(licenseKey->data);
        free(licenseKey);
    }
    if(crackedLicense)
        free(crackedLicense);

    return ceError;
}

static
CENTERROR
ProvisionLicense(
    HANDLE hDirectory,
    PSTR pszMachineName,
    PSTR pszADDomain,
    PGPALICENSEINFO pLicenseInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszGUID = NULL;
    PSTR pszLicenseRoot = NULL;
    PSTR pszLicenseKeyDN = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR pszLicenseKeyText =NULL;
    DWORD dwCount = 0;
    PSTR *ppLicenses = NULL;
    BOOLEAN bUseSiteLicense = TRUE;
    Datastruct *licenseKey = NULL;
    LicensePayload *crackedLicense = NULL;
    u_int32_t i;

    GPA_LOG_VERBOSE("Entering ProvisionLicense...");

    if(!pLicenseInfo->bLicensesProvisioned)
    {
        ceError = useEvalLicense(pLicenseInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);
        return ceError;
    }

    ceError = GPAGetLicenseDN(pszADDomain, &pszLicenseRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPAGetHostGUID(hDirectory, pszMachineName, pszADDomain, &pszGUID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /* check for site licenses */
    ceError = GPAGetAvailableLicenses(hDirectory, pszLicenseRoot, &ppLicenses, &dwCount, bUseSiteLicense, NULL);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(dwCount == 0)
    {
        /* no site licenses -- check for single node licneses assigned to my GUID */
        bUseSiteLicense = FALSE;
        ceError = GPAGetAvailableLicenses(hDirectory, pszLicenseRoot, &ppLicenses, &dwCount, bUseSiteLicense, pszGUID);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(dwCount == 0)
    {
        /* no site license or assigned license -- chec for unassigned licenses */
        ceError = GPAGetAvailableLicenses(hDirectory, pszLicenseRoot, &ppLicenses, &dwCount, bUseSiteLicense, NULL);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if(dwCount == 0)
    {
        /* no single node licenses either -- use an eval license */
        ceError = useEvalLicense(pLicenseInfo);
        goto error;
    }

    /* Grab the first licenseKeyDN from the list returned */
    pszLicenseKeyDN = ppLicenses[0];

    pszLicenseKeyText = ExtractLicenseKeyText(pszLicenseKeyDN);
    if(pszLicenseKeyText == NULL)
    {
        ceError = CENTERROR_LICENSE_NO_KEY_IN_DN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /* Mark the key as mine */
    if(bUseSiteLicense == FALSE)
    {
        ceError = GPAAssignLicense(hDirectory, pszLicenseKeyDN, pszGUID);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    licenseKey = calloc(1, sizeof(*licenseKey));
    licenseKey->size = strlen(pszLicenseKeyText);
    licenseKey->data = calloc(licenseKey->size, sizeof(*(licenseKey->data)));
    memcpy(licenseKey->data, pszLicenseKeyText, licenseKey->size);

    ceError = putLicenseKeyToDisk(licenseKey);
    BAIL_ON_CENTERIS_ERROR(ceError);

    crackedLicense = crackLicense(licenseKey);
    ceError = ValidateLicense(crackedLicense, pLicenseInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    /*
     * At this point, we have provisioned a license that we know to be
     * valid. Mark this happy occasion
     */
    pLicenseInfo->bLicenseValid = TRUE;


error:
    if(crackedLicense)
        free(crackedLicense);

    if(ppLicenses)
    {
        for(i = 0; i < dwCount; i++)
        {
            CTFreeString(ppLicenses[i]);
        }
        CTFreeMemory(ppLicenses);
    }

    if(licenseKey)
    {
        free(licenseKey->data);
        free(licenseKey);
        licenseKey = NULL;
    }
    if(pszLicenseKeyText)
        CTFreeString(pszLicenseKeyText);

    if (pszLicenseRoot)
       CTFreeString(pszLicenseRoot);

    if (pszGUID)
       CTFreeString(pszGUID);

    if (pszDirectoryRoot)
       CTFreeString(pszDirectoryRoot);

    GPA_LOG_VERBOSE("Leaving ProvisionLicense [return code:0x%08X]", ceError);

    return ceError;
}

CENTERROR
CheckLicense(
    HANDLE hDirectory,
    PSTR pszMachineName,
    PSTR pszADDomain,
    PSTR pszDomainControllerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*DWORD retval = 0; */
    HANDLE hDirectoryLocal = hDirectory;
    BOOLEAN bFreeDirectoryHandle = FALSE;
    GPALICENSEINFO licenseInfo;
    /*DWORD licenseMagic = 0;*/
    PSTR pszMachineNameLocal = pszMachineName;
    BOOLEAN bFreeMachineName = FALSE;
    PSTR pszADDomainLocal = pszADDomain;
    BOOLEAN bFreeADDomain = FALSE;
    /*u_int32_t licenseExpiration = 0;*/

    GPA_LOG_INFO("Checking License");

    memset(&licenseInfo, 0, sizeof(GPALICENSEINFO));

    ceError = GPOisJoinedToAD(&licenseInfo.bJoinedToAD);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (licenseInfo.bJoinedToAD)
    {
      if (hDirectoryLocal == (HANDLE)NULL)
      {
        if (pszADDomainLocal && pszDomainControllerName)
        {
           ceError = GPOOpenDirectory(pszADDomainLocal,
                                      pszDomainControllerName,
                                      &hDirectoryLocal);

           if (!CENTERROR_IS_OK(ceError))
           {
	      licenseInfo.bCanTalkToAD = FALSE;
	      BAIL_ON_CENTERIS_ERROR(ceError);
           }

        } else {   

           ceError = GPOOpenDirectory2(&hDirectoryLocal,
                                       &pszMachineNameLocal,
                                       &pszADDomainLocal);

           if (!CENTERROR_IS_OK(ceError))
           {
	      licenseInfo.bCanTalkToAD = FALSE;
	      BAIL_ON_CENTERIS_ERROR(ceError);
           }

           bFreeADDomain = TRUE;
           bFreeMachineName = TRUE;
        }

	bFreeDirectoryHandle = TRUE;
      }

      licenseInfo.bCanTalkToAD = TRUE;

      if (!pszMachineNameLocal)
      {
         ceError = GetDnsSystemNames(NULL, &pszMachineNameLocal, NULL);
         BAIL_ON_CENTERIS_ERROR(ceError);
         bFreeMachineName = TRUE;
      }

      if (!pszADDomainLocal)
      {
         ceError = GPOGetADDomain(&pszADDomainLocal);
         BAIL_ON_CENTERIS_ERROR(ceError);
         bFreeADDomain = TRUE;
      }

      ceError = CheckProvisioningState(hDirectoryLocal,
                                       pszMachineNameLocal,
                                       pszADDomainLocal);
      licenseInfo.bLicensesProvisioned = CENTERROR_IS_OK(ceError);
    }

    licenseInfo.dwLicenseMagic |= requireServerOrWorkstationLicense();

    if (!HaveValidPermanentLicense(&licenseInfo))
    {
        ceError = ProvisionLicense(hDirectoryLocal,
                                   pszMachineNameLocal,
                                   pszADDomainLocal,
                                   &licenseInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);

	licenseInfo.bLicenseValid = TRUE;
    }

 error:

    /* If we couldn't talk to AD (for instance, network connection is down),
       let licensing requirement slide so that cached credentials will
       work.  If the license is invalid, it will be caught next time the
       network is available
    */
    if (licenseInfo.bJoinedToAD && !licenseInfo.bCanTalkToAD)
        licenseInfo.bLicenseValid = TRUE;

    SetLicenseInfo(&licenseInfo);

    if ( bFreeDirectoryHandle )
    {
       GPOCloseDirectory(hDirectoryLocal);
    }

    if (bFreeMachineName && pszMachineNameLocal)
    {
       CTFreeString(pszMachineNameLocal);
    }

    if (bFreeADDomain && pszADDomainLocal)
    {
       CTFreeString(pszADDomainLocal);
    }

    return (licenseInfo.bLicenseValid ? CENTERROR_SUCCESS : CENTERROR_LICENSE_INCORRECT);
}

CENTERROR
GPAGetLicenseDN(
    PSTR  pszADDomain,
    PSTR* ppszLicenseDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszDirectoryRoot = NULL;
    PSTR pszLicenseKeyDN = NULL;

    ceError = ConvertDomainToDN(pszADDomain, &pszDirectoryRoot);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateMemory((sizeof(AD_LICENSE_KEY_PREFIX) + strlen(pszDirectoryRoot)) * sizeof(char),
                               (PVOID *)&pszLicenseKeyDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(pszLicenseKeyDN, "%s%s", AD_LICENSE_KEY_PREFIX, pszDirectoryRoot);

    *ppszLicenseDN = pszLicenseKeyDN;
    pszLicenseKeyDN = NULL;

error:

    if (pszDirectoryRoot) {
        CTFreeString(pszDirectoryRoot);
    }

    if (pszLicenseKeyDN) {
        CTFreeString(pszLicenseKeyDN);
    }

    return ceError;
}

u_int32_t
getTimeStamp()
{
    struct timeval tv;
    int status;

    status = gettimeofday(&tv, NULL);

    /* returns minutes since the start of the epoch (Jan 1, 1970) */
    return (u_int32_t)(tv.tv_sec / (60*60*24));
}
