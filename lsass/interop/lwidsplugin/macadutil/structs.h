#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _ADU_DIRECTORY_CONTEXT
{
    LDAP *ld;

} ADU_DIRECTORY_CONTEXT, *PADU_DIRECTORY_CONTEXT;

typedef struct _ADU_CRED_CONTEXT
{
    PSTR                pszPrincipalName;
    PSTR                pszCachePath;
    PSTR                pszSID;
    uid_t               uid;
    BOOLEAN             bDestroyCachePath;

    LW_PIO_CREDS pAccessToken;

} ADU_CRED_CONTEXT, *PADU_CRED_CONTEXT;

#ifndef AD_USER_ATTRIBUTES_DEFINED
#define AD_USER_ATTRIBUTES_DEFINED 1

typedef struct __AD_USER_ATTRIBUTES
{
    /* Name attributes */
    PSTR  pszDisplayName;
    PSTR  pszFirstName;
    PSTR  pszLastName;
    PSTR  pszADDomain;
    PSTR  pszKerberosPrincipal;

    /* Email attributes */
    PSTR  pszEMailAddress;
    PSTR  pszMSExchHomeServerName;
    PSTR  pszMSExchHomeMDB;

    /* Phone attributes */
    PSTR  pszTelephoneNumber;
    PSTR  pszFaxTelephoneNumber;
    PSTR  pszMobileTelephoneNumber;

    /* Address attributes */
    PSTR  pszStreetAddress;
    PSTR  pszPostOfficeBox;
    PSTR  pszCity;
    PSTR  pszState;
    PSTR  pszPostalCode;
    PSTR  pszCountry;

    /* Work attributes */
    PSTR  pszTitle;
    PSTR  pszCompany;
    PSTR  pszDepartment;

    /* Network setting attributes */
    PSTR  pszHomeDirectory;
    PSTR  pszHomeDrive;
    PSTR  pszPasswordLastSet;
    PSTR  pszUserAccountControl;
    PSTR  pszMaxMinutesUntilChangePassword;
    PSTR  pszMinMinutesUntilChangePassword;
    PSTR  pszMaxFailedLoginAttempts;
    PSTR  pszAllowedPasswordHistory;
    PSTR  pszMinCharsAllowedInPassword;

} AD_USER_ATTRIBUTES, *PAD_USER_ATTRIBUTES;

#endif /* AD_USER_ATTRIBUTES_DEFINED */


#endif /* __STRUCTS_H__ */
