#include "lwimaciface_bluetooth.h"
#include "lwimacifaceutils.h"
#include "gpplistutils.h"

static
int
CreateBluetoothDictionary(
    CFMutableDictionaryRef *pDictRef
    )
{
    int result = 0;

    *pDictRef = CFDictionaryCreateMutable( NULL, 
                                           0, 
                                           &kCFCopyStringDictionaryKeyCallBacks, 
                                           &kCFTypeDictionaryValueCallBacks);
    if (*pDictRef == NULL) {
        result = GP_MAC_ITF_FAILED_CREATE_DICTIONARY;
        goto cleanup;
    }

cleanup:

    return result;
}

int
EnableBluetoothControllerState(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    Boolean bNeedsSaving = FALSE;
    Boolean bFileExists = FALSE;

    CHECK_SETTING_SUPPORT;

    CheckFileExists( BLUETOOTH_PLIST_FILE_PATH,
                     &bFileExists);

    if(!bFileExists) {
        CFMutableDictionaryRef dictRef = NULL;
        result = CreateBluetoothDictionary(&dictRef);
        if (result)
            goto cleanup;

        pList = dictRef;
        bNeedsSaving = TRUE;
    } else {
        result = GPReadPropertyListFile( CFSTR(BLUETOOTH_PLIST_FILE_PATH),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;

        if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
            result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
            goto cleanup;
        }
    }

    if (bValue) { /* Reset the bluetooth controller power state */
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(BLUETOOTH_ENABLED_TAG),
                              kCFBooleanTrue);
        bNeedsSaving = TRUE;
    } else { /* Set the bluetooth controller power state */
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(BLUETOOTH_ENABLED_TAG),
                              kCFBooleanFalse);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList, 
                                     CFSTR(BLUETOOTH_PLIST_FILE_PATH), 
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
ApplyBluetoothSetupAssistantSetting(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    Boolean bNeedsSaving = FALSE;
    Boolean bFileExists = FALSE;

    CHECK_SETTING_SUPPORT;

    CheckFileExists( BLUETOOTH_PLIST_FILE_PATH,
                     &bFileExists);

    if(!bFileExists) {
        CFMutableDictionaryRef dictRef = NULL;

        result = CreateBluetoothDictionary(&dictRef);
        if (result)
            goto cleanup;

        pList = dictRef;
        bNeedsSaving = TRUE;
    } else {
        result = GPReadPropertyListFile( CFSTR(BLUETOOTH_PLIST_FILE_PATH),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;

        if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
            result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
            goto cleanup;
        }
    }

    if (bValue) {
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(BLUETOOTH_SETUP_ASSISTANT_KEY),
                              kCFBooleanTrue);
        bNeedsSaving = TRUE;
    } else {
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(BLUETOOTH_SETUP_ASSISTANT_KEY),
                              kCFBooleanFalse);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList, 
                                     CFSTR(BLUETOOTH_PLIST_FILE_PATH), 
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
ApplyBluetoothShareInternetConnectionSetting(
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    Boolean bNeedsSaving = FALSE;
    CFNumberRef valRef = NULL;
    CFNumberRef releaseValRef = NULL;
    Boolean bFileExists = FALSE;

    CHECK_SETTING_SUPPORT;

    CheckFileExists( BLUETOOTH_PLIST_FILE_PATH,
                     &bFileExists);

    if(!bFileExists) {
        CFMutableDictionaryRef dictRef = NULL;
        result = CreateBluetoothDictionary(&dictRef);
        if (result)
            goto cleanup;

        pList = dictRef;
        bNeedsSaving = TRUE;
    } else {
        result = GPReadPropertyListFile( CFSTR(BLUETOOTH_PLIST_FILE_PATH),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
        if (result)
            goto cleanup;

        if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
            result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
            goto cleanup;
        }
    }

    valRef = CFNumberCreate( NULL,
                             kCFNumberSInt32Type,
                             &bValue);
    releaseValRef = valRef;

    if( valRef != NULL ) {
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(BLUETOOTH_SHARE_INTERNET_CONNECTION_KEY),
                              valRef);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList, 
                                     CFSTR(BLUETOOTH_PLIST_FILE_PATH), 
                                     kCFPropertyListBinaryFormat_v1_0);
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

