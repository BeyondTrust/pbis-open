/*
 *  gpmacinterface.c
 *  lwimacinterface
 *
 *  Created by Sriram Nambakam on 8/21/07.
 *  Copyright 2007 Likewise Software, Inc. All rights reserved.
 *
 */

#include "lwimaciface_security.h"
#include "lwimacifaceutils.h"
#include "gpplistutils.h"
#include <unistd.h>

int
RequirePasswordForEachSecureSystemPreference(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFMutableDictionaryRef dictRef = NULL;
    CFMutableDictionaryRef rightsRef = NULL;
    CFMutableDictionaryRef sysPrefsRef = NULL;
    CFMutableDictionaryRef releaseRightsRef = NULL;
    CFMutableDictionaryRef releaseSysPrefsRef = NULL;
    CFBooleanRef valRef = NULL;
    Boolean bNeedsSaving = FALSE;
    /* If passwords are required, shared is set to "No" */
    Boolean bShared = !bValue;
    
    result = GPReadPropertyListFile( CFSTR(AUTHORIZATION_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListXMLFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }
    
    dictRef = (CFMutableDictionaryRef)pList;
    rightsRef = (CFMutableDictionaryRef)CFDictionaryGetValue(dictRef, CFSTR(RIGHTS_TAG));
    if (rightsRef == NULL) {
        rightsRef = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (rightsRef == NULL) {
            result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
            goto cleanup;
        }
        releaseRightsRef = rightsRef;
        CFDictionarySetValue(dictRef, CFSTR(RIGHTS_TAG), rightsRef);
        bNeedsSaving = TRUE;
    }
       
    sysPrefsRef = (CFMutableDictionaryRef)CFDictionaryGetValue(rightsRef, CFSTR(SYSTEM_PREFERENCES_TAG));
    if (sysPrefsRef == NULL) {
        sysPrefsRef = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (sysPrefsRef == NULL) {
            result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
            goto cleanup;
        }
        releaseSysPrefsRef = sysPrefsRef;
        CFDictionarySetValue(rightsRef, CFSTR(SYSTEM_PREFERENCES_TAG), sysPrefsRef);
        bNeedsSaving = TRUE;
    }
    
    /* 
     * If the passed in value is true, we need to secure system preferences.
     * This means, we have to set shared to false.
	*/
    valRef = (CFBooleanRef)CFDictionaryGetValue(sysPrefsRef, CFSTR(SHARED_TAG));
    if (valRef != NULL) {
        Boolean val = CFBooleanGetValue(valRef);
        if (val != bShared) {
            CFDictionarySetValue(sysPrefsRef, CFSTR(SHARED_TAG), (bShared ? kCFBooleanTrue : kCFBooleanFalse));
            bNeedsSaving = TRUE;
        }
    } else {
        CFDictionarySetValue(sysPrefsRef, CFSTR(SHARED_TAG), (bShared ? kCFBooleanTrue : kCFBooleanFalse));
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList(pList, CFSTR(AUTHORIZATION_PLIST_FILE_PATH), kCFPropertyListXMLFormat_v1_0);
    }

cleanup:

    if (releaseSysPrefsRef) {
        CFRelease(releaseSysPrefsRef);
    }
       
    if (releaseRightsRef) {
        CFRelease(releaseRightsRef);
    }
    
    if (pList) {
        CFRelease(pList);
    }
    
    return result;
}

int
IsPasswordForEachSecureSystemPreferenceRequired(
    int* pbValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFMutableDictionaryRef dictRef = NULL;
    CFMutableDictionaryRef rightsRef = NULL;
    CFMutableDictionaryRef sysPrefsRef = NULL;
    CFBooleanRef valRef = NULL;

    result = GPReadPropertyListFile( CFSTR(AUTHORIZATION_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListXMLFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }
    
    dictRef = (CFMutableDictionaryRef)pList;
    rightsRef = (CFMutableDictionaryRef)CFDictionaryGetValue(dictRef, CFSTR(RIGHTS_TAG));
    if (rightsRef == NULL) {
        *pbValue = 0;
        goto cleanup;
    }
       
    sysPrefsRef = (CFMutableDictionaryRef)CFDictionaryGetValue(rightsRef, CFSTR(SYSTEM_PREFERENCES_TAG));
    if (sysPrefsRef == NULL) {
        *pbValue = 0;
        goto cleanup;
    }
    
    valRef = (CFBooleanRef)CFDictionaryGetValue(sysPrefsRef, CFSTR(SHARED_TAG));
    if (valRef != NULL) {
        Boolean val = CFBooleanGetValue(valRef);
        /* if shared is set to true, it means system preferences are not secured */
        *pbValue = (val ? 0 : 1);
    } else {
        *pbValue = 0;
    }

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
RemoveAutoLoginUserId()
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFStringRef userRef = NULL;
    CFNumberRef userIdRef = NULL;
    Boolean bNeedsSaving = FALSE;
    
    result = GPReadPropertyListFile( CFSTR(LOGINWINDOW_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListBinaryFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }
    
    userRef = (CFStringRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                 CFSTR(AUTOLOGIN_USER_TAG));
    if (userRef != NULL) {
        CFDictionaryRemoveValue( (CFMutableDictionaryRef)pList, 
                                 CFSTR(AUTOLOGIN_USER_TAG));
        bNeedsSaving = TRUE;
    }
    
    userIdRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                   CFSTR(AUTOLOGIN_USER_UID_TAG));
    if (userIdRef != NULL) {
        CFDictionaryRemoveValue( (CFMutableDictionaryRef)pList, 
                                 CFSTR(AUTOLOGIN_USER_UID_TAG));
        bNeedsSaving = TRUE;
    }
    
    if (bNeedsSaving) {
       result = GPSavePropertyList( pList, 
                                    CFSTR(LOGINWINDOW_PLIST_FILE_PATH), 
                                    kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;
    }

cleanup:
    
    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
DisableAutomaticLogin(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFBooleanRef valRef = NULL;
    Boolean bNeedsSaving = FALSE;
    
    result = GPReadPropertyListFile( CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListBinaryFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFBooleanRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                 CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG));
    if (valRef == NULL) {
        if (bValue) {   /* Disable auto login */
            result = RemoveAutoLoginUserId();
            if (result)
                goto cleanup;
                
            CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                  CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG),
                                  kCFBooleanTrue);
        } else {
            CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                  CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG),
                                  kCFBooleanFalse);
        }
        bNeedsSaving = TRUE;
    } else {
        Boolean val = CFBooleanGetValue(valRef);
        if (val != bValue) {
            if (!bValue) { /* Enable auto logins */
                CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG),
                                      kCFBooleanFalse);
            } else {
                result = RemoveAutoLoginUserId();
                if (result)
                    goto cleanup;
                
                /* Disable auto login */
                CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG),
                                      kCFBooleanTrue);
            }
            bNeedsSaving = TRUE;
        }
    }
    
    if (bNeedsSaving) {
        result = GPSavePropertyList( pList, 
                                     CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH), 
                                     kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;
          
        NotifyLoginWindow();
    }

cleanup:
    
    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
IsAutomaticLoginDisabled(
    int* pbValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFBooleanRef valRef = NULL;
    
    result = GPReadPropertyListFile( CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListBinaryFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFBooleanRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                 CFSTR(DISABLE_AUTO_LOGIN_KEY_TAG));
    if (valRef == NULL) {  
        /* Auto logins are enabled */
        *pbValue = 0;
    } else {
        Boolean val = CFBooleanGetValue(valRef);
        *pbValue = (val ? 1 : 0);
    }

cleanup:
    
    if (pList) {
        CFRelease(pList);
    }

    return result;
}

static
int
CreateDictionary(
    int bValue,
    CFStringRef strSettingKey,
    CFMutableDictionaryRef *pDictRef
    )
{
    int result = 0;

    *pDictRef = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    if (*pDictRef == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

    if ( bValue ) {
        CFDictionarySetValue( (CFMutableDictionaryRef)*pDictRef,
                              strSettingKey,
                              kCFBooleanTrue);
    } else {
        CFDictionarySetValue( (CFMutableDictionaryRef)*pDictRef,
                              strSettingKey,
                              kCFBooleanFalse);
    }

cleanup:

    return result;
}

int
EditUseSecureVirtualMemoryTiger(int bValue)
{
    int result = 0;
    FILE* fp_read = NULL;
    FILE* fp_write = NULL;
    Boolean bRemoveFile = FALSE;
    Boolean bNeedsSaving = FALSE;
    Boolean bFound = FALSE;
    char szBuf[256];
    char* pszTmpLine = NULL;
    
    if ((fp_read = fopen(HOST_CONFIG_FILE_PATH, "r")) == NULL) {
        result = GP_MAC_ITF_FAILED_OPEN_FILE;
        goto cleanup;
    }
    
    if ((fp_write = fopen(HOST_CONFIG_FILE_PATH ".lwidentity.tmp", "w")) == NULL) {
        result = GP_MAC_ITF_FAILED_OPEN_FILE;
        goto cleanup;
    }
    
    bRemoveFile = TRUE;
    
    while (1) {
        if (fgets(szBuf, 255, fp_read) == NULL) {
            if (feof(fp_read)) {
                break;
            } else {
                result = errno;
                goto cleanup;
             }
          }
          pszTmpLine = StripWhiteSpaceFromFront(szBuf);
          if (!pszTmpLine)
            continue;
             
          if (!strncmp(pszTmpLine, ENCRYPTSWAP_TAG, sizeof(ENCRYPTSWAP_TAG)-1)) {
             bFound = TRUE;
             if (bValue) {
                /* Use secure virtual memory */
                if (!strstr(pszTmpLine, "-YES-")) {
                   fprintf(fp_write, "%s=-YES-\n", ENCRYPTSWAP_TAG);
                   bNeedsSaving = TRUE;
                }
             } else {
                bNeedsSaving = TRUE;
                /* Don't use secure virtual memory */
                /* Don't write the value */
                continue;
             }
        } else {
            if (fputs(szBuf, fp_write) == EOF) {
                result = errno;
                goto cleanup;
            }
        }
    }
    
    if (!bFound && bValue) {
        fprintf(fp_write, "%s=-YES-\n", ENCRYPTSWAP_TAG);
        bNeedsSaving = TRUE;
    }
    
    if (bNeedsSaving) {
       fclose(fp_write);
       fp_write = NULL;
       
       if (rename(HOST_CONFIG_FILE_PATH ".lwidentity.tmp", HOST_CONFIG_FILE_PATH) < 0) {
          result = errno;
          goto cleanup;
       }
       
       bRemoveFile = FALSE;
    }
    
cleanup:

    if (fp_read) {
       fclose(fp_read);
    }
    
    if (fp_write) {
       fclose(fp_write);
    }

    if (bRemoveFile) {
       unlink(HOST_CONFIG_FILE_PATH ".lwidentity.tmp");
    }
    
    return result;
}

static
int
EditUseSecureVirtualMemoryLeopard(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFNumberRef valRef = NULL;
    Boolean bNeedsSaving = FALSE;
    Boolean bFileExists = FALSE;

    CheckFileExists( VIRTUAL_MEMORY_FILE_PATH_LEOPARD,
                     &bFileExists);

    if(!bFileExists) {
        CFMutableDictionaryRef dictRef = NULL;
        result = CreateDictionary( bValue,
                                   CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD),
                                   &dictRef);
        if (result)
            goto cleanup;

        pList = dictRef;
        bNeedsSaving = TRUE;
    } else {
        result = GPReadPropertyListFile( CFSTR(VIRTUAL_MEMORY_FILE_PATH_LEOPARD),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;

        if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
            result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
            goto cleanup;
        }

        valRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                    CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD));

        if (valRef != NULL) {
            if ( bValue ) {
                CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD),
                                      kCFBooleanTrue);
            } else {
                CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD),
                                      kCFBooleanFalse);
            }
            bNeedsSaving = TRUE;
        } else {
            if ( bValue ) {
                CFDictionaryAddValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD),
                                      kCFBooleanTrue);
            } else {
                CFDictionaryAddValue( (CFMutableDictionaryRef)pList,
                                      CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD),
                                      kCFBooleanFalse);
            }
            bNeedsSaving = TRUE;
        }
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList,
                                     CFSTR(VIRTUAL_MEMORY_FILE_PATH_LEOPARD),
                                     kCFPropertyListBinaryFormat_v1_0);
    }

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
UseSecureVirtualMemory(int bValue)
{
    int bIsLeopard = 0;

    IsLeopardSystem(&bIsLeopard);
    if (bIsLeopard) {
        return EditUseSecureVirtualMemoryLeopard(bValue);
    } else {
        return EditUseSecureVirtualMemoryTiger(bValue);
    }
}

int
IsSecureVirtualMemoryUsedTiger(int *pbValue)
{
    int result = 0;
    FILE* fp_read = NULL;
    Boolean bFound = FALSE;
    char szBuf[256];
    char* pszTmpLine = NULL;
    
    if ((fp_read = fopen(HOST_CONFIG_FILE_PATH, "r")) == NULL) {
        result = GP_MAC_ITF_FAILED_OPEN_FILE;
        goto cleanup;
    }
    
    while (1) {
        if (fgets(szBuf, 255, fp_read) == NULL) {
            if (feof(fp_read)) {
                break;
            } else {
                result = errno;
                goto cleanup;
            }
        }
        pszTmpLine = StripWhiteSpaceFromFront(szBuf);
        if (!pszTmpLine)
            continue;
             
        if (!strncmp(pszTmpLine, "ENCRYPTSWAP", sizeof("ENCRYPTSWAP")-1)) {
            bFound = TRUE;
            /* Use secure virtual memory */
            if (!strstr(pszTmpLine, "-YES-")) {
                *pbValue = 1;
            } else {
                *pbValue = 0;
            }
            goto cleanup;
        }
    }
    
cleanup:

    if (fp_read) {
        fclose(fp_read);
    }

    return result;
}

int
IsSecureVirtualMemoryUsedLeopard(int *pbValue)
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFBooleanRef valRef = NULL;
    Boolean bFileExists = FALSE;

    CheckFileExists( VIRTUAL_MEMORY_FILE_PATH_LEOPARD,
                     &bFileExists);
    if (!bFileExists) {
        goto cleanup;
    }
    
    result = GPReadPropertyListFile( CFSTR(VIRTUAL_MEMORY_FILE_PATH_LEOPARD),
                                     &pList,
                                     kCFPropertyListBinaryFormat_v1_0);
    if (result)
        goto cleanup;
       
    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFBooleanRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                 CFSTR(SECURE_VIRTUAL_MEMORY_TAG_LEOPARD));
    if (valRef == NULL) {  
        *pbValue = 0;
    } else {
        Boolean val = CFBooleanGetValue(valRef);
        *pbValue = (val ? 1 : 0);
    }

cleanup:
    
    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
IsSecureVirtualMemoryUsed(int *pbValue)
{
    int result = 0;

    IsLeopardSystem(&result);
    if (result) {
        result = IsSecureVirtualMemoryUsedLeopard(pbValue);
    } else {
        result = IsSecureVirtualMemoryUsedTiger(pbValue);
    }

    return result;
}

int
LogoutInMinutesOfInactivity(
    int nMinutes
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    Boolean bNeedsSaving = FALSE;
    CFNumberRef minRef = NULL;
    CFNumberRef minValRef = NULL;
    
    result = GPReadPropertyListFile( CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH), 
                                     &pList, 
                                     kCFPropertyListOpenStepFormat);
    if (result)
       goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    minRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList, 
                                                CFSTR(MINUTES_OF_INACTIVITY_TAG));
    minValRef = CFNumberCreate( NULL, 
                                kCFNumberSInt32Type, 
                                &nMinutes);
	
    if( minRef != NULL ) {
        CFDictionarySetValue( (CFMutableDictionaryRef)pList, 
                              CFSTR(MINUTES_OF_INACTIVITY_TAG), 
                              minValRef);
        bNeedsSaving = TRUE;
    } else {
        CFDictionaryAddValue( (CFMutableDictionaryRef)pList, 
                              CFSTR(MINUTES_OF_INACTIVITY_TAG), 
                              minValRef);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList, 
                                     CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH), 
                                     kCFPropertyListXMLFormat_v1_0);
        if (result)
            goto cleanup;
    }

cleanup:
    
    if (pList) {
       CFRelease(pList);
    }
   
    return result;
}

int
GetMinutesOfInactivityToLogout(
    int *pNMinutes
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFNumberRef valRef = NULL;

    result = GPReadPropertyListFile( CFSTR(GLOBAL_PREFERENCES_PLIST_FILE_PATH), 
                                     &pList,
                                     kCFPropertyListBinaryFormat_v1_0);
    if (result)
       goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
       result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
       goto cleanup;
    }

    valRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList, 
                                                CFSTR(MINUTES_OF_INACTIVITY_TAG));
    if (valRef == NULL) {
       *pNMinutes = 0;
    } else {
       SInt8 val = 0;
       CFNumberGetValue( valRef, 
                         kCFNumberSInt8Type, 
                         &val);
       *pNMinutes = val;
    }

cleanup:

    if (pList) {
       CFRelease(pList);
    }

    return result;
}

