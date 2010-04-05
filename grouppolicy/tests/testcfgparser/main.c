#include "ctbase.h"

void
ShowUsage()
{
  printf("Usage: testcfgparser <file path> <{1,0} 1 if file has windows double byte format. 0 otherwise>\n");
}

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCFGSECTION pSectionList = NULL;

    if (argc == 2 && !strcmp(argv[1], "-h")) {
      ShowUsage();
      exit(0);
    }
 
    if (argc < 3) {
      ShowUsage();
      ceError = 1;
      BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = GPAParseConfigFile(argv[1], &pSectionList, argv[2] ? 1 : 0);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPASaveConfigSectionListToFile(stdout, pSectionList);
    BAIL_ON_CENTERIS_ERROR(ceError);
 
error:

    if (pSectionList)
       GPAFreeConfigSectionList(pSectionList);

    return(ceError);
}
