#ifndef GPOLICENSE_H_
#define GPOLICENSE_H_

#include "gpacommon.h"

#define LICENSE_BUFSIZE 1024

#define EVAL_DURATION 31 /* days */

#define AD_LICENSE_KEY_PREFIX "CN=Licenses,CN=Likewise,CN=Centeris,CN=Program Data,"

typedef struct data_struct
{
    unsigned char *data;
    size_t size;
} Datastruct;

#define LICENSE_SERVER 0x00000001
#define LICENSE_WORKSTATION 0x00000002
#define LICENSE_EVAL 0x00000004
#define LICENSE_SITE 0x00000008

#define LICENSE_PRODUCT_IDENTITY 1
#define LICENSE_VERSION 0

typedef struct license_payload_struct
{
    /* all data is stored on disk in network byte order */
    u_int16_t crc16;             /* the result of the CRC-16(licenseeData, expirationDate, licenseVersion, productId) function */
    u_int8_t licenseVersion;     /* numerical value of the license version */
    u_int8_t productId;          /* numerical value of the product ID */
    u_int16_t variable;          /* variable portion of the license (random or not) */
    u_int32_t licenseExpiration; /* 32-bit timestamp of day the license expires, expressed as days since the start of the UNIX epoch */
    u_int32_t licenseMagic;      /* reserved for license-internal use. Currently used to specify server, workstation, and eval licenses */
} LicensePayload;

u_int32_t
getTimeStamp();

GroupPolicyMessageType
handle_license_request(
    PDWORD licenseMagic,
    u_int32_t *licenseExpiration
    );

DWORD
requireServerOrWorkstationLicense();

CENTERROR
GPAGetLicenseDN(
    PSTR  pszADDomain,
    PSTR* ppszLicenseDN
    );

CENTERROR
ReleaseLicense();

#endif /* GPOLICENSE_H_ */
