/* Copyright (C) 2015 The Android Open Source Project
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

#pragma once

#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/cellular_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/gps/GpsFix.h"
#include "android/hw-sensors.h"
#include "android/settings-agent.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/ui-emu-agent.h"
#include "android/utils/path.h"

#include <QFile>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTableWidget>
#include <QThread>
#include <QTimer>
#include <QDoubleValidator>

#include <vector>
#include <map>

class EmulatorQtWindow;
class ToolWindow;
class GeoDataLoaderThread;

namespace Ui {
    class ExtendedControls;
}

class ExtendedWindow : public QFrame
{
    Q_OBJECT

public:
    ExtendedWindow(
        EmulatorQtWindow *eW,
        ToolWindow *tW,
        const UiEmuAgent *agentPtr,
        const ShortcutKeyStore<QtUICommand>* shortcuts);

    void completeInitialization();
    void showPane(ExtendedWindowPane pane);

    static void switchAllIconsForTheme(SettingsTheme theme);

private:

    ~ExtendedWindow();
    void closeEvent(QCloseEvent *ce) override;

    EmulatorQtWindow    *mEmulatorWindow;
    ToolWindow          *mToolWindow;
    GeoDataLoaderThread *mGeoDataLoader;
    GpsFixArray          mGpsFixesArray;

    class BatteryState {
    public:
        int            mChargeLevel; // Percent
        BatteryCharger mCharger;
        BatteryHealth  mHealth;
        BatteryStatus  mStatus;

        BatteryState() :
            mChargeLevel(50),
            mCharger(BATTERY_CHARGER_AC),
            mHealth(BATTERY_HEALTH_GOOD),
            mStatus(BATTERY_STATUS_CHARGING) { }
    };

    typedef enum { Call_Inactive, Call_Active, Call_Held } CallActivity;

    bool eventFilter (QObject* object, QEvent* event) override;

    class SettingsState {
    public:
        SettingsTheme mTheme;
        QString       mSavePath;
        QString       mSdkPath;

        SettingsState() :
            mTheme(SETTINGS_THEME_LIGHT)
            { }
    };

    class TelephonyState {
    public:

        CallActivity  mActivity;
        QString       mPhoneNumber;

        TelephonyState() :
            mActivity(Call_Inactive),
            mPhoneNumber("6505551212")
        { }
    };

    void initBattery();
    void initCellular();
    void initDPad();
    void initFinger();
    void initHelp();
    void initKbdShorts();
    void initLocation();
    void initSettings();
    void initSms();
    void initTelephony();
    void initVirtualSensors();

    void completeSettingsInitialization();

    BatteryState    mBatteryState;
    SettingsState   mSettingsState;
    TelephonyState  mTelephonyState;

    const QAndroidBatteryAgent* mBatteryAgent;
    const QAndroidCellularAgent* mCellularAgent;
    const QAndroidEmulatorWindowAgent* mEmulatorWindowAgent;
    const QAndroidFingerAgent* mFingerAgent;
    const QAndroidLocationAgent* mLocationAgent;
    const QAndroidSensorsAgent* mSensorsAgent;
    const QAndroidTelephonyAgent* mTelephonyAgent;
    const SettingsAgent* mSettingsAgent;
    const QAndroidUserEventAgent* mUserEventsAgent;

    int      mLoc_mSecRemaining;
    bool     mLoc_nowPlaying;
    bool     mLoc_nowLoadingGeoData;
    int      mLoc_rowToSend;
    QTimer   mLoc_timer;
    bool     mCloseRequested;
    QDoubleValidator mMagFieldValidator;
    QDoubleValidator mAltitudeValidator;
    std::map<ExtendedWindowPane, QPushButton*> mPaneButtonMap;
    const ShortcutKeyStore<QtUICommand>* mQtUIShortcuts;
    Ui::ExtendedControls *mExtendedUi;

    void    adjustTabs(ExtendedWindowPane thisIndex);
    void    loc_appendToTable(int row,
                              const std::string& lat,
                              const std::string& lon,
                              const std::string& elev,
                              const std::string& name,
                              const std::string& description,
                              time_t time);

    static void setButtonEnabled(QPushButton*  theButton,
                                 SettingsTheme theme,
                                 bool          isEnabled);
    void    dpad_setPressed(QPushButton* button);
    void    dpad_setReleased(QPushButton* button);

    static QString apiVersionString(int apiVersion);

    QIcon getIconForCurrentTheme(const QString& icon_name) {
        QString iconType =
            mSettingsState.mTheme == SETTINGS_THEME_LIGHT ? LIGHT_PATH : DARK_PATH;
        return QIcon(":/" + iconType + "/" + icon_name);
    }
private slots:
    // Master tabs
    void on_batteryButton_clicked();
    void on_cellularButton_clicked();
    void on_dpadButton_clicked();
    void on_fingerButton_clicked();
    void on_helpButton_clicked();
    void on_locationButton_clicked();
    void on_settingsButton_clicked();
    void on_telephoneButton_clicked();

    // Battery
    void on_bat_chargerBox_activated(int value);
    void on_bat_healthBox_activated(int index);
    void on_bat_levelSlider_valueChanged(int value);
    void on_bat_statusBox_activated(int index);

    // Cellular
    void on_cell_dataStatusBox_currentIndexChanged(int index);
    void on_cell_delayBox_currentIndexChanged(int index);
    void on_cell_standardBox_currentIndexChanged(int index);
    void on_cell_voiceStatusBox_currentIndexChanged(int index);

    // DPad
#define ON_PRESS_RELEASE(button) \
    void on_dpad_ ## button ## Button_pressed(); \
    void on_dpad_ ## button ## Button_released(); \

    ON_PRESS_RELEASE(back);
    ON_PRESS_RELEASE(down);
    ON_PRESS_RELEASE(forward);
    ON_PRESS_RELEASE(left);
    ON_PRESS_RELEASE(play);
    ON_PRESS_RELEASE(right);
    ON_PRESS_RELEASE(select);
    ON_PRESS_RELEASE(up);

    // Fingerprint
    void on_finger_touchButton_pressed();
    void on_finger_touchButton_released();

    // Location
    void on_loc_GpxKmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_playStopButton_clicked();
    void on_loc_decimalModeSwitch_toggled(bool checked);
    void on_loc_sexagesimalModeSwitch_toggled(bool checked);
    void on_loc_sendPointButton_clicked();
    void on_loc_longitudeInput_valueChanged(double);
    void on_loc_latitudeInput_valueChanged(double);
    void on_loc_altitudeInput_editingFinished();
    void on_loc_playbackSpeed_currentIndexChanged(int index);
    bool loc_cellIsValid(QTableWidget *table, int row, int col);
    void loc_populateTable(GpsFixArray *fixes);
    void loc_slot_timeout();
    void loc_geoDataLoadingStarted();
    void loc_geoDataLoadingFinished(QString file_name, bool ok, QString error);
    void loc_startupGeoDataLoadingFinished(QString file_name, bool ok, QString error);

    void locationPlaybackStart();
    void locationPlaybackStop();

    // Settings
    void on_set_allowKeyboardGrab_toggled(bool);
    void on_set_saveLocBox_textEdited(const QString&);
    void on_set_saveLocFolderButton_clicked();
    void on_set_sdkPathBox_textEdited(const QString&);
    void on_set_sdkPathButton_clicked();
    void on_set_themeBox_currentIndexChanged(int index);

    // Help
    void on_help_docs_clicked();
    void on_help_fileBug_clicked();
    void on_help_sendFeedback_clicked();

    // Sensors
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void on_lightSensorValueWidget_valueChanged(double value);
    void on_pressureSensorValueWidget_valueChanged(double value);
    void on_humiditySensorValueWidget_valueChanged(double value);
    void onPhoneRotationChanged();

    // SMS messaging
    void on_sms_sendButton_clicked();

    // Telephony
    void on_tel_startEndButton_clicked();
    void on_tel_holdCallButton_clicked();
};

class phoneNumberValidator : public QValidator
{
public:
    State validate(QString &input, int &pos) const;
};

class GeoDataLoaderThread : public QThread {
Q_OBJECT
public:
    // Loads geo data from a gpx or kml file specified
    // by file_name into the GpsFixArray pointed to
    // by fixes
    void loadGeoDataFromFile(const QString& file_name, GpsFixArray* fixes);


    static GeoDataLoaderThread* newInstance(
            const QObject* handler,
            const char* started_slot,
            const char* finished_slot);

signals:
    void loadingFinished(QString file_name, bool ok, QString error);

protected:
    // Reimplemented to load the file into the given fixes array.
    void run() override;

private:
    GeoDataLoaderThread() : mFixes(nullptr) {}
    QString mFileName;
    GpsFixArray* mFixes;
};
