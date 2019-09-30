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


DWORD
PbpsApiInitialize(PbpsApi_t **ppApi)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   PbpsApi_t *pApiTmp = NULL;

   dwError = LwAllocateMemory(sizeof(PbpsApi_t), PPCAST(&pApiTmp));
   BAIL_ON_LW_ERROR(dwError);


   pApiTmp->config.dwVersionTemplate = 0;
   pApiTmp->config.pszUrlBase = NULL;
   pApiTmp->config.pszRunAsUser = NULL;
   pApiTmp->config.pszRunAsUserPwd = NULL;
   pApiTmp->config.pszApiKey = NULL;
   pApiTmp->config.pszHeaderAuth = NULL;
   pApiTmp->config.pszJoinAccount = NULL;
   pApiTmp->config.dwDurationMinutes = PBPSAPI_DEFAULT_DURATION_MINUTES;
   pApiTmp->config.pszCertFileClient = NULL;
   pApiTmp->config.pszCertFileCA = NULL;

   pApiTmp->session.bDoSignout = FALSE;
   pApiTmp->session.bDoCheckin = FALSE;
   pApiTmp->session.dwRequestId = 0;
   pApiTmp->session.pCurlHandle = NULL;
   pApiTmp->session.pHeaderList = NULL;
   pApiTmp->session.pManagedAccountList = NULL;

   pApiTmp->session.pCurlHandle = curl_easy_init();
   if (pApiTmp->session.pCurlHandle == NULL)
   {
      BAIL_WITH_CURL_ERROR(CURLE_OUT_OF_MEMORY);
   }

   *ppApi = pApiTmp;

cleanup:

   return dwError;
error:
   goto cleanup;
}

VOID
PbpsApiRelease(PbpsApi_t *pApi)
{
   if (!pApi)
     return;

   if (pApi->config.pszUrlBase)
       LW_SAFE_FREE_STRING(pApi->config.pszUrlBase);

   if (pApi->config.pszRunAsUser)
       LW_SAFE_FREE_STRING(pApi->config.pszRunAsUser);

   if (pApi->config.pszRunAsUserPwd)
       LW_SAFE_FREE_STRING(pApi->config.pszRunAsUserPwd);

   if (pApi->config.pszApiKey)
       LW_SAFE_FREE_STRING(pApi->config.pszApiKey);

   if (pApi->config.pszHeaderAuth)
       LW_SAFE_FREE_STRING(pApi->config.pszHeaderAuth);

   if (pApi->config.pszJoinAccount)
       LW_SAFE_FREE_STRING(pApi->config.pszJoinAccount);

   if (pApi->config.pszCertFileClient)
       LW_SAFE_FREE_STRING(pApi->config.pszCertFileClient);

   if (pApi->config.pszCertFileCA)
       LW_SAFE_FREE_STRING(pApi->config.pszCertFileCA);

   if (pApi->session.pHeaderList != NULL)
   {
      curl_slist_free_all(pApi->session.pHeaderList);
      pApi->session.pHeaderList = NULL;
   }

   if (pApi->session.pCurlHandle)
     curl_easy_cleanup(pApi->session.pCurlHandle);

   if (pApi->session.pManagedAccountList != NULL)
      PbpsManagedAccountListFree(pApi->session.pManagedAccountList);

   LwFreeMemory(pApi);

   return;
}

static
DWORD
PbpsCurlSlistAppend(
        struct curl_slist **ppStringList,
        const char *string
        )
{
    struct curl_slist * pStringItem = NULL;
    DWORD dwError= LW_ERROR_SUCCESS;

    if (string == NULL)
    {
      // nothing to do
      BAIL_ON_LW_ERROR(dwError);
    }

    pStringItem = curl_slist_append(
                    *ppStringList,
                    string);
    if (pStringItem == NULL)
    {
        BAIL_WITH_CURL_ERROR(CURLE_OUT_OF_MEMORY);
    }

    if (*ppStringList == NULL)
    {
        *ppStringList = pStringItem;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
PbpsApiCurlHeaderSet(PbpsApi_t *pApi)
{
   DWORD dwError = LW_ERROR_SUCCESS;
   CURLcode curlResult = CURLE_OK;

   if (pApi->session.pHeaderList != NULL)
   {
      curl_slist_free_all(pApi->session.pHeaderList);
      pApi->session.pHeaderList = NULL;
   }

   dwError = PbpsCurlSlistAppend(
                   &pApi->session.pHeaderList,
                   CURL_HEADER_CONTENT_FORMAT);
   BAIL_ON_LW_ERROR(dwError);

   dwError = PbpsCurlSlistAppend(
                   &pApi->session.pHeaderList,
                   pApi->config.pszHeaderAuth);
   BAIL_ON_LW_ERROR(dwError);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_HTTPHEADER,
                   pApi->session.pHeaderList);
   BAIL_ON_CURL_ERROR(curlResult);

   // Cookie is used for subsequent http calls to PBPS.
   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_COOKIEJAR,
                   NULL);
   BAIL_ON_CURL_ERROR(curlResult);

   curlResult = curl_easy_setopt(
                   pApi->session.pCurlHandle,
                   CURLOPT_SSLCERTTYPE,
                   PBPS_CURLOPT_SSLCERTTYPE);
   BAIL_ON_CURL_ERROR(curlResult);

   if (pApi->config.pszCertFileCA)
   {
      // PBPS server certificate. Curl --cacert
      DJ_LOG_ERROR("Setting CAINFO: %s", pApi->config.pszCertFileCA);
      curlResult = curl_easy_setopt(
                      pApi->session.pCurlHandle,
                      CURLOPT_CAINFO,
                      pApi->config.pszCertFileCA);
      BAIL_ON_CURL_ERROR(curlResult);

      curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                    CURLOPT_SSL_VERIFYPEER, 1L);
      BAIL_ON_CURL_ERROR(curlResult);
   }
   else
   {
      curlResult = curl_easy_setopt(pApi->session.pCurlHandle,
                                    CURLOPT_SSL_VERIFYPEER, 0);
      BAIL_ON_CURL_ERROR(curlResult);
   }

   if (pApi->config.pszCertFileClient)
   {
      // PBPS client certificate. Curl --cert
      DJ_LOG_INFO("Setting CURLOPT_SSLCERT: %s", pApi->config.pszCertFileClient);
      curlResult = curl_easy_setopt(
                      pApi->session.pCurlHandle,
                      CURLOPT_SSLCERT,
                      pApi->config.pszCertFileClient);
      BAIL_ON_CURL_ERROR(curlResult);

      DJ_LOG_INFO("Setting SSLKEY:", pApi->config.pszCertFileClient);
      curlResult = curl_easy_setopt(
               pApi->session.pCurlHandle,
               CURLOPT_SSLKEY,
               pApi->config.pszCertFileClient);
      BAIL_ON_CURL_ERROR(curlResult);
   }

cleanup:

   return dwError;

error:
   goto cleanup;
}


/*
 * curl invoked function to store response from a curl request.
 */
size_t PbpsApiWriteCallBackFunction(
           char *pNewData, size_t newSize,
           size_t nmemb, void *userData)

{
   DWORD dwError = LW_ERROR_SUCCESS;
   size_t  newDataSize = newSize * nmemb;
   responseBuffer_t *pUserData = (responseBuffer_t *) userData;

   dwError = LwReallocMemory(pUserData->buffer,
                             (PVOID*)&pUserData->buffer,
                             newDataSize + pUserData->size + 1);
   BAIL_ON_LW_ERROR(dwError);

   memcpy(&(pUserData->buffer[pUserData->size]), pNewData, newDataSize);

   pUserData->size += newDataSize;
   pUserData->buffer[pUserData->size] = 0;

cleanup:
   return newDataSize;

error:
   goto cleanup;
}


size_t
PbpsApiReadCallBackFunction(void *ptr, size_t size, size_t nmemb, void *stream)
{
  DWORD dwLength = 0;
  responseBuffer_t *checkInReason = (responseBuffer_t*) stream;

  dwLength = checkInReason->size;

  if (dwLength == 0)
    goto cleanup;

  if (dwLength > nmemb)
  {
     DJ_LOG_ERROR("PbpsApiReadCallBackFunction truncating from %d to %d",
                  dwLength, (DWORD) nmemb);
     dwLength = nmemb;
  }

  memcpy(ptr, checkInReason->buffer, dwLength);

  // The next time curl calls this function, dwLength would be zero
  // which tells curl we're done.
  checkInReason->size = 0;

cleanup:
  return dwLength;
}

