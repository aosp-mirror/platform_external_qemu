/* Copyright (C) 2015-2016 The Android Open Source Project
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

#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/gps/GpsFix.h"
#include "android/hw-sensors.h"
#include "android/settings-agent.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/extended-pages/common.h"
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
#include <QShowEvent>

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

    void showPane(ExtendedWindowPane pane);

private:
    void closeEvent(QCloseEvent *ce) override;

    EmulatorQtWindow    *mEmulatorWindow;
    ToolWindow          *mToolWindow;
    GeoDataLoaderThread *mGeoDataLoader;
    GpsFixArray          mGpsFixesArray;

    void initLocation();
    void initVirtualSensors();

    const QAndroidEmulatorWindowAgent* mEmulatorWindowAgent;
    const QAndroidLocationAgent* mLocationAgent;
    const QAndroidSensorsAgent* mSensorsAgent;
    const SettingsAgent* mSettingsAgent;

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
    bool mFirstShowEvent = true;

    void    adjustTabs(ExtendedWindowPane thisIndex);
    void    loc_appendToTable(int row,
                              const std::string& lat,
                              const std::string& lon,
                              const std::string& elev,
                              const std::string& name,
                              const std::string& description,
                              time_t time);

    QIcon getIconForCurrentTheme(const QString& icon_name) {
        QString iconType =
            getSelectedTheme() == SETTINGS_THEME_LIGHT ? LIGHT_PATH : DARK_PATH;
        return QIcon(":/" + iconType + "/" + icon_name);
    }

private slots:
    void switchOnTop(bool isOntop);
    void switchToTheme(SettingsTheme theme);

    // Master tabs
    void on_batteryButton_clicked();
    void on_cellularButton_clicked();
    void on_dpadButton_clicked();
    void on_fingerButton_clicked();
    void on_helpButton_clicked();
    void on_locationButton_clicked();
    void on_settingsButton_clicked();
    void on_telephoneButton_clicked();

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

    // Sensors
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void on_lightSensorValueWidget_valueChanged(double value);
    void on_pressureSensorValueWidget_valueChanged(double value);
    void on_humiditySensorValueWidget_valueChanged(double value);
    void onPhoneRotationChanged();

private:
    void showEvent(QShowEvent* e) override;
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
