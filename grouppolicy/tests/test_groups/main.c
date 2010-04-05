#include "libgpcses.h"

void
ShowUsage()
{
    fprintf(stdout, "Usage: test_groups <group id>\n");
}

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    gid_t gid = 0;

    if (argc < 2) {
       ShowUsage();
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(stdout, "Checking Group ID [%s]\n", argv[1]);

    ceError = CTGetGID(argv[1], &gid);
    if (!CENTERROR_IS_OK(ceError)) {
       fprintf(stderr, "Failed to resolve GID. Error code: [%.8x]\n", ceError);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(stdout, "Resolved GID: [%d]\n", (int)gid);
    
error:

    return (ceError);
}
