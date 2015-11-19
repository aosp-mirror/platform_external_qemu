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
#include <QTimer>
#include <QDoubleValidator>

#include <vector>
#include <map>

class EmulatorQtWindow;
class ToolWindow;

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
    void closeEvent(QCloseEvent *ce);

    EmulatorQtWindow   *mParentWindow;
    ToolWindow         *mToolWindow;

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
    const QAndroidEmulatorWindowAgent* mEmulatorWindow;
    const QAndroidFingerAgent* mFingerAgent;
    const QAndroidLocationAgent* mLocationAgent;
    const QAndroidSensorsAgent* mSensorsAgent;
    const QAndroidTelephonyAgent* mTelephonyAgent;
    const SettingsAgent* mSettingsAgent;
    const QAndroidUserEventAgent* mUserEventsAgent;

    int      mLoc_mSecRemaining;
    bool     mLoc_nowPlaying;
    int      mLoc_rowToSend;
    QTimer   mLoc_timer;
    QDoubleValidator mMagFieldValidator;
    QDoubleValidator mAltitudeValidator;
    std::map<ExtendedWindowPane, QPushButton*> mPaneButtonMap;
    const ShortcutKeyStore<QtUICommand>* mQtUIShortcuts;
    Ui::ExtendedControls *mExtendedUi;

    void    adjustTabs(ExtendedWindowPane thisIndex);
    void    loc_appendToTable(std::string lat,
                              std::string lon,
                              std::string elev,
                              std::string name,
                              std::string description,
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
    void on_bat_chargerBox_currentIndexChanged(int value);
    void on_bat_levelSlider_valueChanged(int value);
    void on_bat_healthBox_currentIndexChanged(int index);
    void on_bat_statusBox_currentIndexChanged(int index);

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
    bool loc_cellIsValid(QTableWidget *table, int row, int col);
    void loc_populateTable(GpsFixArray *fixes);
    void loc_slot_timeout();

    void locationPlaybackStart();
    void locationPlaybackStop();

    // Settings
    void on_set_folderButton_clicked();
    void on_set_themeBox_currentIndexChanged(int index);
    void on_set_saveLocBox_textEdited(const QString&);

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
