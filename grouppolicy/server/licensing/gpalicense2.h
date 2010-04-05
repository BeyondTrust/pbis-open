#ifndef __GPALICENSE2_H__
#define __GPALICENSE2_H__

#include "gpacommon.h"

CENTERROR
GPAGetAvailableLicenses(
    HANDLE hDirectory,
    PSTR pszLicenseRoot,
    PSTR** pppLicenses,
    PDWORD pdwCount,
    BOOLEAN bUseSiteLicense,
    PSTR pszGUID
    );

CENTERROR
GPAGetAssignedLicense(
    HANDLE hDirectory,
    PSTR pszLicenseRoot,
    PSTR** pppLicenses,
    PDWORD pdwCount,
    BOOLEAN bUseSiteLicense,
    PSTR pszGUID
    );

CENTERROR
GPAAssignLicense(
    HANDLE hDirectory,
    PSTR   pszLicenseDN,
    PSTR   pszGUID
    );
    
CENTERROR
CheckLicense(
    HANDLE hDirectory,
    PSTR pszHostname,
    PSTR pszADDomain,
    PSTR pszDomainControllerName
    );

#endif /* __GPALICENSE2_H__ */
