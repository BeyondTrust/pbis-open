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

#include "domainjoin.h"

static PCSTR methodsPath = "/usr/lib/security/methods.cfg";

DWORD
DJHasMethodsCfg(BOOLEAN *exists)
{
    return CTCheckFileExists(methodsPath, exists);
}

DWORD
DJIsMethodsCfgConfigured(BOOLEAN *configured)
{
    DWORD ceError = ERROR_SUCCESS;
    PCSTR pszRegExp32 = "^[[:space:]]*program[[:space:]]*=[[:space:]]*\\/usr\\/lib\\/security\\/LSASS[[:space:]]*$";
    PCSTR pszRegExp64 = "^[[:space:]]*program_64[[:space:]]*=[[:space:]]*\\/usr\\/lib\\/security\\/LSASS_64[[:space:]]*$";
    BOOLEAN bPatternExists32 = FALSE;
    BOOLEAN bPatternExists64 = FALSE;
    BOOLEAN bFileExists = FALSE;

    *configured = FALSE;

    ceError = CTCheckFileExists(methodsPath, &bFileExists);
    GOTO_CLEANUP_ON_DWORD(ceError);

    if (!bFileExists)
    {
        *configured = TRUE;
        goto cleanup;
    }

    ceError = CTCheckFileHoldsPattern(methodsPath, pszRegExp32, &bPatternExists32);
    GOTO_CLEANUP_ON_DWORD(ceError);

    ceError = CTCheckFileHoldsPattern(methodsPath, pszRegExp64, &bPatternExists64);
    GOTO_CLEANUP_ON_DWORD(ceError);

    if(bPatternExists32 && bPatternExists64)
        *configured = TRUE;

cleanup:
    return ceError;
}

DWORD
DJUnconfigMethodsConfigFile()
{
    BOOLEAN exists;
    DWORD ceError = ERROR_SUCCESS;
    PCSTR exprRemoveBlankLine = "/^$/ {\nN\n/\\nLSASS.*/ D\n}";

    ceError = DJHasMethodsCfg(&exists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(!exists)
        goto error;

    ceError = CTRunSedOnFile(methodsPath, methodsPath, FALSE,
            exprRemoveBlankLine);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTRunSedOnFile(methodsPath, methodsPath, FALSE,
            "/^LSASS.*/d");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTRunSedOnFile(methodsPath, methodsPath, FALSE,
            "/^[ \t]*[^ \t#*].*LSASS.*/d");
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

DWORD
DJFixMethodsConfigFile()
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR pszTmpPath = NULL;
    PSTR pszFinalPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    BOOLEAN isConfigured = FALSE;
    FILE* fp = NULL;

    ceError = DJIsMethodsCfgConfigured(&isConfigured);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(isConfigured)
        goto done;

    ceError = CTGetFileTempPath(
                        methodsPath,
                        &pszFinalPath,
                        &pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTCopyFileWithOriginalPerms(pszFinalPath, pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    if ((fp = fopen(pszTmpPath, "a")) == NULL) {
        ceError = LwMapErrnoToLwError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(fp, "\nLSASS:\n");
    fprintf(fp, "\tprogram = /usr/lib/security/LSASS\n");
    fprintf(fp, "\tprogram_64 = /usr/lib/security/LSASS_64\n");
    fclose(fp); fp = NULL;

    ceError = CTSafeReplaceFile(pszFinalPath, pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

done:
error:

    if (fp)
        fclose(fp);

    if (bRemoveFile)
        CTRemoveFile(pszTmpPath);

    CT_SAFE_FREE_STRING(pszTmpPath);
    CT_SAFE_FREE_STRING(pszFinalPath);

    return ceError;
}
