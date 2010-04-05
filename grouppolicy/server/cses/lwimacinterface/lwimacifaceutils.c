#include "lwimacifaceutils.h"
#include "gpplistutils.h"
#include "lwimaciface_bluetooth.h"
#include "lwimaciface_energy.h"
#include "lwimaciface_firewall.h"
#include "lwimaciface_network.h"
#include "lwimaciface_security.h"
#include <unistd.h>

char*
StripWhiteSpaceFromFront(
    char* pszLine
    )
{
    char* pszTmp = pszLine;
    if (pszLine == NULL)
        return NULL;

    while (*pszTmp != '\0' && isspace((int)*pszTmp))
        pszTmp++;

    return (pszTmp && *pszTmp ? pszTmp : NULL);
}

void
NotifyLoginWindow()
{
    CFMessagePortRef port = CFMessagePortCreateRemote( NULL,
                                                       CFSTR("com.apple.loginwindow.notify"));
    if (port) {
        CFMessagePortSendRequest(port, 500, 0, 0, 0, 0, 0);
        CFRelease(port);
    }
}

void
NotifySecurityDaemon()
{

}

int
IsLeopardSystem(
    int *pbIsLeopard
    )
{
    int result = 0;
    CFPropertyListRef pList = NULL;
    CFStringRef valRef = NULL;

    result = GPReadPropertyListFile( CFSTR(SYSTEM_VERSION_PLIST_FILE_PATH),
                                     &pList,
                                     kCFPropertyListXMLFormat_v1_0);
    if (result)
        goto cleanup;

    if (CFDictionaryGetTypeID() != CFGetTypeID(pList)) {
        result = GP_MAC_ITF_FAILED_WRONG_FORMAT;
        goto cleanup;
    }

    valRef = (CFStringRef)CFDictionaryGetValue( (CFMutableDictionaryRef)pList,
                                                CFSTR(SYSTEM_VERSION_KEY));
    if (valRef != NULL) {
        if (CFStringCompare( valRef,
                             CFSTR("10.5"),
                             kCFCompareCaseInsensitive) >= 0) {
            *pbIsLeopard = 1;
        }
        else {
            *pbIsLeopard = 0;
        }
    }

cleanup:

    if (pList) {
        CFRelease(pList);
    }

    return result;
}

