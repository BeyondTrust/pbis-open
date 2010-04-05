#include "lwimaciface_firewall.h"

int
GetFirewallPListNumberValueAtTopLevel(
    CFStringRef keyName,
    int* pbValue
    )
{
    int result = 0;
    int bIsLeopard = 0;
    CFPropertyListRef pList = NULL;
    CFNumberRef valRef = NULL;

    IsLeopardSystem(&bIsLeopard);
    if (bIsLeopard) {
        result = GPReadPropertyListFile( CFSTR (FIREWALL_PLIST_FILE_PATH_LEOPARD),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
    } else {
        result = GPReadPropertyListFile( CFSTR (FIREWALL_PLIST_FILE_PATH),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
    }

    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                keyName);
    if (valRef == NULL) {
        pbValue = 0;
    } else {
        SInt8 val = 0;
        CFNumberGetValue( valRef,
                          kCFNumberSInt8Type,
                          &val);
        *pbValue = val;
    }

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
SetFirewallPListNumberValueAtTopLevel(
    CFStringRef keyName,
    int bValue
    )
{
    int result = 0;
    int bIsLeopard = 0;
    CFPropertyListRef pList = NULL;
    CFNumberRef valRef = NULL;
    CFNumberRef releaseValRef = NULL;
    Boolean bNeedsSaving = FALSE;
    SInt8 newValue = (bValue ? 1 : 0);

    IsLeopardSystem(&bIsLeopard);
    if(bIsLeopard) {
        result = GPReadPropertyListFile( CFSTR (FIREWALL_PLIST_FILE_PATH_LEOPARD),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
    } else {
        result = GPReadPropertyListFile( CFSTR (FIREWALL_PLIST_FILE_PATH),
                                         &pList,
                                         kCFPropertyListBinaryFormat_v1_0);
    }

    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFNumberRef)CFDictionaryGetValue((CFMutableDictionaryRef)pList, keyName);
    if (!newValue) {
        /* Do not block UDP Traffic */
        if (valRef != NULL) {
            CFDictionaryRemoveValue((CFMutableDictionaryRef)pList, keyName);
            bNeedsSaving = TRUE;
        }
    } else {
        Boolean bSetValue = FALSE;

        /* Block UDP Traffic */
        if (valRef == NULL)
            bSetValue = TRUE;
        else {
            SInt8 val = 0;
            CFNumberGetValue(valRef, kCFNumberSInt8Type, &val);
            if (val != newValue) {
                bSetValue = TRUE;
            }
        }

        if (bSetValue) {
            valRef = CFNumberCreate(NULL, kCFNumberSInt8Type, &newValue);
            releaseValRef = valRef;
            /* Disable Auto Login */
            CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                                  keyName,
                                  valRef);
            bNeedsSaving = TRUE;
        }
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        if(bIsLeopard) {
            result = GPSavePropertyList(pList, CFSTR(FIREWALL_PLIST_FILE_PATH_LEOPARD), kCFPropertyListBinaryFormat_v1_0);
        } else {
            result = GPSavePropertyList(pList, CFSTR (FIREWALL_PLIST_FILE_PATH), kCFPropertyListBinaryFormat_v1_0);
        }
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
EditTigerFirewallState(
    CFStringRef keyName,
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    Boolean bNeedsSaving = FALSE;

    result = GPReadPropertyListFile( CFSTR (FIREWALL_PLIST_FILE_PATH),
                                     &pList,kCFPropertyListBinaryFormat_v1_0);

    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    if ( bValue ) {
        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              keyName,
                              kCFBooleanTrue);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList,
                                     CFSTR(FIREWALL_PLIST_FILE_PATH),
                                     kCFPropertyListBinaryFormat_v1_0);
    }

    if ( bValue ) {
        result = system("/usr/libexec/FirewallTool >/dev/null 2>&1");
    } else {
        result = system("/sbin/ipfw -f flush >/dev/null 2>&1");
    }

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

int
UpdateLeopardFirewallState(
    SInt32 stateVal,
    CFStringRef keyName,
    CFPropertyListRef pList
    )
{
    int result = 0;

    CFNumberRef stateRef = NULL;

    stateRef = CFNumberCreate( NULL,
                               kCFNumberSInt32Type,
                               &stateVal);

    CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                          keyName,
                          stateRef);

    return result;
}

int
AddLeopardFirewallState(
    SInt32 stateVal,
    CFStringRef keyName,
    CFPropertyListRef pList
    )
{
    int result = 0;

    CFNumberRef stateRef = NULL;

    stateRef = CFNumberCreate( NULL,
                               kCFNumberSInt32Type,
                               &stateVal);

    CFDictionaryAddValue( (CFMutableDictionaryRef)pList,
                           keyName,
                           stateRef);

    return result;
}

int
EditLeopardFirewallState(
    CFStringRef keyName,
    int bValue
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFNumberRef valRef = NULL;
    Boolean bNeedsSaving = FALSE;

    result = GPReadPropertyListFile( CFSTR(FIREWALL_PLIST_FILE_PATH_LEOPARD),
                                     &pList,kCFPropertyListBinaryFormat_v1_0);

    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                keyName);

    if (valRef != NULL) {
        result = UpdateLeopardFirewallState( bValue,
                                             keyName,
                                             pList);
       bNeedsSaving = TRUE;
    } else {
        result = AddLeopardFirewallState( bValue,
                                          keyName,
                                          pList);
        bNeedsSaving = TRUE;
    }

    if (bNeedsSaving) {
        result = GPSavePropertyList( pList,
                                     CFSTR(FIREWALL_PLIST_FILE_PATH_LEOPARD),
                                     kCFPropertyListBinaryFormat_v1_0);
    }

cleanup:

    if (pList) {
       CFRelease(pList);
    }

    return result;
}

int
EditFirewallState(
    int bValue
    )
{
    int result = 0;

    IsLeopardSystem(&result);
    if (result) {
        result = EditLeopardFirewallState( CFSTR(LEOPARD_FIREWALL_STATE_TAG),
                                           bValue);
    } else {
        result = EditTigerFirewallState( CFSTR(TIGER_FIREWALL_STATE_TAG),
                                         bValue);
    }

    return result;
}

int
IsFirewallEnabled(int* pbValue)
{
    int bIsLeopard = 0;

    IsLeopardSystem(&bIsLeopard);
    if (bIsLeopard) {
        return GetFirewallPListNumberValueAtTopLevel( CFSTR(LEOPARD_FIREWALL_STATE_TAG),
                                                      pbValue);
    } else {
        return GetFirewallPListNumberValueAtTopLevel( CFSTR(TIGER_FIREWALL_STATE_TAG),
                                                      pbValue);
    }
}

int
EnableFirewallLogging(int bValue)
{
    return SetFirewallPListNumberValueAtTopLevel(CFSTR(FIREWALL_LOGGING_ENABLED_TAG), bValue);
}

int
IsFirewallLoggingEnabled(int* pbValue)
{
    return GetFirewallPListNumberValueAtTopLevel(CFSTR(FIREWALL_LOGGING_ENABLED_TAG), pbValue);
}

int
BlockUDPTraffic(int bValue)
{
    return SetFirewallPListNumberValueAtTopLevel(CFSTR(UDPENABLED_TAG), bValue);
}

int
IsUDPTrafficBlocked(int* pbValue)
{
    return GetFirewallPListNumberValueAtTopLevel(CFSTR(UDPENABLED_TAG), pbValue);
}

int
EnableStealthMode(int bValue)
{
    return SetFirewallPListNumberValueAtTopLevel(CFSTR(STEALTHENABLED_TAG), bValue);
}

int
IsStealthModeEnabled(int* pbValue)
{
    return GetFirewallPListNumberValueAtTopLevel(CFSTR(STEALTHENABLED_TAG), pbValue);
}

