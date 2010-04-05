#include "ctbase.h"
#include "macadutil.h"
#include "gpldap.h"
#include "dns.h"
#include "Utilities.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

/* used by inet_addr, not defined on Solaris anywhere!? */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) -1)
#endif

long
GetDnsHostName(
    char ** hostname
    )
{
    long macError = eDSNoErr;
    char buffer[256];
    char * szLocal = NULL;
    char * szDot = NULL;
    uint len = 0;
    PSTR pszHostName = NULL;

    if ( gethostname(buffer, sizeof(buffer)) != 0 )
    {
        macError = ePlugInInitError;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    len = strlen(buffer);
    if ( len > strlen(".local") )
    {
        szLocal = &buffer[len - strlen(".local")];
        if ( !strcasecmp( szLocal, ".local" ) )
        {
            szLocal[0] = '\0';
        }
    }

    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    szDot = strchr(buffer, '.');
    if ( szDot )
    {
        szDot[0] = '\0';
    }

    macError = LWAllocateString(buffer, &pszHostName);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if ( hostname ) {
        *hostname = pszHostName;
        pszHostName = NULL;
    }

    // return below so that we free the unrequested out parameters.

cleanup:

    if ( pszHostName )
    {
        LWFreeString(pszHostName);
    }

    return macError;
}
