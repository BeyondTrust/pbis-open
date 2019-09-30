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
 * GET Credentials/{requestId}
 *    dwRequestId: request id from POST Requests
 *    ppCredentials: Returns the credentials which must be freed by the
 *                   caller.
 *  
 */
DWORD
PbpsApiCredentialsGet(
   PbpsApi_t *pApi,
   DWORD dwRequestId,
   PSTR *ppszCredentials)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;
   long responseCode = 0;
   PSTR pszUrl = NULL;
   PSTR pszFunction = "Credentials";
   PSTR pszCredentials = NULL;
   responseBuffer_t responseBuffer;

   responseBuffer.buffer = NULL;
   responseBuffer.size = 0;

   dwError = LwAllocateStringPrintf(
               &pszUrl, "%s/%s/%d", pApi->config.pszUrlBase, pszFunction, dwRequestId);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_URL, pszUrl);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(pApi->session.pCurlHandle, CURLOPT_HTTPGET, 1);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_WRITEFUNCTION,
                   PbpsApiWriteCallBackFunction);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_WRITEDATA,
                   (void*) &responseBuffer);
   BAIL_ON_CURL_ERROR(curlResult);

   DJ_LOG_INFO("%s get url:%s", pszFunction, pszUrl);

   curlResult = curl_easy_perform(pApi->session.pCurlHandle);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_getinfo(
                   pApi->session.pCurlHandle,
                   CURLINFO_RESPONSE_CODE,
                   &responseCode);
  BAIL_ON_CURL_ERROR(curlResult);

  if (responseCode != PBPS_HTTP_SUCCESS)
  {
     DJ_LOG_ERROR("HTTP failed. Response code:%ld Response:%s(%d)",
                  responseCode,
                  responseBuffer.buffer,
                  responseBuffer.size);
     dwError = PbpsHttpErrorToLwError(responseCode);
     BAIL_ON_LW_ERROR(dwError);
  }

  if ((responseBuffer.size > 0) && (responseBuffer.buffer))
  {
     dwError = LwAllocateStringPrintf(
                 &pszCredentials, "%s", responseBuffer.buffer);
     BAIL_ON_LW_ERROR(dwError);

     LwStripTrailingCharacters(pszCredentials, '"');
     LwStripLeadingCharacters(pszCredentials, '"');
  }

cleanup:

  *ppszCredentials = pszCredentials;

  if (responseBuffer.buffer)
    LwFreeMemory(responseBuffer.buffer);

   LW_SAFE_FREE_STRING(pszUrl);

   return dwError;

error:
   goto cleanup;
}

