#ifndef __LWIMACINTERFACE_FIREWALL_H__
#define __LWIMACINTERFACE_FIREWALL_H__

#include "gpplistutils.h"
#include "lwimacifaceutils.h"
#include <unistd.h>

#define FIREWALL_PLIST_FILE_PATH                LIBRARY_PREFERENCES_PATH "/com.apple.sharing.firewall.plist"
#define FIREWALL_PLIST_FILE_PATH_LEOPARD        LIBRARY_PREFERENCES_PATH "/com.apple.alf.plist"

#define FIREWALL_LOGGING_ENABLED_TAG            "loggingenabled"
#define TIGER_FIREWALL_STATE_TAG                "state"
#define LEOPARD_FIREWALL_STATE_TAG              "globalstate"
#define UDPENABLED_TAG                          "udpenabled"
#define STEALTHENABLED_TAG                      "stealthenabled"

int
EditFirewallState(
    int bValue
    );

int
IsFirewallEnabled(
    int* pbValue
    );

int
EnableFirewallLogging(
    int bValue
    );

int
IsFirewallLoggingEnabled(
    int* pbValue
    );

int
BlockUDPTraffic(
    int bValue
    );

int
IsUDPTrafficBlocked(
    int* pbValue
    );

int
EnableStealthMode(
    int bValue
    );

int
IsStealthModeEnabled(
    int* pbValue
    );

#endif

