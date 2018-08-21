/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Beyondtrust Software    2018
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */
#ifndef _PBPS_INT_H_
#define _PBPS_INT_H_

#include "DomainJoinConfig.h"
#ifdef __LWI_DARWIN__
#ifdef _MK_HOST_X86_32
#include PBIS_CURL_INCLUDE_32
#endif
#ifdef _MK_HOST_X86_64
#include PBIS_CURL_INCLUDE_64
#endif
#endif
#include <bail.h>
#include <lwdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lwerror.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lwfile.h>
#include <lw/rtlstring.h>
#include <lw/rtllog.h>
#include <curl/curl.h>
#include <lwdlinked-list.h>
#include <djdefines.h>
#include <ctbase.h>
#include <ctfileutils.h>
#include <djlogger.h>
#include <djhostinfo.h>


// Certificates in the domain join configuration file needs to be 
// in PEM format.
#define PBPS_CURLOPT_SSLCERTTYPE  "PEM"
 
#define BAIL_WITH_CURL_ERROR(_curlError, ...) \
    do { \
        DJ_LOG_ERROR("CURL error %d (%s)\n", \
            _curlError, \
            curl_easy_strerror(_curlError)); \
        dwError = PbpsCurlErrorToLwError(_curlError); \
        goto error; \
    } while(0);

#define BAIL_ON_CURL_ERROR(_curlError, ...) \
    do { \
        if (_curlError != CURLE_OK) { \
            BAIL_WITH_CURL_ERROR(_curlError , ## __VA_ARGS__); \
        } \
    } while(0);


/* 
 * Credential checkout duration time as defined in Password Safe API.
 */
#define PBPSAPI_MINIMUM_DURATION_MINUTES 1
#define PBPSAPI_MAXIMUM_DURATION_MINUTES 10079
#define PBPSAPI_DEFAULT_DURATION_MINUTES PBPSAPI_MINIMUM_DURATION_MINUTES

#define CURL_HEADER_AUTH_FORMAT  "Authorization: PS-Auth key=%s;runas=%s;"
#define CURL_HEADER_AUTH_PWD_NEEDLE  "pwd=["
#define CURL_HEADER_AUTH_FORMAT_PWD  " " CURL_HEADER_AUTH_PWD_NEEDLE "%s];"
#define CURL_HEADER_AUTH_FORMAT_WITH_PWD CURL_HEADER_AUTH_FORMAT CURL_HEADER_AUTH_FORMAT_PWD
#define CURL_HEADER_CONTENT_FORMAT  "Content-Type: application/json; charset=utf-8"


typedef struct PbpsApiManagedAccount_s
{
   DWORD dwPlatformId;
   DWORD dwSystemId;
   DWORD dwAccountId;
   PSTR  pszSystemName;
   PSTR  pszDomainName;
   PSTR  pszAccountName;
   PSTR  pszAccountNameFull;
} PbpsApiManagedAccount_t;

typedef struct PbpsApiConfig_s
{      
   DWORD dwVersionTemplate;
   PSTR pszUrlBase;         // PasswordSafe server
   PSTR pszRunAsUser;       // run as user in http header 
   PSTR pszRunAsUserPwd;
   PSTR pszApiKey;
   PSTR pszHeaderAuth;
   PSTR pszJoinAccount;     // Needs to be a PasswordSafe managed account.
                            // Account to request credentials.
   DWORD dwDurationMinutes;
   PSTR pszCertFileClient;  // Certificate PBIS provides to Password Safe for validation
   PSTR pszCertFileCA;      // CA for PBIS to use to verify certificate from Password Safe
}  PbpsApiConfig_t;

typedef struct PbpsApiSession_s
{
   CURL              *pCurlHandle;
   struct curl_slist *pHeaderList;
   PLW_DLINKED_LIST  pManagedAccountList;  // Fill-in by GET ManagedAccounts
}  PbpsApiSession_t;

typedef struct PbpsApi_s
{
   PbpsApiConfig_t config;
   PbpsApiSession_t session;
}  PbpsApi_t;

typedef struct responseBuffer_s
{
  PSTR buffer;
  DWORD size;
} responseBuffer_t;

/* 
 * Possible http errors codes from Password Safe.
 */
typedef enum PbpsErrorCode_e {
   PBPS_HTTP_SUCCESS                                  = 200,
   PBPS_HTTP_SUCCESS_WITH_CONTENT                     = 201,
   PBPS_HTTP_SUCCESS_WITHOUT_CONTENT                  = 204,
   PBPS_HTTP_BAD_REQUEST                              = 400,
   PBPS_HTTP_UNAUTHORIZED                             = 401,
   PBPS_HTTP_ACCESS_FORBIDDEN                         = 403,
   PBPS_HTTP_ACCESS_FORBIDDEN_PERMISSION              = 4031,
   PBPS_HTTP_ACCESS_FORBIDDEN_REQUESTOR_ONLY          = 4032,
   PBPS_HTTP_ACCESS_FORBIDDEN_APPROVER_ONLY           = 4033,
   PBPS_HTTP_ACCESS_FORBIDDEN_REQUEST_NOT_YET_APPROVED = 4034,
   PBPS_HTTP_ACCESS_FORBIDDEN_NOT_ENOUGH_APPROVER     = 4035,
   PBPS_HTTP_OBJECT_NOT_FOUND                         = 404,
   PBPS_HTTP_CONFLICTING_REQUESTS                     = 409,
   PBPS_HTTP_API_VERSION_DISABLED                     = 410,
   PBPS_HTTP_UNEXPECTED_SERVER_ERROR                  = 500
} PbpsErrorCode_t;


extern DWORD PbpsCurlErrorToLwError(CURLcode curlError);
extern DWORD PbpsHttpErrorToLwError(PbpsErrorCode_t pbpsErrorCode);

extern DWORD PbpsApiCurlDebugStart(PbpsApi_t *pApi);
extern DWORD PbpsApiCurlDebugStop(PbpsApi_t *pApi);

extern DWORD PbpsApiInitialize(PbpsApi_t **ppApi);
extern VOID PbpsApiRelease(PbpsApi_t *pApi);

extern DWORD PbpsApiCurlHeaderSet(PbpsApi_t *pApi);

extern size_t PbpsApiWriteCallBackFunction(
           char *pNewData, size_t newSize,
           size_t nmemb, void *userData);

extern size_t PbpsApiReadCallBackFunction(
           void *ptr, size_t size, size_t nmemb, void *stream);

extern DWORD PbpsApiGetConfig(PbpsApi_t *pApi, PSTR pszConfigFile);
extern DWORD PbpsApiSignIn(PbpsApi_t *pApi);
extern DWORD PbpsApiRequestId(PbpsApi_t *pApi, DWORD *pdwRequestId);
extern DWORD PbpsApiCredentialsGet(PbpsApi_t *pApi, DWORD dwRequestId, 
                                   PSTR *ppszCredentials);
extern DWORD PbpsApiRequestIdCheckin(PbpsApi_t *pApi, DWORD dwRequestId);
extern DWORD PbpsApiSignOut(PbpsApi_t *pApi);


extern DWORD PbpsApiManagedAccountsGet(PbpsApi_t *pApi);
extern VOID  PbpsApiManagedAccountFree(PbpsApiManagedAccount_t *pAccount);
extern VOID  PbpsManagedAccountListFree(PLW_DLINKED_LIST pManagedAccountList);
extern DWORD PbpsApiGetJoinAccount(
           PbpsApi_t *pApi,
           PSTR pszJoinAccount,
           PbpsApiManagedAccount_t **ppAccount);

#endif  // _PBPS_INT_H_





