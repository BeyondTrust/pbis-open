#include <stdio.h>
#include <stdlib.h>

#include <lwio/srvshareapi.h>


#define IOCTL_NAME_RELOAD   "reload"


static
void
Usage(
    void)
{
    fprintf(stderr, "Usage:\n\ttest_srv_share_ioctl %s\n", IOCTL_NAME_RELOAD);
    exit(1);
}


int
main(
    int argc,
    char** argv
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (argc != 2 || strcmp(argv[1], IOCTL_NAME_RELOAD))
    {
        Usage();
    }

    ntStatus = LwIoSrvShareReloadConfiguration();
    if (ntStatus)
    {
        fprintf(stderr, "ERROR: 0x%08X (%s)\n", ntStatus,
                LwNtStatusToName(ntStatus));
        return 1;
    }

    return 0;
}
