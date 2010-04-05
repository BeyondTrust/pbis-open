
#include "includes.h"


void display_status(char *msg, OM_uint32 maj_stat, OM_uint32 min_stat);

void display_status_1(char *m, OM_uint32 code, int type);

int
strupr(char *szDomainName);

CENTERROR
ConvertDomainToDN(
    PSTR pszDomainName,
    PSTR * ppszDomainDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszChar = NULL;
    PSTR pszDomainDN = NULL;
    CHAR  pszBuffer[256];

    memset(pszBuffer, 0, sizeof(pszBuffer));
    while ((pszChar = strchr(pszDomainName, '.'))) {
        strcat(pszBuffer,"dc=");
        strncat(pszBuffer, pszDomainName, pszChar - pszDomainName);
        strcat(pszBuffer,",");
        pszDomainName = pszChar+1;
    }
    strcat(pszBuffer, "dc=");
    strcat(pszBuffer, pszDomainName);

    ceError = LwAllocateString(pszBuffer, &pszDomainDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszDomainDN = pszDomainDN;

    return(ceError);

error:

    *ppszDomainDN = NULL;
    return(ceError);
}



int
strupr(char *szDomainName)
{
    if (!szDomainName) {
        return(0);
    }
    while (*szDomainName != '\0'){
        *szDomainName = toupper(*szDomainName);
        szDomainName++;
    }
    return (0);
}

void display_status(char *msg, OM_uint32 maj_stat, OM_uint32 min_stat)
{
    display_status_1(msg, maj_stat, GSS_C_GSS_CODE);
    display_status_1(msg, min_stat, GSS_C_MECH_CODE);
}

void display_status_1(char *m, OM_uint32 code, int type)
{
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1) {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        GPA_LOG_VERBOSE("GSS-API error calling %s: %d (%s)\n", m, code, (char *)msg.value);
        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

