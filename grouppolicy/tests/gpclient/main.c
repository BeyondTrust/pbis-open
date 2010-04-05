#include "gpaclient.h"
#include "gpacommon.h"

void
ShowUsage()
{
	printf("Usage: gpaclient ip-address\n");
}

int
ParseArgs(
    int argc,
    char* argv[],
    PSTR pszHostAddress)
{
	int iArg = 1;
	PSTR pszArg = NULL;
	do {
		pszArg = argv[iArg++];
		if (pszArg == NULL || *pszArg == '\0')
		{
			break;
		}
		
		if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
		{
			ShowUsage();
			exit(0);
		}
		else
		{
            strcpy(pszHostAddress, pszArg);
		}
	} while (iArg < argc);
	
	return 0;
}

int
main(
    int argc,
    char* argv[]
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPOCLIENTCONNECTIONCONTEXT pContext = NULL;

    gpa_set_log_level(LOG_LEVEL_VERBOSE);

	ceError = GPOClientOpenContext(&pContext);
    BAIL_ON_CENTERIS_ERROR(ceError);

    GPOClientCloseContext(pContext);
    pContext = NULL;

    GPA_LOG_INFO("Successfully communicated with the GPO Agent.\n");

    return (ceError);

  error:
 
    if (pContext != NULL) {
       GPOClientCloseContext(pContext);
    }

    GPA_LOG_ERROR("Failed communication with the GPO Agent. Error code: [%d] (%s)\n", ceError, CTErrorName(ceError) != NULL ? CTErrorName(ceError) : "Unknown");

    return (ceError);
}
