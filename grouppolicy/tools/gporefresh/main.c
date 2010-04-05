#include "includes.h"

#ifdef BAIL_ON_CENTERIS_ERROR
#undef BAIL_ON_CENTERIS_ERROR
#endif

#define BAIL_ON_CENTERIS_ERROR(ceError) \
    if ((ceError) != 0) goto error;

void
ShowUsage()
{
    printf("Usage: gporefresh\n");
}

int
ParseArgs(
    int argc,
    char* argv[])
{
    CENTERROR ceError = 0;
    int iArg = 1;
    const char* pArg = NULL;

    while (iArg < argc) {
        pArg = argv[iArg++];
        if ((strcmp(pArg, "--help") == 0) || (strcmp(pArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            fprintf(stderr, "Error: Invalid option \n");
            ShowUsage();
            exit(1);
        }
	}
    
    return ceError;
}

CENTERROR
MapError(
    int ceError
    )
{

    switch(ceError)
    {
    case CENTERROR_CONNECTION_REFUSED:
        fprintf(stderr, "Error: Connection refused by gpagentd. Please check if gpagentd is running\n");
        break;
    case CENTERROR_INVALID_OPERATION:
        fprintf(stderr, "Error: Invalid operation\n");
        break;
    case CENTERROR_INVALID_VALUE:
        fprintf(stderr, "Error: Invalid value\n");
        break;
    case CENTERROR_INVALID_PARAMETER:
        fprintf(stderr, "Error: Invalid parameter\n");
        break;
    case CENTERROR_ACCESS_DENIED:
        fprintf(stderr, "Error: Access denied\n");
        break;
    case CENTERROR_OUT_OF_MEMORY:
        fprintf(stderr, "Error: Out of memory\n");
        break;
    case CENTERROR_INVALID_MESSAGE:
        fprintf(stderr, "Error: Invalid message\n");
        break;
    default:
        fprintf(stderr, "Error: GPO Refresh unsuccesfull %s\n",CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");
        break;
    }

    return ceError;
}

int
main(int argc, char* argv[])
{
    CENTERROR ceError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    ceError = ParseArgs(argc, argv);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = gpa_init_logging_to_file(LOG_LEVEL_VERBOSE, "");
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOClientRefresh(hGPConnection);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    GPA_LOG_INFO("GPO Refresh succeeded");

cleanup:


    if (hGPConnection != (HANDLE)NULL)
    {
        GPOClientCloseContext(hGPConnection);
    }

    return ceError;

error:

    MapError(ceError);

    goto cleanup;
}
