#include "ctbase.h"

//#define TEST_FILE_HOLDS_PATTERN  1
#define TEST_MATCHING_FILES 1

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*BOOLEAN bHoldsPattern = FALSE;*/
#if TEST_MATCHING_FILES
    PSTR*   ppszFilePaths = NULL;
    DWORD   dwNPaths = 0;
    DWORD   iPath = 0;
#endif
 
#if TEST_CREATE_DIRECTORY
    ceError = LwCreateDirectory("/tmp/mydir", S_IRUSR);
    BAIL_ON_CENTERIS_ERROR(ceError);
#endif

#if TEST_FILE_HOLDS_PATTERN
    if (argc < 3) {
       printf("Usage: testfutils <file path> <pattern>\n");
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }
    ceError = CTCheckFileHoldsPattern(argv[1],
                                      argv[2],
                                      &bHoldsPattern);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if (bHoldsPattern)
       printf("File holds pattern\n");
    else
       printf("File does not hold pattern\n");
#endif

#if TEST_MATCHING_FILES
    if (argc < 3) {
       printf("Usage: testfutils <dir path> <file name pattern>\n");
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTGetMatchingFilePathsInFolder(argv[1],
                                             argv[2],
                                             &ppszFilePaths,
                                             &dwNPaths
                                            );
    BAIL_ON_CENTERIS_ERROR(ceError);
    for (iPath = 0; iPath < dwNPaths; iPath++)
        printf("%s\n", *(ppszFilePaths+iPath));
#endif
 
error:

#if TEST_MATCHING_FILES
    if (ppszFilePaths)
       LwFreeStringArray(ppszFilePaths, dwNPaths);
#endif

    return(ceError);
}
