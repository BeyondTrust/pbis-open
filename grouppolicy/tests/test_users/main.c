#include "libgpcses.h"

void
ShowUsage()
{
    fprintf(stdout, "Usage: test_users <user id>\n");
}

int
main(int argc, char* argv[])
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    uid_t uid = 0;

    if (argc < 2) {
       ShowUsage();
       ceError = CENTERROR_INVALID_PARAMETER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(stdout, "Checking User ID [%s]\n", argv[1]);

    ceError = CTGetUID(argv[1], &uid);
    if (!CENTERROR_IS_OK(ceError)) {
       fprintf(stderr, "Failed to resolve UID. Error code: [%.8x]\n", ceError);
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(stdout, "Resolved UID: [%d]\n", (int)uid);
    
error:

    return (ceError);
}
