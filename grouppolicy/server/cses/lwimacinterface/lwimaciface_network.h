#ifndef __LWIMACINTERFACE_NETWORK_H__
#define __LWIMACINTERFACE_NETWORK_H__


#define NETID_TAG                               "NetworkID"
#define NODEID_TAG                              "NodeID"

#define DNS_SETTING_TAG                         "DNS Settings"
#define DNS_SERVERS_TAG                         "ServerAddresses"
#define DNS_SEARCH_DOMAINS_TAG                  "SearchDomains"

#define SYSCONF_PREFERENCES_PLIST_FILE_PATH     LIBRARY_PREFERENCES_PATH "/SystemConfiguration/preferences.plist"

int
ApplyAppleTalkSettings( int nMode,
                        int nNodeId,
                        int nNetId);

int
ApplyDNSSettings( char *pszServerAddresses,
                  char *pszSearchDomains);

#endif
