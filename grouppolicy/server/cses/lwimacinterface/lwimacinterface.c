#include "lwimaciface_bluetooth.h"
#include "lwimaciface_energy.h"
#include "lwimaciface_firewall.h"
#include "lwimaciface_network.h"
#include "lwimaciface_security.h"

/* Bluetooth settings */
int
GPEnableBluetoothControllerState(
    int bValue
    )
{
    return EnableBluetoothControllerState(bValue);
}

int
GPApplyBluetoothSetupAssistantSetting(
    int bValue
    )
{
    return ApplyBluetoothSetupAssistantSetting(bValue);
}

int
GPApplyBluetoothShareInternetConnectionSetting(
    int bValue
    )
{
    return ApplyBluetoothShareInternetConnectionSetting(bValue);
}

/* Energy saver settings */
int
GPApplyEnergySaverSettings(
    int bValue,
    char *pszSettingName
    )
{
	return  ApplyEnergySaverSettings(bValue,pszSettingName);
}

/* Firewall settings */
int
GPEditFirewallState(
    int bValue
    )
{
	return  EditFirewallState(bValue);
}

int
GPIsFirewallEnabled(
    int* pbValue
    )
{
	return  IsFirewallEnabled(pbValue);
}

int
GPEnableFirewallLogging(
    int bValue
    )
{
	return  EnableFirewallLogging(bValue);
}

int
GPIsFirewallLoggingEnabled(
    int* pbValue
    )
{
	return  IsFirewallLoggingEnabled(pbValue);
}

int
GPBlockUDPTraffic(
    int bValue
    )
{
	return  BlockUDPTraffic(bValue);
}

int
GPIsUDPTrafficBlocked(
    int* pbValue
    )
{
	return  IsUDPTrafficBlocked(pbValue);
}

int
GPEnableStealthMode(
    int bValue
    )
{
	return  EnableStealthMode(bValue);
}

int
GPIsStealthModeEnabled(
    int* pbValue
    )
{
	return  IsStealthModeEnabled(pbValue);
}


/* Network settings */
int
GPApplyAppleTalkSettings( 
    int nMode,
    int nNodeId,
    int nNetId)
{
    return ApplyAppleTalkSettings(nMode,nNodeId,nNetId);
}


int
GPApplyDNSSettings( 
    char *pszServerAddresses,
    char *pszSearchDomains
    )
{
    return ApplyDNSSettings(pszServerAddresses,pszSearchDomains);
}

/* Security settings */
int
GPRequirePasswordForEachSecureSystemPreference(
    int bValue
    )
{
	return RequirePasswordForEachSecureSystemPreference(bValue);
}

int
GPIsPasswordForEachSecureSystemPreferenceRequired(
    int* pbValue
    )
{
	return IsPasswordForEachSecureSystemPreferenceRequired(pbValue);
}

int
GPDisableAutomaticLogin(
    int bValue
    )
{
	return DisableAutomaticLogin(bValue);
}

int
GPIsAutomaticLoginDisabled(
    int* pbValue
    )
{
	return IsAutomaticLoginDisabled( pbValue);
}

int
GPUseSecureVirtualMemory(
    int bValue
    )
{
	return UseSecureVirtualMemory(bValue);
}

int
GPIsSecureVirtualMemoryUsed(
    int* pbValue
    )
{
	return IsSecureVirtualMemoryUsed(pbValue);
}

int
GPLogoutInMinutesOfInactivity(
    int nMinutes
    )
{
	return LogoutInMinutesOfInactivity(nMinutes);
}

int
GPGetMinutesOfInactivityToLogout(
    int* pNMinutes
    )
{
	return GetMinutesOfInactivityToLogout(pNMinutes);
}

