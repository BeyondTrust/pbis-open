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
#include <toml.h>


/*
 *
 * Read contents of the config file into the config structure.
 * The certificate values in the config file are written out
 * to /etc/pbis/pbpsClient.pem and /etc/pbis/pbpsServer.pem
 *
 */
DWORD
PbpsApiGetConfig(PbpsApi_t *pApi, PSTR pszConfigFile)
{
   DWORD         dwError = LW_ERROR_SUCCESS;
   BOOLEAN       bExists = FALSE;
   FILE          *pConfigFp = NULL;
   FILE          *pFp = NULL;
   char          errorBuffer[200];
   toml_table_t* pConfig = NULL;
   toml_table_t* pSectionVersion = NULL;
   toml_table_t* pSectionDomainJoin = NULL;
   toml_table_t* pSectionPbps = NULL;
   PSTR          pszValueStr = NULL;
   PCSTR         pszTomlRaw = NULL;
   DWORD         dwValue = 0;

   dwError = LwCheckFileExists(pszConfigFile, &bExists);
   BAIL_ON_LW_ERROR(dwError);

   if (!bExists)
   {
      dwError = ERROR_FILE_NOT_FOUND;
      BAIL_ON_LW_ERROR(dwError);
   }

   dwError = CTOpenFile(pszConfigFile,
                        "r",
                         &pConfigFp);
   BAIL_ON_LW_ERROR(dwError);

   pConfig = toml_parse_file(pConfigFp, errorBuffer, sizeof(errorBuffer));

   dwError = CTCloseFile(pConfigFp);
   BAIL_ON_LW_ERROR(dwError);

   if (pConfig == NULL)
   {
      DJ_LOG_ERROR("Failed to parse %s. Reason:%s", pszConfigFile, errorBuffer);
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_CANNOT_PARSE;
      BAIL_ON_LW_ERROR(dwError);
   }

   //
   // Section Version
   //
   pSectionVersion = toml_table_in(pConfig, "Version");
   if (!pSectionVersion)
   {
      DJ_LOG_ERROR("Section Version missing in %s", pszConfigFile);
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_SECTION;
      BAIL_ON_LW_ERROR(dwError);
   }


   // Template attribute
   if ((pszTomlRaw = toml_raw_in(pSectionVersion, "Template")) == 0)
   {
      DJ_LOG_ERROR("Section Version missing Template");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_ATTRIBUTE;
      BAIL_ON_LW_ERROR(dwError);
   }

   if (toml_rtoi(pszTomlRaw, (int64_t*)&dwValue) != 0)
   {
       DJ_LOG_ERROR("Section Version Template invalid value");
       dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
       BAIL_ON_LW_ERROR(dwError);
   }

   if (dwValue == 1)
   {
      pApi->config.dwVersionTemplate = dwValue;
   }
   else
   {
      DJ_LOG_ERROR("Section Version Template %d not valid. Expected 1",
                      dwValue);
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_OUT_OF_RANGE;
      BAIL_ON_LW_ERROR(dwError);
   }


   //
   // Section DomainJoin
   //
   pSectionDomainJoin = toml_table_in(pConfig, "DomainJoin");
   if (!pSectionDomainJoin)
   {
      DJ_LOG_ERROR("Section DomainJoin missing in %s", pszConfigFile);
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_SECTION;
      BAIL_ON_LW_ERROR(dwError);
   }

   // DomainJoinUser attribute
   if ((pszTomlRaw = toml_raw_in(pSectionDomainJoin, "DomainJoinUser")) == 0)
   {
      DJ_LOG_ERROR("Section DomainJoin missing DomainJoinUser");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_ATTRIBUTE;
      BAIL_ON_LW_ERROR(dwError);
   }

   if (toml_rtos(pszTomlRaw, &pszValueStr) != 0)
   {
      DJ_LOG_ERROR("Section DomainJoin DomainJoinUser invalid string");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
      BAIL_ON_LW_ERROR(dwError);
   }

   dwError = LwAllocateStringPrintf(
                    &(pApi->config.pszJoinAccount), "%s",
                    pszValueStr);
   BAIL_ON_LW_ERROR(dwError);

   LwStrToLower(pApi->config.pszJoinAccount);

   LW_SAFE_FREE_STRING(pszValueStr);
   pszValueStr = NULL;


   //
   // Section PasswordSafe
   //
   pSectionPbps = toml_table_in(pConfig, "PasswordSafe");
   if (!pSectionPbps)
   {
      DJ_LOG_ERROR("Section PasswordSafe missing in %s", pszConfigFile);
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_SECTION;
      BAIL_ON_LW_ERROR(dwError);
   }

   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "ServerUrl")) == 0)
   {
      DJ_LOG_ERROR("Section PasswordSafe missing ServerUrl");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_ATTRIBUTE;
      BAIL_ON_LW_ERROR(dwError);
   }

   if (toml_rtos(pszTomlRaw, &pszValueStr))
   {
      DJ_LOG_ERROR("Section PasswordSafe ServerUrl invalid string");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
      BAIL_ON_LW_ERROR(dwError);
   }

   dwError = LwAllocateStringPrintf(
                    &(pApi->config.pszUrlBase), "%s/BeyondTrust/api/public/v3",
                    pszValueStr);
   BAIL_ON_LW_ERROR(dwError);

   LW_SAFE_FREE_STRING(pszValueStr);
   pszValueStr = NULL;

   // RunAsUser
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "RunAsUser")) == 0)
   {
      DJ_LOG_ERROR("Section PasswordSafe missing RunAsUser");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_ATTRIBUTE;
      BAIL_ON_LW_ERROR(dwError);
   }

   if (toml_rtos(pszTomlRaw, &pszValueStr) != 0)
   {
      DJ_LOG_ERROR("Section PasswordSafe RunAsUser invalid string");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
      BAIL_ON_LW_ERROR(dwError);
   }

   dwError = LwAllocateStringPrintf(
                    &(pApi->config.pszRunAsUser), "%s",
                    pszValueStr);
   BAIL_ON_LW_ERROR(dwError);

   LW_SAFE_FREE_STRING(pszValueStr);
   pszValueStr = NULL;

   // RunAsUserPassword
   // Optional. Needed if Password Safe web console shows
   // "User Password Required" is checked in Configuration->API Registration.
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "RunAsUserPassword")) != 0)
   {
      if (toml_rtos(pszTomlRaw, &pszValueStr) == 0)
      {
         dwError = LwAllocateStringPrintf(
                          &(pApi->config.pszRunAsUserPwd), "%s",
                          pszValueStr);
         BAIL_ON_LW_ERROR(dwError);
      }
      else
      {
         DJ_LOG_ERROR("Section PasswordSafe RunAsUserPassword invalid string");
         dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
         BAIL_ON_LW_ERROR(dwError);
      }
   }

   LW_SAFE_FREE_STRING(pszValueStr);
   pszValueStr = NULL;

   // ApiKey
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "ApiKey")) == 0)
   {
      DJ_LOG_ERROR("Section PasswordSafe missing ApiKey");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_MISSING_ATTRIBUTE;
      BAIL_ON_LW_ERROR(dwError);
   }

   if (toml_rtos(pszTomlRaw, &pszValueStr) != 0)
   {
      DJ_LOG_ERROR("Section PasswordSafe ApiKey invalid string");
      dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
      BAIL_ON_LW_ERROR(dwError);
   }

   dwError = LwAllocateStringPrintf(
                    &(pApi->config.pszApiKey), "%s",
                    pszValueStr);
   BAIL_ON_LW_ERROR(dwError);


   LW_SAFE_FREE_STRING(pszValueStr);
   pszValueStr = NULL;


   // DurationMinutes
   pApi->config.dwDurationMinutes = PBPSAPI_DEFAULT_DURATION_MINUTES;
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "DurationMinutes")) != 0)
   {
      if (toml_rtoi(pszTomlRaw, (int64_t*)&dwValue) != 0)
      {
         DJ_LOG_ERROR("Section PasswordSafe DurationMinutes invalid value");
         dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
         BAIL_ON_LW_ERROR(dwError);
      }

      if (dwValue >= PBPSAPI_MINIMUM_DURATION_MINUTES &&
          dwValue <= PBPSAPI_MAXIMUM_DURATION_MINUTES)
      {
         pApi->config.dwDurationMinutes = dwValue;
      }
      else
      {
         DJ_LOG_ERROR("Section PasswordSafe DurationMinutes %d out of range %d to %d",
                      dwValue,
                      PBPSAPI_MINIMUM_DURATION_MINUTES,
                      PBPSAPI_MAXIMUM_DURATION_MINUTES);

         dwError = LW_ERROR_DOMAINJOIN_CONFIG_OUT_OF_RANGE;
         BAIL_ON_LW_ERROR(dwError);
      }
   }



   // Client Certificate
   // Needed if Password Safe web console shows "Client Certificate
   // Required" is checked in Configuration->API Registration.
   pApi->config.pszCertFileClient = NULL;
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "ClientCertificate")) != 0)
   {
      if (toml_rtos(pszTomlRaw, &pszValueStr) != 0)
      {
         DJ_LOG_ERROR("Section PasswordSafe ClientCertificate invalid string");
         dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
         BAIL_ON_LW_ERROR(dwError);
      }

      dwError = LwAllocateStringPrintf(
                      &(pApi->config.pszCertFileClient), "%s",
                      "/etc/pbis/pbpsClient.pem");
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTOpenFile(pApi->config.pszCertFileClient, "w", &pFp);
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTFilePrintf(pFp, "%s", pszValueStr);
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTCloseFile(pFp);
      BAIL_ON_LW_ERROR(dwError);

      LwChangePermissions(pApi->config.pszCertFileClient, S_IRUSR|S_IWUSR);

      LW_SAFE_FREE_STRING(pszValueStr);
      pszValueStr = NULL;
   }


   // CA Certificate
   // Needed if BeyondTrust AD Bridge agent needs to verify PBPS server certificate.
   pApi->config.pszCertFileCA = NULL;
   if ((pszTomlRaw = toml_raw_in(pSectionPbps, "CACertificate")) != 0)
   {
      if (toml_rtos(pszTomlRaw, &pszValueStr) != 0)
      {
         DJ_LOG_ERROR("Section PasswordSafe CACertificate invalid string");
         dwError = LW_ERROR_DOMAINJOIN_CONFIG_BAD_VALUE;
         BAIL_ON_LW_ERROR(dwError);
      }

      dwError = LwAllocateStringPrintf(
                      &(pApi->config.pszCertFileCA), "%s",
                      "/etc/pbis/pbpsCA.pem");
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTOpenFile(pApi->config.pszCertFileCA, "w", &pFp);
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTFilePrintf(pFp, "%s", pszValueStr);
      BAIL_ON_LW_ERROR(dwError);

      dwError = CTCloseFile(pFp);
      BAIL_ON_LW_ERROR(dwError);

      LwChangePermissions(pApi->config.pszCertFileCA, S_IRUSR|S_IWUSR);

      LW_SAFE_FREE_STRING(pszValueStr);
      pszValueStr = NULL;
   }


   if (pApi->config.pszRunAsUserPwd)
   {
      dwError = LwAllocateStringPrintf(
                       &(pApi->config.pszHeaderAuth),
                       CURL_HEADER_AUTH_FORMAT_WITH_PWD,
                       pApi->config.pszApiKey,
                       pApi->config.pszRunAsUser,
                       pApi->config.pszRunAsUserPwd);
   }
   else
   {
      dwError = LwAllocateStringPrintf(
                       &(pApi->config.pszHeaderAuth), CURL_HEADER_AUTH_FORMAT,
                       pApi->config.pszApiKey,
                       pApi->config.pszRunAsUser);
   }
   BAIL_ON_LW_ERROR(dwError);


cleanup:

   if (pConfig)
     toml_free(pConfig);

   if (pszValueStr)
     LW_SAFE_FREE_STRING(pszValueStr);

   return dwError;

error:
   goto cleanup;
}
