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

#include "android/ui-emu-agent.h"

#include <QFile>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QValidator>

#include "android/battery-agent.h"
#include "android/cellular-agent.h"
#include "android/finger-agent.h"
#include "android/location-agent.h"
#include "android/settings-agent.h"
#include "android/telephony-agent.h"

class EmulatorQtWindow;
class ToolWindow;

namespace Ui {
    class ExtendedControls;
}

class ExtendedWindow : public QFrame
{
    Q_OBJECT

public:
    explicit ExtendedWindow(EmulatorQtWindow *eW, ToolWindow *tW, const UiEmuAgent *agentPtr);

    void     completeInitialization();

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
    void initFinger();
    void initKbdShorts();
    void initLocation();
    void initSd();
    void initSettings();
    void initSms();
    void initTelephony();

    BatteryState    mBatteryState;
    SettingsState   mSettingsState;
    TelephonyState  mTelephonyState;

    const BatteryAgent    *mBatteryAgent;
    const CellularAgent   *mCellularAgent;
    const FingerAgent     *mFingerAgent;
    const LocationAgent   *mLocationAgent;
    const SettingsAgent   *mSettingsAgent;
    const TelephonyAgent  *mTelephonyAgent;

    int      mLoc_mSecRemaining;
    bool     mLoc_nowPaused;
    int      mLoc_rowToSend;
    QTimer   mLoc_timer;

    Ui::ExtendedControls *mExtendedUi;

    void    adjustTabs(QPushButton *thisButton, int thisIndex);
    void    loc_appendToTable(std::string lat,
                              std::string lon,
                              std::string elev,
                              std::string name,
                              std::string description,
                              std::string time);

    void    setButtonEnabled(QPushButton *theButton, bool isEnabled);

private slots:
    // Master tabs
    void on_batteryButton_clicked();
    void on_cameraButton_clicked();
    void on_cellularButton_clicked();
    void on_dpadButton_clicked();
    void on_fingerButton_clicked();
    void on_hwSensorsButton_clicked();
    void on_kbdShortsButton_clicked();
    void on_locationButton_clicked();
    void on_messageButton_clicked();
    void on_recordScrButton_clicked();
    void on_sdButton_clicked();
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

    // Fingerprint
    void on_finger_touchButton_pressed();
    void on_finger_touchButton_released();

    // Location
    void on_loc_addRowButton_clicked();
    void on_loc_GpxButton_clicked();
    void on_loc_KmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_pauseButton_clicked();
    void on_loc_playButton_clicked();
    void on_loc_removeRowButton_clicked();
    void on_loc_stopButton_clicked();

    bool loc_cellIsValid(QTableWidget *table, int row, int col);
    void loc_slot_timeout();

    // Settings
    void on_set_themeBox_currentIndexChanged(int index);

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
