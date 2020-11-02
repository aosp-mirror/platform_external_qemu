/* Copyright (C) 2015-2017 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

// This file contains values that are used for saving and
// restoring user settings associated with the Qt-based UI.

// For settings that apply to all AVDs, use the default QSettings
// object:
//     QSettings universalSettings;
//
// For settings that apply only to the current AVD, construct
// a QSettings object using the AVD's path:
//     QSettings perAvdSettings(<avd path> + PER_AVD_SETTINGS_NAME, QSettings::IniFormat);

#pragma once

namespace Ui {
namespace Settings {

// ***** These settings apply to all AVDs *****

constexpr char ORG_NAME[] = "Android Open Source Project";
constexpr char ORG_DOMAIN[] = "android.com";
constexpr char APP_NAME[] = "Emulator";

// Update SettingsPage::on_set_resetNotifications_pressed when new
// "don't ask again" settings are added.
constexpr char SHOW_ADB_WARNING[] = "showAdbWarning";
constexpr char SHOW_AVD_ARCH_WARNING[] = "showAvdArchWarning";
constexpr char SHOW_GPU_WARNING[] = "showGpuWarning";
constexpr char SHOW_VIRTUALSCENE_INFO[] = "showVirtualSceneInfo";
#ifdef _WIN32
constexpr char SHOW_VGK_WARNING[] = "showVgkWarning";
#endif
constexpr char SHOW_NESTED_WARNING[] = "showNestedWarning";

// Note: The "set/" here is because these settings are
//       associated with "settings-page", not because
//       we're in "namespace Settings".

constexpr char ADB_PATH[] = "set/adbPath";
constexpr char AUTO_FIND_ADB[] = "set/autoFindAdb";
constexpr char ALWAYS_ON_TOP[] = "set/alwaysOnTop";
constexpr char FORWARD_SHORTCUTS_TO_DEVICE[] = "set/forwardShortcutsToDevice";
constexpr char FRAME_ALWAYS[] = "set/frameAlways4"; // Do not use "set/frameAlways": the current
                                                    // version is 4
                                                    // (for canary, default with frame ON for Wear
                                                    // devices, default OFF for everything else)
constexpr char SAVE_PATH[] = "set/savePath";
constexpr char UI_THEME[] = "set/theme";
constexpr char DISABLE_MOUSE_WHEEL[] = "set/disableMouseWheel";

constexpr char BATTERY_CHARGE_LEVEL[]  = "battery/charge_level";
constexpr char BATTERY_CHARGER_TYPE[]  = "battery/charger_type";
constexpr char BATTERY_CHARGER_TYPE2[] = "battery/charger_type2";
constexpr char BATTERY_HEALTH[]        = "battery/health";
constexpr char BATTERY_STATUS[]        = "battery/status";

constexpr char CELLULAR_NETWORK_TYPE[]    = "cell/network_type";
constexpr char CELLULAR_SIGNAL_STRENGTH[] = "cell/signal_strength";
constexpr char CELLULAR_VOICE_STATUS[]    = "cell/voice_status";
constexpr char CELLULAR_METER_STATUS[]    = "cell/meter_status";
constexpr char CELLULAR_DATA_STATUS[]     = "cell/data_status";

constexpr char CRASHREPORT_PREFERENCE[] = "set/crashReportPreference";
enum CRASHREPORT_PREFERENCE_VALUE {
    CRASHREPORT_PREFERENCE_ASK = 0,
    CRASHREPORT_PREFERENCE_ALWAYS = 1,
    CRASHREPORT_PREFERENCE_NEVER = 2
};
constexpr char CRASHREPORT_SAVEPREFERENCE_CHECKED[] = "set/crashReportSavePreferenceChecked";
constexpr int CRASHREPORT_COMBOBOX_ALWAYS = 0;
constexpr int CRASHREPORT_COMBOBOX_NEVER = 1;
constexpr int CRASHREPORT_COMBOBOX_ASK = 2;

constexpr char GLESBACKEND_PREFERENCE[] = "set/glesBackendPreference";
constexpr char GLESAPILEVEL_PREFERENCE[] ="set/glesApiLevelPreference";

constexpr char CLIPBOARD_SHARING[] = "set/clipboardSharing";

constexpr char HTTP_PROXY_USE_STUDIO[] = "set/proxy/useStudio";
constexpr char HTTP_PROXY_TYPE[] = "set/proxy/type";
enum HTTP_PROXY_TYPE {
    HTTP_PROXY_TYPE_NONE = 0,
    HTTP_PROXY_TYPE_MANUAL = 1
    // Future?: HTTP_PROXY_TYPE_AUTO = 2
};
constexpr char HTTP_PROXY_HOST[] = "set/proxy/host";
constexpr char HTTP_PROXY_PORT[] = "set/proxy/port";
constexpr char HTTP_PROXY_AUTHENTICATION[] = "set/proxy/authentication";
constexpr char HTTP_PROXY_USERNAME[] = "set/proxy/username";

constexpr char DELETE_INVALID_SNAPSHOTS[] = "set/deleteInvalidSnapshots";
// Enum values saved in the settings for save snapshot on exit
enum class DeleteInvalidSnapshots { Auto, Ask, No };
// Order of the items on the GUI for deleting invalid snapshots
enum class DeleteInvalidSnapshotsUiOrder { Auto, Ask, No };

constexpr char LOCATION_PLAYBACK_FILE[] = "loc/playback_file_path";
constexpr char LOCATION_PLAYBACK_SPEED[] = "loc/playback_speed";
constexpr char LOCATION_RECENT_ALTITUDE[] = "loc/recent_altitude";
constexpr char LOCATION_RECENT_HEADING[] = "loc/recent_heading";
constexpr char LOCATION_RECENT_LATITUDE[] = "loc/recent_latitude";
constexpr char LOCATION_RECENT_LONGITUDE[] = "loc/recent_longitude";
constexpr char LOCATION_RECENT_VELOCITY[] = "loc/recent_velocity";

constexpr char SCREENREC_SAVE_PATH[] = "rec/savePath";


// ***** These settings apply only to the current AVD *****

constexpr char PER_AVD_SETTINGS_NAME[] = "/AVD.conf";

constexpr char SAVE_SNAPSHOT_ON_EXIT[] = "perAvd/set/saveSnapshotOnExit";
// Enum values saved in the settings for save snapshot on exit
enum class SaveSnapshotOnExit { Always, Never, Ask };
// Order of the items on the GUI for save snapshot on exit
enum class SaveSnapshotOnExitUiOrder { Always, Never, Ask };

constexpr char PER_AVD_ALTITUDE[] = "perAvd/loc/altitude";
constexpr char PER_AVD_HEADING[] = "perAvd/loc/heading";
constexpr char PER_AVD_LONGITUDE[] = "perAvd/loc/longitude";
constexpr char PER_AVD_LATITUDE[] = "perAvd/loc/latitude";
constexpr char PER_AVD_VELOCITY[] = "perAvd/loc/velocity";

constexpr char PER_AVD_LOC_PLAYBACK_FILE[] = "perAvd/loc/playback_file_path";
constexpr char PER_AVD_LOC_PLAYBACK_SPEED[] = "perAvd/loc/playback_speed";

constexpr char PER_AVD_VIRTUAL_SCENE_POSTERS[] = "perAvd/virtualscene/posters";
constexpr char PER_AVD_VIRTUAL_SCENE_POSTER_SIZES[] =
        "perAvd/virtualscene/poster_sizes";
constexpr char PER_AVD_VIRTUAL_SCENE_TV_ANIMATION[] =
        "perAvd/virtualscene/tv_animation";

constexpr char PER_AVD_BATTERY_CHARGE_LEVEL[]  = "perAvd/battery/charge_level";
constexpr char PER_AVD_BATTERY_CHARGER_TYPE2[] = "perAvd/battery/charger_type2";
constexpr char PER_AVD_BATTERY_CHARGER_TYPE3[] = "perAvd/battery/charger_type3";
constexpr char PER_AVD_BATTERY_HEALTH[]        = "perAvd/battery/health";
constexpr char PER_AVD_BATTERY_STATUS[]        = "perAvd/battery/status";

constexpr char PER_AVD_CELLULAR_NETWORK_TYPE[]    = "perAvd/cell/network_type";
constexpr char PER_AVD_CELLULAR_SIGNAL_STRENGTH[] = "perAvd/cell/signal_strength";
constexpr char PER_AVD_CELLULAR_VOICE_STATUS[]    = "perAvd/cell/voice_status";
constexpr char PER_AVD_CELLULAR_METER_STATUS[]    = "perAvd/cell/meter_status";
constexpr char PER_AVD_CELLULAR_DATA_STATUS[]     = "perAvd/cell/data_status";
}  // namespace Settings
}  // namespace Ui
