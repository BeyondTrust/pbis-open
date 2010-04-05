#ifndef __LWIMACINTERFACE_ENERGY_H__
#define __LWIMACINTERFACE_ENERGY_H__


#define ENERGY_SAVER_FILE_PATH                  LIBRARY_PREFERENCES_PATH "/SystemConfiguration/com.apple.PowerManagement.plist"

#define ENERGY_SAVER_SYSTEM_SLEEP_TIMER         "System Sleep Timer"
#define ENERGY_SAVER_DISPLAY_SLEEP_TIMER        "Display Sleep Timer"
#define ENERGY_SAVER_DISK_SLEEP_TIMER           "Disk Sleep Timer"
#define ENERGY_SAVER_WAKE_ON_MODEM_RING         "Wake On Modem Ring"
#define ENERGY_SAVER_WAKE_ON_LAN                "Wake On LAN"
#define ENERGY_SAVER_SLEEP_ON_POWER_BUTTON      "Sleep On Power Button"
#define ENERGY_SAVER_AUTO_RESTART_ON_POWER_LOSS "Automatic Restart On Power Loss"

int
ApplyEnergySaverSettings(
    int bValue,
    char *pszSettingName
    );

#endif

