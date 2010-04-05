#ifndef __LWIMACINTERFACE_BLUETOOTH_H__
#define __LWIMACINTERFACE_BLUETOOTH_H__


#define BLUETOOTH_PLIST_FILE_PATH               LIBRARY_PREFERENCES_PATH "/com.apple.Bluetooth.plist"
#define BLUETOOTH_ENABLED_TAG                   "ControllerPowerState"
#define BLUETOOTH_SETUP_ASSISTANT_KEY           "BluetoothAutoSeekHIDDevices"
#define BLUETOOTH_SHARE_INTERNET_CONNECTION_KEY "PANServices"

int
EnableBluetoothControllerState(
    int bValue
    );

int
ApplyBluetoothSetupAssistantSetting(
    int bValue
    );

int
ApplyBluetoothShareInternetConnectionSetting(
    int bValue
    );

#endif
