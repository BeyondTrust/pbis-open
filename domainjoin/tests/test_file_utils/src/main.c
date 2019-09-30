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

#define TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER 1

#if TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER
DWORD
TestGetMatchingFilePathsInFolder()
{
    DWORD ceError = ERROR_SUCCESS;
    PSTR* ppszFilePaths = NULL;
    DWORD nPaths = 0;
    int   iPath = 0;

    ceError = CTGetMatchingFilePathsInFolder("/etc",
                                             "nss.*",
                                             &ppszFilePaths,
                                             &nPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (iPath = 0; iPath < nPaths; iPath++)
        printf("File path:%s\n", *(ppszFilePaths+iPath));

    if (nPaths == 0)
       printf("No paths were found\n");

error:

    if (ppszFilePaths)
       CTFreeStringArray(ppszFilePaths, nPaths);

    return ceError;
}
#endif

int
main(int argc, char* argv[])
{
    DWORD ceError = ERROR_SUCCESS;
 
#if TEST_CREATE_DIRECTORY
    ceError = DJCreateDirectory("/tmp/mydir", S_IRUSR);
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

#if TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER
    ceError = TestGetMatchingFilePathsInFolder();
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif
 
error:

    return(ceError);
}
