#include "lwimaciface_energy.h"
#include "lwimacifaceutils.h"
#include "gpplistutils.h"

static
int
CreateEnergySaverDictionary(
    CFNumberRef valSettingRef,
    CFStringRef strSettingKey,
    CFMutableDictionaryRef *pDictRef
    )
{
    int result = 0;
    SInt32 defValue = -1;

    CFMutableDictionaryRef dictRef0 = NULL;
    CFMutableDictionaryRef dictRef1 = NULL;
    CFMutableDictionaryRef dictRef2 = NULL;

    CFNumberRef valRef = NULL;

    *pDictRef = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (*pDictRef == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

    dictRef0 = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (dictRef0 == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

    dictRef1 = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (dictRef1 == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

    dictRef2 = CFDictionaryCreateMutable(NULL, 0, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (dictRef2 == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

    valRef = CFNumberCreate( NULL,
                             kCFNumberSInt32Type,
                             &defValue);
    if (valRef != NULL) {
        CFDictionarySetValue( (CFMutableDictionaryRef)dictRef0,
                              CFSTR("AC Power"),
                              valRef);
    }

    CFDictionarySetValue( (CFMutableDictionaryRef)*pDictRef,
                          CFSTR("ActivePowerProfiles"),
                          dictRef0);

    if( valSettingRef != NULL ) {
        CFDictionarySetValue( (CFMutableDictionaryRef)dictRef2,
                              strSettingKey,
                              valSettingRef);
    }

    CFDictionarySetValue( (CFMutableDictionaryRef)dictRef1,
                           CFSTR("AC Power"),
                           dictRef2);

    CFDictionarySetValue( (CFMutableDictionaryRef)*pDictRef,
                          CFSTR("Custom Profile"),
                          dictRef1);

cleanup:

    return result;
}

static
int
ApplySetting(
    int nValue,
    CFStringRef strSettingKeyRef
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFPropertyListRef pListLev0 = NULL;
    Boolean bNeedsSaving = FALSE;
    CFNumberRef valRef = NULL;
    CFNumberRef releaseValRef = NULL;
    Boolean bFileExists = FALSE;

    CheckFileExists( ENERGY_SAVER_FILE_PATH,
                     &bFileExists);

    valRef = CFNumberCreate( NULL,
                             kCFNumberSInt32Type,
                             &nValue);
    releaseValRef = valRef;

    if(!bFileExists) {
        CFMutableDictionaryRef dictRef = NULL;
        result = CreateEnergySaverDictionary( valRef,
                                              strSettingKeyRef,
                                              &dictRef);
        if (result)
            goto cleanup;

        pList = dictRef;
        bNeedsSaving = TRUE;
    } else {
        result = GPReadPropertyListFile( CFSTR(ENERGY_SAVER_FILE_PATH),
                                         &pList,
                                         kCFPropertyListOpenStepFormat);
        if (result)
            goto cleanup;

        if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
            result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
            goto cleanup;
        }

        pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                             CFSTR("Custom Profile"));
        ASSERT_QUERY(pListLev0);

        pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                             CFSTR("AC Power"));
        ASSERT_QUERY(pListLev0);

        if( valRef != NULL ) {
            CFDictionarySetValue( (CFMutableDictionaryRef)pListLev0,
                                  strSettingKeyRef,
                                  valRef);
            bNeedsSaving = TRUE;
        }
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList,
                                     CFSTR(ENERGY_SAVER_FILE_PATH),
                                     kCFPropertyListXMLFormat_v1_0);
        if (result)
            goto cleanup;
    }

cleanup:

    if (releaseValRef) {
        CFRelease(releaseValRef);
    }

    if (pList) {
       CFRelease(pList);
    }

    return result;
}

int
ApplyEnergySaverSettings(
    int bValue,
    char *pszSettingName
    )
{
    int result = 0;

    CFStringRef strSettingNameRef = CFStringCreateWithCString( NULL,
                                                               (const char *)pszSettingName,
                                                               kCFStringEncodingUTF8);

    if (CFStringCompare(strSettingNameRef, CFSTR(ENERGY_SAVER_WAKE_ON_MODEM_RING), kCFCompareCaseInsensitive) == 0) {
        CHECK_SETTING_SUPPORT;
    }

    result = ApplySetting( bValue, 
                           strSettingNameRef);
    if (result)
        goto cleanup;

cleanup:

    return result;
}

