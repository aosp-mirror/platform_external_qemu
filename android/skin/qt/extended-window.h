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
#include "android/gps/GpsFix.h"
#include "android/hw-sensors.h"
#include "android/settings-agent.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/ui-emu-agent.h"

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
        bool          mIsCharging;
        int           mChargeLevel; // Percent
        BatteryHealth mHealth;
        BatteryStatus mStatus;

        BatteryState() :
            mIsCharging(true),
            mChargeLevel(50),
            mHealth(BATTERY_HEALTH_GOOD),
            mStatus(BATTERY_STATUS_CHARGING) { }
    };

    typedef enum { Call_Inactive, Call_Active, Call_Held } CallActivity;

    class SettingsState {
    public:
        SettingsTheme mTheme;

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
            mPhoneNumber("6505551212") // TODO: Change to "(650) 555-1212"
        { }
    };

    void initBattery();
    void initCellular();
    void initDPad();
    void initFinger();
    void initKbdShorts();
    void initLocation();
    void initSettings();
    void initSms();
    void initTelephony();
    void initVirtualSensors();

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

    int      mLoc_mSecRemaining;
    bool     mLoc_nowPaused;
    bool     mLoc_nowPlaying;
    int      mLoc_rowToSend;
    QTimer   mLoc_timer;
    QDoubleValidator mMagFieldValidator;
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
    void on_kbdShortsButton_clicked();
    void on_locationButton_clicked();
    void on_settingsButton_clicked();
    void on_telephoneButton_clicked();
    void on_virtSensorsButton_clicked();

    // Battery
    void on_bat_chargeCkBox_toggled(bool checked);
    void on_bat_levelSlider_valueChanged(int value);
    void on_bat_healthBox_currentIndexChanged(int index);
    void on_bat_statusBox_currentIndexChanged(int index);

    // Cellular
    void on_cell_signalStrengthSlider_valueChanged(int value);
    void on_cell_standardBox_currentIndexChanged(int index);
    void on_cell_voiceStatusBox_currentIndexChanged(int index);
    void on_cell_dataStatusBox_currentIndexChanged(int index);

    // DPad
#define ON_PRESS_RELEASE_CLICK(button) \
    void on_dpad_ ## button ## Button_pressed(); \
    void on_dpad_ ## button ## Button_released(); \
    void on_dpad_ ## button ## Button_clicked();

    ON_PRESS_RELEASE_CLICK(back);
    ON_PRESS_RELEASE_CLICK(down);
    ON_PRESS_RELEASE_CLICK(forward);
    ON_PRESS_RELEASE_CLICK(left);
    ON_PRESS_RELEASE_CLICK(play);
    ON_PRESS_RELEASE_CLICK(right);
    ON_PRESS_RELEASE_CLICK(select);
    ON_PRESS_RELEASE_CLICK(up);

    // Fingerprint
    void on_finger_touchButton_pressed();
    void on_finger_touchButton_released();

    // Location
    void on_loc_addRowButton_clicked();
    void on_loc_GpxKmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_removeRowButton_clicked();
    void on_loc_stopButton_clicked();
    void on_loc_pauseButton_clicked();
    void on_loc_playButton_clicked();

    bool loc_cellIsValid(QTableWidget *table, int row, int col);
    void loc_populateTable(GpsFixArray *fixes);
    void loc_slot_timeout();

    // Settings
    void on_set_themeBox_currentIndexChanged(int index);

    // Sensors
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void onPhoneRotationChanged();

    // SMS messaging
    void on_sms_sendButton_clicked();

    // Telephony
    void on_tel_startCallButton_clicked();
    void on_tel_endCallButton_clicked();
    void on_tel_holdCallButton_clicked();
};

class phoneNumberValidator : public QValidator
{
public:
    State validate(QString &input, int &pos) const;
};
