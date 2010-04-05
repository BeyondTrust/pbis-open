#include "gpplistutils.h"
#include "lwimacifaceutils.h"
#include "lwimaciface_network.h"
#include <unistd.h>

static
int
GetInterfaceList(
    CFPropertyListRef pList,
    CFArrayRef *serviceOrderRef
    )
{
    int result = 0;
    CFStringRef strRef = NULL;
    CFStringRef subStrRef = NULL;
    CFPropertyListRef pListLev0 = NULL;

    strRef = (CFStringRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                CFSTR("CurrentSet"));
    ASSERT_QUERY(strRef);

    subStrRef = CFStringCreateWithSubstring( NULL,
                                             strRef,
                                             CFRangeMake(6, CFStringGetLength(strRef) - 6));
    ASSERT_QUERY(subStrRef);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                         CFSTR("Sets"));
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         subStrRef);
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("Network"));
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("Global"));
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("IPv4"));
    ASSERT_QUERY(pListLev0);

    *serviceOrderRef = (CFArrayRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("ServiceOrder"));
    ASSERT_QUERY(*serviceOrderRef);

cleanup:

    return result;
}

static
int
UpdateAppleTalkManualSettings(
    int nNodeId,
    int nNetId,
    Boolean bSetValue,
    CFPropertyListRef pList
    )
{
    int result = 0;

    CFNumberRef netIdRef = NULL;
    CFNumberRef nodeIdRef = NULL;
    CFNumberRef releaseNetIdRef = NULL;
    CFNumberRef releaseNodeIdRef = NULL;

    netIdRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                  CFSTR(NETID_TAG));
    nodeIdRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                   CFSTR(NODEID_TAG));

    if( netIdRef == NULL || nodeIdRef == NULL ) {
        bSetValue = TRUE;
    } else {
        SInt32 netIdVal = 0;
        SInt32 nodeIdVal = 0;

        CFNumberGetValue( netIdRef,
                          kCFNumberSInt32Type,
                          &netIdVal);
        CFNumberGetValue( nodeIdRef,
                          kCFNumberSInt32Type,
                          &nodeIdVal);

        if( netIdVal != nNetId)
            bSetValue = TRUE;
        else if(nodeIdVal != nNodeId)
            bSetValue = TRUE;
    }

    if(bSetValue) {
        netIdRef = CFNumberCreate( NULL,
                                   kCFNumberSInt32Type,
                                   &nNetId);
        releaseNetIdRef = netIdRef;

        nodeIdRef = CFNumberCreate( NULL,
                                    kCFNumberSInt32Type,
                                    &nNodeId);
        releaseNodeIdRef = nodeIdRef;

        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(NETID_TAG),
                              netIdRef);

        CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                              CFSTR(NODEID_TAG),
                              nodeIdRef);
    }

    if (releaseNetIdRef) {
        CFRelease(releaseNetIdRef);
    }

    if (releaseNodeIdRef) {
        CFRelease(releaseNodeIdRef);
    }

    return result;
}

static
int
UpdateAppleTalkAutomaticSetting(
    CFPropertyListRef pList
    )
{
    int result = 0;

    CFNumberRef netIdRef = NULL;
    CFNumberRef nodeIdRef = NULL;

    netIdRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                  CFSTR(NETID_TAG));
    nodeIdRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                   CFSTR(NODEID_TAG));

    if (netIdRef != NULL) {
        CFDictionaryRemoveValue( (CFMutableDictionaryRef)pList,
                                 CFSTR(NETID_TAG));
    }

    if (nodeIdRef != NULL) {
        CFDictionaryRemoveValue( (CFMutableDictionaryRef)pList,
                                 CFSTR(NODEID_TAG));
    }

    return result;
}

static
int
UpdateAppleTalkInActiveSetting(
    CFPropertyListRef pList
    )
{
    int result = 0;
    int val = 1;

    CFNumberRef numRef = NULL;

    numRef = CFNumberCreate( NULL,
                             kCFNumberSInt32Type,
                             &val);

    CFDictionarySetValue( (CFMutableDictionaryRef)pList,
                          CFSTR("__INACTIVE__"),
                          numRef);

    if (numRef) {
        CFRelease(numRef);
    }

    return result;
}

static
int
EditAppleTalkSettings(
    CFPropertyListRef pList,
    int nMode,
    int nNodeId,
    int nNetId
    )
{
    int result = 0;
    CFNumberRef inactiveRef = NULL;
    Boolean bSetValue = FALSE;
    inactiveRef = (CFNumberRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                     CFSTR("__INACTIVE__"));

    if( inactiveRef ) {
        CFDictionaryRemoveValue( (CFMutableDictionaryRef)pList,
                                 CFSTR("__INACTIVE__"));
        bSetValue = TRUE;
    }

    switch(nMode) {
        case 1:
            result = UpdateAppleTalkManualSettings( nNodeId,
                                                    nNetId,
                                                    bSetValue,
                                                    pList);
            break;

        case 2:
            result = UpdateAppleTalkAutomaticSetting(pList);
            break;

        case 3:
            result = UpdateAppleTalkInActiveSetting(pList);
            break;
    }

    return result;
}

static
int
ProcessAppleTalkSettingUpdate(
    CFStringRef guidStrRef,
    CFPropertyListRef pList,
    int nMode,
    int nNodeId,
    int nNetId
    )
{
    int result = 0;
    CFPropertyListRef pListLev0 = NULL;
    CFPropertyListRef pListLev1 = NULL;
    CFStringRef strRef = NULL;

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                         CFSTR("NetworkServices"));
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         guidStrRef);
    ASSERT_QUERY(pListLev0);

    pListLev1 = pListLev0;

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("Interface"));
    ASSERT_QUERY(pListLev0);

    strRef = (CFStringRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                CFSTR("Hardware"));
    ASSERT_QUERY(strRef);

    if (CFStringCompare(strRef, CFSTR("Ethernet"), kCFCompareCaseInsensitive) == 0 ||
        CFStringCompare(strRef, CFSTR("Airport"), kCFCompareCaseInsensitive) == 0 ) {
        pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev1,
                                                             CFSTR("AppleTalk"));
        ASSERT_QUERY(pListLev0);

        result = EditAppleTalkSettings( pListLev0,
                                        nMode,
                                        nNodeId,
                                        nNetId);
    }

cleanup:

    return result;
}

int
ApplyAppleTalkSettings(
    int nMode,
    int nNodeId,
    int nNetId
    )
{
    int result = 0;
    int i = 0;

    CFStringRef guidStrRef;
    CFPropertyListRef pList = NULL;
    CFArrayRef serviceOrderRef = NULL;

    result = GPReadPropertyListFile( CFSTR(SYSCONF_PREFERENCES_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListOpenStepFormat);
    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    if (GetInterfaceList( pList,
                          &serviceOrderRef)) {
        result = GP_MAC_ITF_FAILED_GET_PLIST_ENTRY;
        goto cleanup;
    }

    for (i = 0; i < CFArrayGetCount(serviceOrderRef); i++) {
        guidStrRef = CFArrayGetValueAtIndex(serviceOrderRef, i);
        if (ProcessAppleTalkSettingUpdate( guidStrRef,
                                           pList,
                                           nMode,
                                           nNodeId,
                                           nNetId)) {
            result = GP_MAC_ITF_FAILED_SETTING_UPDATE;
            goto cleanup;
        }
    }

    result = GPSavePropertyList( pList,
                                 CFSTR(SYSCONF_PREFERENCES_PLIST_FILE_PATH),
                                 kCFPropertyListXMLFormat_v1_0);
    if (result)
        goto cleanup;

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

static
int
FormatSettings(
    char *pszServerAddresses,
    char *pszSearchDomains,
    CFArrayRef *pServerAddrStrRef,
    CFArrayRef *pSearchDomainsStrRef
    )
{
    int result = 0;
    CFStringRef serverAddrRef = NULL;
    CFStringRef searchDomainsRef = NULL;

    if( pszServerAddresses ) {
        serverAddrRef = CFStringCreateWithCString( NULL,
                                                   (const char *)pszServerAddresses,
                                                   kCFStringEncodingUTF8);
        if (serverAddrRef) {
            *pServerAddrStrRef = (CFArrayRef)CFStringCreateArrayBySeparatingStrings( kCFAllocatorDefault,
                                                                                     serverAddrRef,
                                                                                     CFSTR(",") );
        }
    }

    if( pszSearchDomains ) {

        searchDomainsRef = CFStringCreateWithCString( NULL,
                                                      (const char *)pszSearchDomains,
                                                      kCFStringEncodingUTF8);
        if (searchDomainsRef) {
            *pSearchDomainsStrRef = (CFArrayRef)CFStringCreateArrayBySeparatingStrings( kCFAllocatorDefault,
                                                                                        searchDomainsRef,
                                                                                        CFSTR(",") );
        }
    }

    return result;
}

static
int
ProcessDNSSettingUpdate(
    CFStringRef guidStrRef,
    CFPropertyListRef pList,
    CFArrayRef serverAddrStrRef,
    CFArrayRef searchDomainsStrRef
    )
{
    int result = 0;
    CFPropertyListRef pListLev0 = NULL;
    CFPropertyListRef pListLev1 = NULL;
    CFStringRef strRef = NULL;

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                         CFSTR("NetworkServices"));
    ASSERT_QUERY(pListLev0);

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         guidStrRef);
    ASSERT_QUERY(pListLev0);

    pListLev1 = pListLev0;

    pListLev0 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                         CFSTR("Interface"));
    ASSERT_QUERY(pListLev0);

    strRef = (CFStringRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev0,
                                                CFSTR("Hardware"));
    ASSERT_QUERY(strRef);

    if (CFStringCompare(strRef, CFSTR("Ethernet"), kCFCompareCaseInsensitive) == 0 ||
        CFStringCompare(strRef, CFSTR("Airport"), kCFCompareCaseInsensitive) == 0 ) {
        pListLev1 = (CFPropertyListRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pListLev1,
                                                             CFSTR("DNS"));
        ASSERT_QUERY(pListLev1);

        if (serverAddrStrRef) {
            CFDictionarySetValue( (CFMutableDictionaryRef)pListLev1,
                                  CFSTR(DNS_SERVERS_TAG),
                                  serverAddrStrRef);
        }
        if (searchDomainsStrRef) {
            CFDictionarySetValue( (CFMutableDictionaryRef)pListLev1,
                                  CFSTR(DNS_SEARCH_DOMAINS_TAG),
                                  searchDomainsStrRef);
        }
    }

cleanup:

    return result;
}

int
ApplyDNSSettings(
    char *pszServerAddresses,
    char *pszSearchDomains
    )
{
    int result = 0;
    int i = 0;
    CFStringRef guidStrRef;

    CFPropertyListRef pList = NULL;

    CFArrayRef serverAddrStrRef = NULL;
    CFArrayRef searchDomainsStrRef = NULL;
    CFArrayRef serviceOrderRef = NULL;

    result = GPReadPropertyListFile( CFSTR(SYSCONF_PREFERENCES_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListOpenStepFormat);
    if (result)
       goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    if (GetInterfaceList( pList,
                          &serviceOrderRef)) {
        result = GP_MAC_ITF_FAILED_GET_PLIST_ENTRY;
        goto cleanup;
    }

    FormatSettings( pszServerAddresses,
                    pszSearchDomains,
                    &serverAddrStrRef,
                    &searchDomainsStrRef);

    for (i = 0; i < CFArrayGetCount(serviceOrderRef); i++) {
        guidStrRef = CFArrayGetValueAtIndex(serviceOrderRef, i);
        if (ProcessDNSSettingUpdate( guidStrRef,
                                     pList,
                                     serverAddrStrRef,
                                     searchDomainsStrRef)) {
            result = GP_MAC_ITF_FAILED_SETTING_UPDATE;
            goto cleanup;
        }
    }

    result = GPSavePropertyList( pList,
                                 CFSTR(SYSCONF_PREFERENCES_PLIST_FILE_PATH),
                                 kCFPropertyListXMLFormat_v1_0);
    if (result)
        goto cleanup;

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

