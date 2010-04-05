void display_status(char *msg, OM_uint32 maj_stat, OM_uint32 min_stat);

void display_status_1(char *m, OM_uint32 code, int type);

int
strupr(char *szDomainName);

CENTERROR
ConvertDomainToDN(
    PSTR pszDomainName,
    PSTR * ppszDomainDN
    );
