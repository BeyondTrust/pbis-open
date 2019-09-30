/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "pbps-int.h"

/*
 * The module contains functionals for translating curl and http errors to
 * LW  errors. Also curl debug functions:
 *   - PbpsApiCurlDebugStart()/PbpsApiCurlDebugStop()
 *      - emit curl requests and responses to a debug file 
 *        /var/log/pbps-curl.log
 *
 */

DWORD
PbpsCurlErrorToLwError(CURLcode curlError)
{
    switch (curlError)
    {
        case CURLE_OK:
            return LW_ERROR_SUCCESS;

        case CURLE_OUT_OF_MEMORY:
            return LW_ERROR_OUT_OF_MEMORY;

        case CURLE_BAD_FUNCTION_ARGUMENT:
        case CURLE_URL_MALFORMAT:
            return LW_ERROR_INVALID_PARAMETER;

        case CURLE_COULDNT_RESOLVE_HOST:
            return LW_ERROR_DOMAINJOIN_CANNOT_RESOLVE_HOST;

        case CURLE_COULDNT_CONNECT:
        case CURLE_SSL_CONNECT_ERROR:
            return ERROR_CONNECTION_REFUSED;

        case CURLE_WRITE_ERROR:
        case CURLE_READ_ERROR:
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
            return LW_ERROR_GENERIC_IO;

        case CURLE_CONV_FAILED:
            return LW_ERROR_STRING_CONV_FAILED;

        case CURLE_SSL_CACERT:
            return LW_ERROR_DOMAINJOIN_CONFIG_CANT_VERIFY_CERTIFICATE;

        case CURLE_SSL_CERTPROBLEM:
            return LW_ERROR_DOMAINJOIN_CONFIG_INVALID_CERTIFICATE;

        default:
            return LW_ERROR_UNKNOWN;
    }

    return LW_ERROR_UNKNOWN;
}

DWORD
PbpsHttpErrorToLwError(PbpsErrorCode_t pbpsErrorCode)
{
    switch (pbpsErrorCode)
    {
        case PBPS_HTTP_SUCCESS:
        case PBPS_HTTP_SUCCESS_WITH_CONTENT:
        case PBPS_HTTP_SUCCESS_WITHOUT_CONTENT:
            return LW_ERROR_SUCCESS;

        case PBPS_HTTP_BAD_REQUEST:
           return LW_ERROR_INVALID_MESSAGE;

        case PBPS_HTTP_UNAUTHORIZED:
           return  LW_ERROR_ACCESS_DENIED;

        case PBPS_HTTP_ACCESS_FORBIDDEN:
        case PBPS_HTTP_ACCESS_FORBIDDEN_PERMISSION:
        case PBPS_HTTP_ACCESS_FORBIDDEN_REQUESTOR_ONLY:
        case PBPS_HTTP_ACCESS_FORBIDDEN_APPROVER_ONLY:
        case PBPS_HTTP_ACCESS_FORBIDDEN_REQUEST_NOT_YET_APPROVED:
        case PBPS_HTTP_ACCESS_FORBIDDEN_NOT_ENOUGH_APPROVER:
           return LW_ERROR_PASSWORD_RESTRICTION;

        case PBPS_HTTP_OBJECT_NOT_FOUND:
           return LW_ERROR_NO_SUCH_OBJECT;

        case PBPS_HTTP_CONFLICTING_REQUESTS:
           return LW_ERROR_DOMAINJOIN_PASSWORD_REQUEST_CONFLICT;

        case PBPS_HTTP_API_VERSION_DISABLED:
           return LW_ERROR_SERVICE_NOT_AVAILABLE;

        case PBPS_HTTP_UNEXPECTED_SERVER_ERROR:
        default:
            return LW_ERROR_UNKNOWN;
    }

    return LW_ERROR_UNKNOWN;
}

static
int PbpsApiDebugCallback(
       CURL          *pHandle,
       curl_infotype infoType,
       char          *pData,
       size_t        size,
       void          *pUserData)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   DWORD dwSize = 0;
   PCHAR pStr = NULL;
   PCHAR pFoundPwd = NULL;

   if ((size <= 0) ||
       (infoType == CURLINFO_SSL_DATA_IN) ||   // binary data
       (infoType == CURLINFO_SSL_DATA_OUT))    // binary data
     goto cleanup;

   dwSize = (sizeof(char) * (size+1));
   dwError = LwAllocateMemory(dwSize, OUT_PPVOID(&pStr));
   BAIL_ON_LW_ERROR(dwError);

   memset(pStr, 0, dwSize);

   strncpy(pStr, pData, dwSize-1);
   pStr[dwSize-1] = '\0';
 
   if (infoType == CURLINFO_HEADER_OUT)
   {
       // Scrub out the password before logging. So pwd=[password] is 
       // changed to pwd=[XXXXXXXX]
       if ((pFoundPwd = strstr(pStr, CURL_HEADER_AUTH_PWD_NEEDLE)) != NULL)
       {
          pFoundPwd = pFoundPwd + sizeof(CURL_HEADER_AUTH_PWD_NEEDLE) - 1;
          while ((pFoundPwd) &&
                 (*pFoundPwd != ']') &&
                 (*pFoundPwd != '\0'))  
          {
             *pFoundPwd = 'X';
             pFoundPwd++;
          }
       }
   }

   DJ_LOG_VERBOSE("%s", pStr);

cleanup:

   if (pStr)
     LwFreeMemory(pStr);

   return 0;

error:
   goto cleanup;
}

DWORD
PbpsApiCurlDebugStart(PbpsApi_t *pApi)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                 CURLOPT_DEBUGFUNCTION, 
                                 PbpsApiDebugCallback);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                 CURLOPT_VERBOSE, 1);
   BAIL_ON_CURL_ERROR(curlResult);

cleanup:

   return dwError;

error:
   goto cleanup;
}

DWORD
PbpsApiCurlDebugStop(PbpsApi_t *pApi)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                 CURLOPT_VERBOSE, 0);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                 CURLOPT_DEBUGFUNCTION, 
                                 NULL);
   BAIL_ON_CURL_ERROR(curlResult);

cleanup:
   return dwError;

error:
   goto cleanup;
}



