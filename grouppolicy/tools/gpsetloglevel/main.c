#include "includes.h"

#ifdef BAIL_ON_CENTERIS_ERROR
#undef BAIL_ON_CENTERIS_ERROR
#endif

#define BAIL_ON_CENTERIS_ERROR(ceError) \
    if ((ceError) != 0) goto error;

void
ShowUsage()
{
    printf("Usage: gp-set-log-level {error, warning, info, verbose}\n");
}

int
ParseArgs(
    int argc,
    char* argv[],
    PDWORD pdwLogLevel)
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwLogLevel    = LOG_LEVEL_ERROR;
    BOOLEAN bLogLevelSpecified = FALSE;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        switch (parseMode)
        {
            case PARSE_MODE_OPEN:
        
                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else
                {
                    if (!strcasecmp(pszArg, "error"))
                    {
                        dwLogLevel = LOG_LEVEL_ERROR;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "warning"))
                    {
                        dwLogLevel = LOG_LEVEL_WARNING;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "info"))
                    {
                        dwLogLevel = LOG_LEVEL_INFO;
                        bLogLevelSpecified = TRUE;
                    }
                    else if (!strcasecmp(pszArg, "verbose"))
                    {
                        dwLogLevel = LOG_LEVEL_VERBOSE;
                        bLogLevelSpecified = TRUE;
                    }
                    else
                    {
                        ShowUsage();
                        exit(1);
                    }
                }
                break;
        }
        
    } while (iArg < argc);
    
    if (!bLogLevelSpecified)
    {
        ShowUsage();
        exit(1);
    }

    *pdwLogLevel = dwLogLevel;
    
    return dwError;
}

CENTERROR
MapError(
    int ceError
    )
{

    switch(ceError)
    {
    case CENTERROR_GP_SETLOGLEVEL_FAILED:
        fprintf(stderr, "Error: Setloglevel failed\n");
        break;
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
        fprintf(stderr, "Error: Set log level was unsuccesfull\n");
        break;
    }

    return ceError;
}

int
main(int argc, char* argv[])
{
    CENTERROR ceError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;
    DWORD dwLogLevel = LOG_LEVEL_ALWAYS;

    ceError = ParseArgs(argc, argv,&dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOClientOpenContext(&hGPConnection);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = GPOSetLogLevel(hGPConnection,dwLogLevel);
    BAIL_ON_CENTERIS_ERROR(ceError);
    
    fprintf(stdout, "The log level was set successfully\n\n");

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
