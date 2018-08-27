// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/location-page.h"

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/control/location_agent.h"
#include "android/globals.h"
#include "android/gps/GpxParser.h"
#include "android/gps/KmlParser.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/settings-agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"

#include <QFileDialog>
#include <QSettings>
#include <algorithm>
#include <string>
#include <utility>
#include <unistd.h>

using android::base::AutoLock;
using android::base::LazyInstance;

static constexpr double kGplexLon = -122.084;
static constexpr double kGplexLat =   37.422;
static constexpr double kGplexAlt =    5.0; // Meters
static constexpr double MAX_SPEED =  666.0; // Arbitrary (~speed of sound in knots)

static double getHeading(double startLat, double startLon, double endLat, double endLon);
static double getDistanceNm(double startLat, double startLon, double endLat, double endLon);
static void sendLocationToDevice();
static void getDeviceLocation(double* pLatitude, double* pLongitude, double* pAltitude);
static void getDeviceLocationFromSettings(double* pOutLatitude,
                                          double* pOutLongitude,
                                          double* pOutAltitude);
static void getDeviceVelocity(double* speed, double* heading);
static void putdeviceVelocity(double speed, double heading);

static const QAndroidLocationAgent* sLocationAgent = nullptr;
static double sDeviceSpeed = 0.0; // Knots
static double sDeviceHeading = 0.0; // Degrees 0=north, 90=east

static void updateThreadLoop();

struct LocationPageGlobals {
    LocationPage* locationPagePtr = nullptr;

    // For sending periodic GPS updates
    bool                             shouldCloseUpdateThread = false;
    android::base::ConditionVariable updateThreadCv;
    android::base::Lock              updateThreadLock;
    android::base::FunctorThread     updateThread { &updateThreadLoop };
};

static LazyInstance<LocationPageGlobals> sGlobals = LAZY_INSTANCE_INIT;

static void updateThreadLoop() {
    // Update the location every second, until we exit
    static constexpr int sleepMicrosec = 1 * 1000 * 1000;
    AutoLock lock(sGlobals->updateThreadLock);
    auto now = android::base::System::get()->getUnixTimeUs();
    auto wakeTime = now + sleepMicrosec;

    while (1) {
        bool gotSignal = sGlobals->updateThreadCv.timedWait(&sGlobals->updateThreadLock, wakeTime);

        if (sGlobals->shouldCloseUpdateThread) break;

        now = android::base::System::get()->getUnixTimeUs();
        if (!gotSignal && now < wakeTime) {
            // We did not get the signal and it's too early for
            // time out. This is a spurious return from timedWait().
            // Just wait again, without updating "wakeTime".
            continue;
        }
        if (sLocationAgent &&
            sLocationAgent->gpsGetPassiveUpdate())
        {
            sendLocationToDevice();
        }
        wakeTime = now + sleepMicrosec;
    }
}

LocationPage::LocationPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::LocationPage)
{
    sGlobals->locationPagePtr = this;
    mUi->setupUi(this);
    mTimer.setSingleShot(true);

    // We can only send 1 decimal of altitude (in meters).
    mAltitudeValidator.setNotation(QDoubleValidator::StandardNotation);
    mAltitudeValidator.setRange(-1000, 10000, 1);
    mAltitudeValidator.setLocale(QLocale::C);
    mUi->loc_altitudeInput->setValidator(&mAltitudeValidator);

    mUi->loc_latitudeInput->setMinValue(-90.0);
    mUi->loc_latitudeInput->setMaxValue(90.0);
    QObject::connect(&mTimer, &QTimer::timeout, this, &LocationPage::timeout);
    QObject::connect(this, &LocationPage::locationUpdateRequired,
                     this, &LocationPage::updateDisplayedLocation);
    QObject::connect(this, &LocationPage::populateNextGeoDataChunk,
                     this, &LocationPage::populateTableByChunks,
                     Qt::QueuedConnection);

    setButtonEnabled(mUi->loc_playStopButton, getSelectedTheme(), false);

    // Set the GUI to the current values
    double curLat, curLon, curAlt;
    getDeviceLocation(&curLat, &curLon, &curAlt);
    updateDisplayedLocation(curLat, curLon, curAlt);

    mUi->loc_altitudeInput->setText(QString::number(curAlt, 'f', 1));
    mUi->loc_latitudeInput->setValue(curLat);
    mUi->loc_longitudeInput->setValue(curLon);

    QSettings settings;
    mUi->loc_playbackSpeed->setCurrentIndex(
            getLocationPlaybackSpeedFromSettings());
    mUi->loc_pathTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QString location_data_file =
        getLocationPlaybackFilePathFromSettings();
    mGeoDataLoader = GeoDataLoaderThread::newInstance(
            this,
            SLOT(geoDataThreadStarted()),
            SLOT(startupGeoDataThreadFinished(QString, bool, QString)));
    mGeoDataLoader->loadGeoDataFromFile(location_data_file, &mGpsFixesArray);

    using android::metrics::PeriodicReporter;
    mMetricsReportingToken = PeriodicReporter::get().addCancelableTask(
            60 * 10 * 1000,  // reporting period
            [this](android_studio::AndroidStudioEvent* event) {
                if (mLocationUsed) {
                    event->mutable_emulator_details()
                            ->mutable_used_features()
                            ->set_gps(true);
                    mMetricsReportingToken.reset();  // Report it only once.
                    return true;
                }
                return false;
    });
}

LocationPage::~LocationPage() {
    sGlobals->locationPagePtr = nullptr;

    shutDown();
    if (mGeoDataLoader != nullptr) {
        // If there's a loader thread running in the background,
        // ignore all signals from it and wait for it to finish.
        mGeoDataLoader->blockSignals(true);
        mGeoDataLoader->wait();
    }
    mUi->loc_pathTable->blockSignals(true);
    mUi->loc_pathTable->clear();
}

static void locationAgentQtSettingsWriter(double lat, double lon, double alt) {
    LocationPage::writeDeviceLocationToSettings(lat, lon, alt);
    if (sGlobals->locationPagePtr) {
        sGlobals->locationPagePtr->locationUpdateRequired(lat, lon, alt);
    }
}

// static
void LocationPage::setLocationAgent(const QAndroidLocationAgent* agent) {
    sLocationAgent = agent;

    location_registerQtSettingsWriter(locationAgentQtSettingsWriter);

    double curLat, curLon, curAlt;
    getDeviceLocation(&curLat, &curLon, &curAlt);
    writeDeviceLocationToSettings(curLat, curLon, curAlt);

    sendLocationToDevice();

    sGlobals->updateThread.start();
}

// static
void LocationPage::shutDown() {
    AutoLock lock(sGlobals->updateThreadLock);
    sGlobals->shouldCloseUpdateThread = true;
    sGlobals->updateThreadCv.signalAndUnlock(&lock);
    sGlobals->updateThread.wait();
}

void LocationPage::on_loc_GpxKmlButton_clicked()
{
    // Use dialog to get file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open GPX or KML File"), ".",
                                                    tr("GPX and KML files (*.gpx *.kml)"));

    if (fileName.isNull()) return;

    // Asynchronously parse the file with geo data.
    // If the file is big enough, parsing it synchronously will cause a noticeable
    // hiccup in the UI.
    mGeoDataLoader = GeoDataLoaderThread::newInstance(this,
                                                      SLOT(geoDataThreadStarted()),
                                                      SLOT(geoDataThreadFinished(QString, bool, QString)));
    mGeoDataLoader->loadGeoDataFromFile(fileName, &mGpsFixesArray);
}

void LocationPage::on_loc_pathTable_cellChanged(int row, int col)
{
    // If the cell's contents are bad, turn the cell red
    QString outErrorMessage;
    bool cellOK = validateCell(mUi->loc_pathTable, row, col, &outErrorMessage);
    QColor normalColor =
        getSelectedTheme() == SETTINGS_THEME_LIGHT ? Qt::black : Qt::white;
    QColor newColor = (cellOK ? normalColor : Qt::red);
    mUi->loc_pathTable->item(row, col)->setForeground(QBrush(newColor));
}

void LocationPage::on_loc_playStopButton_clicked() {
    mLocationUsed = true;
    if (mNowPlaying) {
        locationPlaybackStop();
    } else {
        locationPlaybackStart();
    }
}

void LocationPage::on_loc_modeSwitch_currentIndexChanged(int index) {
    if (index == 1) {
        mUi->loc_latitudeInput->setInputMode(AngleInputWidget::InputMode::Sexagesimal);
        mUi->loc_longitudeInput->setInputMode(AngleInputWidget::InputMode::Sexagesimal);
    } else {
        mUi->loc_latitudeInput->setInputMode(AngleInputWidget::InputMode::Decimal);
        mUi->loc_longitudeInput->setInputMode(AngleInputWidget::InputMode::Decimal);
    }
}

void LocationPage::on_loc_sendPointButton_clicked() {
    mLocationUsed = true;
    mUi->loc_latitudeInput->forceUpdate();
    mUi->loc_longitudeInput->forceUpdate();

    double lat = 0.0;
    double lon = 0.0;
    double altitude = mUi->loc_altitudeInput->text().toDouble();
    if (altitude < -1000.0 || altitude > 10000.0) {
        double validAltitude;
        getDeviceLocationFromSettings(&lat, &lon, &validAltitude);
        mUi->loc_altitudeInput->setText(QString::number(validAltitude));
    }

    AutoLock lock(sGlobals->updateThreadLock);
    lat = mUi->loc_latitudeInput->value(),
    lon = mUi->loc_longitudeInput->value(),
    altitude = mUi->loc_altitudeInput->text().toDouble();
    updateDisplayedLocation(lat, lon, altitude);

    sGlobals->updateThreadCv.signalAndUnlock(&lock);
}

void LocationPage::updateDisplayedLocation(double lat, double lon, double alt) {
    QString curLoc = tr("Longitude: %1\nLatitude: %2\nAltitude: %3")
                     .arg(lon, 0, 'f', 4).arg(lat, 0, 'f', 4).arg(alt, 0, 'f', 1);
    mUi->loc_currentLoc->setPlainText(curLoc);
    writeDeviceLocationToSettings(lat, lon, alt);
}

void LocationPage::on_loc_longitudeInput_valueChanged(double value) {
    // no-op
}

void LocationPage::on_loc_latitudeInput_valueChanged(double value) {
    // no-op
}

void LocationPage::on_loc_altitudeInput_editingFinished() {
    // no-op
}

void LocationPage::on_loc_playbackSpeed_currentIndexChanged(int index) {
    writeLocationPlaybackSpeedToSettings(index);
}

bool LocationPage::validateCell(QTableWidget* table,
                                int row,
                                int col,
                                QString* outErrorMessage) {
    QTableWidgetItem *theItem;
    double cellValue;
    bool cellOK = true;

    // The entry is a number. Check its range.
    switch (col) {
        case 0: // Delay
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) {
                *outErrorMessage = tr("Delay must be a number.");
                return false;
            }
            // Except for the first entry, the delay must be >= 1 millisecond
            if (row != 0 && cellValue < 0.001) {
                *outErrorMessage =
                        tr("Delay must be >= 1 millisecond with the exception "
                           "of the first entry.");
                cellOK = false;
            }
            break;
        case 1: // Latitude
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) {
                *outErrorMessage = tr("Latitude must be a number.");
                return false;
            }
            if (cellValue < -90.0 || cellValue > 90.0) {
                *outErrorMessage =
                        tr("Latitude must be between -90 and 90, inclusive.");
                cellOK = false;
            }
            break;
        case 2: // Longitude
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) {
                *outErrorMessage = tr("Longitude must be a number.");
                return false;
            }
            if (cellValue < -180.0 || cellValue > 180.0) {
                *outErrorMessage = tr(
                        "Longitude must be between -180 and 180, inclusive.");
                cellOK = false;
            }
            break;
        case 3: // Elevation
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) {
                *outErrorMessage = tr("Elevation must be a number.");
                return false;
            }
            if (cellValue < -1000.0 || cellValue > 10000.0) {
                *outErrorMessage = tr(
                        "Altitude must be between -1000 and 10000, inclusive.");
                cellOK = false;
            }
            break;
        default:
            // Name, description: Anything is OK
            cellOK = true;
            break;
    }
    return cellOK;
}


static QTableWidgetItem* itemForTable(QString&& text) {
    QTableWidgetItem* item = new QTableWidgetItem(std::move(text));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

void LocationPage::populateTableByChunks() {
    const GpsFixArray& fixes = mGpsFixesArray;

    if (mGpsNextPopulateIndex == 0) {
        // Starting: cleanup the table and prepare for loading.
        mUi->loc_pathTable->blockSignals(true);
        mUi->loc_pathTable->clearContents();
        mUi->loc_pathTable->setRowCount(fixes.size());
        mUi->loc_pathTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    if (mGpsNextPopulateIndex < (int)fixes.size()) {
        // Special case, the first row will have delay 0
        time_t previousTime = fixes[std::max(mGpsNextPopulateIndex - 1, 0)].time;
        // Calculate the last chunk index.
        const int endIndex = std::min<int>(mGpsNextPopulateIndex + 100, fixes.size());
        int i = mGpsNextPopulateIndex;
        for (; i < endIndex; i++) {
            const GpsFix& fix = fixes[i];
            time_t delay = fix.time - previousTime; // In seconds

            // Ensure all other delays are > 0, even if multiple points have
            // the same timestamp.
            if (delay == 0 && i != 0) {
                delay = 2;
            }

            mUi->loc_pathTable->setItem(i, 0, itemForTable(QString::number(delay)));
            mUi->loc_pathTable->setItem(i, 1, itemForTable(QString::number(fix.latitude)));
            mUi->loc_pathTable->setItem(i, 2, itemForTable(QString::number(fix.longitude)));
            mUi->loc_pathTable->setItem(i, 3, itemForTable(QString::number(fix.elevation)));
            mUi->loc_pathTable->setItem(i, 4, itemForTable(QString::fromStdString(fix.name)));
            mUi->loc_pathTable->setItem(i, 5, itemForTable(QString::fromStdString(fix.description)));

            previousTime = fix.time;
        }
        mGpsNextPopulateIndex = i;
    }

    if (mGpsNextPopulateIndex < (int)fixes.size()) {
        // More data to load: allow Qt to handle pending events and then process
        // the next chunk via a queued signal conneciton.
        emit populateNextGeoDataChunk();
    } else {
        // Done.
        mUi->loc_pathTable->setEditTriggers(
                    QAbstractItemView::DoubleClicked |
                    QAbstractItemView::EditKeyPressed |
                    QAbstractItemView::AnyKeyPressed);
        mUi->loc_pathTable->blockSignals(false);
        updateControlsAfterLoading();
    }
}

void LocationPage::locationPlaybackStart()
{
    if (mUi->loc_pathTable->rowCount() <= 0) {
        return;
    }

    mRowToSend = std::max(0, mUi->loc_pathTable->currentRow());

    SettingsTheme theme = getSelectedTheme();

    // Disable editing the data in the table while playback is in progress.
    mUi->loc_pathTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Disable loading a new dataset while playback is in progress.
    setButtonEnabled(mUi->loc_GpxKmlButton,  theme, false);

    // Change the icon on the play/stop button.
    mUi->loc_playStopButton->setIcon(getIconForCurrentTheme("stop"));
    mUi->loc_playStopButton->setProperty("themeIconName", "stop");


    // The timer will be triggered for the first row after this
    // function returns.
    mTimer.setInterval(0);
    mTimer.start();
    mNowPlaying = true;
}

void LocationPage::locationPlaybackStop()
{
    mTimer.stop();
    if (mRowToSend > 0 &&
        mRowToSend <= mUi->loc_pathTable->rowCount()) {
        if (mRowToSend == mUi->loc_pathTable->rowCount()) {
            mUi->loc_pathTable->setCurrentItem(nullptr);
        }
        mUi->loc_pathTable->item(mRowToSend - 1, 0)->setIcon(QIcon());
    }
    mRowToSend = -1;
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->loc_GpxKmlButton,  theme, true);
    mUi->loc_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->loc_playStopButton->setProperty("themeIconName", "play_arrow");
    mUi->loc_pathTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                                QAbstractItemView::EditKeyPressed |
                                                QAbstractItemView::AnyKeyPressed);
    putdeviceVelocity(0.0, 0.0);
    mNowPlaying = false;
}


void LocationPage::timeout() {
    bool cellOK;
    QTableWidgetItem *theItem;

    // Check if we've reached the end of the dataset.
    if (mRowToSend < 0  ||
        mRowToSend >= mUi->loc_pathTable->rowCount()) {
        // No more to send. Same as clicking Stop.
        locationPlaybackStop();
        return;
    }

    // Validate the current cell.
    QString outErrorMessage;
    for (int col = 0; col < mUi->loc_pathTable->columnCount(); col++) {
        if (!validateCell(mUi->loc_pathTable, mRowToSend, col, &outErrorMessage)) {
            mUi->loc_pathTable->setCurrentCell(mRowToSend, col);
            showErrorDialog(tr("This entry contains errors. The location "
                               "was not sent.<br/>Error: %1")
                                    .arg(outErrorMessage),
                            tr("GPS Playback"));
            // Stop the playback here
            locationPlaybackStop();
            return;
        }
    }

    // Get the data for the point we're about to send.
    theItem = mUi->loc_pathTable->item(mRowToSend, 1);
    double lat = theItem->text().toDouble(&cellOK);
    theItem = mUi->loc_pathTable->item(mRowToSend, 2);
    double lon = theItem->text().toDouble(&cellOK);
    theItem = mUi->loc_pathTable->item(mRowToSend, 3);
    double alt = theItem->text().toDouble(&cellOK);

    // Get the speed and direction to the next point
    double speed = 0.0;
    double direction = 0.0;
    double deltaTimeSec = 0.0;
    int nextRow = mRowToSend + 1;
    if (nextRow < mUi->loc_pathTable->rowCount()) {
        theItem = mUi->loc_pathTable->item(nextRow, 0);
        deltaTimeSec = theItem->text().toDouble() / (mUi->loc_playbackSpeed->currentIndex() + 1);
        theItem = mUi->loc_pathTable->item(nextRow, 1);
        double nextLat = theItem->text().toDouble(&cellOK);
        theItem = mUi->loc_pathTable->item(nextRow, 2);
        double nextLon = theItem->text().toDouble(&cellOK);
        double distance = getDistanceNm(lat, lon, nextLat, nextLon);
        double deltaTimeHours = deltaTimeSec / (60.0 * 60.0);
        if (deltaTimeHours > 0.0) {
            speed = std::min(distance / deltaTimeHours, MAX_SPEED);
        }
        direction = getHeading(lat, lon, nextLat, nextLon);
    }
    putdeviceVelocity(speed, direction);

    emit targetHeadingChanged(direction);

    // Update the appearance of the table:
    // 1. Clear the "play arrow" icon from the previous point, if necessary.
    // 2. Set the "play arrow" icon near the point we're about to send.
    // 3. Scroll to the point that is being sent.
    // 4. Make it selected.
    if (mRowToSend > 0) {
        mUi->loc_pathTable->item(mRowToSend - 1, 0)->setIcon(QIcon());
    }
    QTableWidgetItem* currentItem = mUi->loc_pathTable->item(mRowToSend, 0);
    currentItem->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->loc_pathTable->scrollToItem(currentItem);
    mUi->loc_pathTable->setCurrentItem(currentItem);

    AutoLock lock (sGlobals->updateThreadLock);
    updateDisplayedLocation(lat, lon, alt);

    // Send the command.
    sGlobals->updateThreadCv.signalAndUnlock(&lock);

    // Go on to the next row
    mRowToSend = nextRow;
    if (mRowToSend >= mUi->loc_pathTable->rowCount()) {
        // No more to send. Same as clicking Stop.
        locationPlaybackStop();
    } else {
        // Set a timer for when this row should be sent
        int mSec = deltaTimeSec * 1000.0;
        if (mSec < 0) mSec = 0;
        mTimer.setInterval(mSec);
        mTimer.start();
    }
}

void LocationPage::geoDataThreadStarted() {
    {
        QSignalBlocker blocker(mUi->loc_pathTable);
        mUi->loc_pathTable->setRowCount(0);
    }

    SettingsTheme theme = getSelectedTheme();

    // Prevent the user from initiating a load gpx/kml while another load is already
    // in progress
    setButtonEnabled(mUi->loc_GpxKmlButton, theme, false);
    setButtonEnabled(mUi->loc_playStopButton, theme, false);
    mNowLoadingGeoData = true;
}

void LocationPage::finishGeoDataLoading(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error) {
    mGeoDataLoader = nullptr;
    if (!ok) {
        if (!ignore_error) {
            showErrorDialog(error_message, tr("Geo Data Parser"));
        }
        updateControlsAfterLoading();
        return;
    }

    writeLocationPlaybackFilePathToSettings(file_name);
    mGpsNextPopulateIndex = 0;
    populateTableByChunks();
}

void LocationPage::startupGeoDataThreadFinished(QString file_name, bool ok, QString error_message) {
    // on startup, we silently ignore the previously remebered geo data file being
    // missing or malformed.
    finishGeoDataLoading(file_name, ok, error_message, true);
}

void LocationPage::geoDataThreadFinished(QString file_name, bool ok, QString error_message) {
    finishGeoDataLoading(file_name, ok, error_message, false);
}

void LocationPage::updateControlsAfterLoading() {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->loc_GpxKmlButton, theme, true);
    setButtonEnabled(mUi->loc_playStopButton, theme, true);
    mNowLoadingGeoData = false;
}

// static
void LocationPage::writeLocationPlaybackFilePathToSettings(const QString& file) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_LOC_PLAYBACK_FILE, file);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::LOCATION_PLAYBACK_FILE, file);
    }
}

// static
QString LocationPage::getLocationPlaybackFilePathFromSettings() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_LOC_PLAYBACK_FILE, "").toString();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::LOCATION_PLAYBACK_FILE, "").toString();
    }
}

// static
void LocationPage::writeLocationPlaybackSpeedToSettings(int speed) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_LOC_PLAYBACK_SPEED, speed);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::LOCATION_PLAYBACK_SPEED, speed);
    }
}

// static
int LocationPage::getLocationPlaybackSpeedFromSettings() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings.value(Ui::Settings::PER_AVD_LOC_PLAYBACK_SPEED, 0).toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings.value(Ui::Settings::LOCATION_PLAYBACK_SPEED, 0).toInt();
    }
}

static void getDeviceLocationFromSettings(double* pOutLatitude,
                                          double* pOutLongitude,
                                          double* pOutAltitude) {

    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        *pOutLatitude = avdSpecificSettings.value(Ui::Settings::PER_AVD_LATITUDE, kGplexLat).toDouble();
        *pOutLongitude = avdSpecificSettings.value(Ui::Settings::PER_AVD_LONGITUDE, kGplexLon).toDouble();
        *pOutAltitude = avdSpecificSettings.value(Ui::Settings::PER_AVD_ALTITUDE, kGplexAlt).toDouble();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        *pOutLatitude  = settings.value(Ui::Settings::LOCATION_RECENT_LATITUDE,
                                     kGplexLat).toDouble();
        *pOutLongitude = settings.value(Ui::Settings::LOCATION_RECENT_LONGITUDE,
                                     kGplexLon).toDouble();
        *pOutAltitude  = settings.value(Ui::Settings::LOCATION_RECENT_ALTITUDE,
                                     kGplexAlt).toDouble();
    }
}

// static
void LocationPage::writeDeviceLocationToSettings(double lat,
                                                 double lon,
                                                 double alt) {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_LATITUDE, lat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_LONGITUDE, lon);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_ALTITUDE, alt);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::LOCATION_RECENT_LATITUDE, lat);
        settings.setValue(Ui::Settings::LOCATION_RECENT_LONGITUDE, lon);
        settings.setValue(Ui::Settings::LOCATION_RECENT_ALTITUDE, alt);
    }
}

// Get the current location from the device. If that fails, use
// the saved location from this UI.
static void getDeviceLocation(double* pLatitude, double* pLongitude, double* pAltitude) {
    bool gotDeviceLoc = false;

    if (sLocationAgent && sLocationAgent->gpsGetLoc) {
        // Query the device
        gotDeviceLoc = sLocationAgent->gpsGetLoc(pLatitude, pLongitude, pAltitude, nullptr);
    }

    if (!gotDeviceLoc) {
        getDeviceLocationFromSettings(pLatitude, pLongitude, pAltitude);
    }
}

// Get the current speed and direction
static void getDeviceVelocity(double* speed, double* heading) {
    *speed = sDeviceSpeed;
    *heading = sDeviceHeading;
}

// Set the current speed and direction
static void putdeviceVelocity(double speed, double heading) {
    sDeviceSpeed = speed;
    sDeviceHeading = heading;
}

// Send a GPS location to the device
static void sendLocationToDevice() {
    if (sLocationAgent && sLocationAgent->gpsSendLoc) {
        // Send these to the device
        double latitude;
        double longitude;
        double altitude;
        getDeviceLocation(&latitude, &longitude, &altitude);

        double speed;   // Knots
        double heading; // Degrees
        getDeviceVelocity(&speed, &heading);

        timeval timeVal = {};
        gettimeofday(&timeVal, nullptr);
        sLocationAgent->gpsSendLoc(latitude, longitude, altitude, speed, heading, 4, &timeVal);
    }
}

void GeoDataLoaderThread::loadGeoDataFromFile(const QString& file_name, GpsFixArray* fixes) {
    mFileName = file_name;
    mFixes = fixes;
    start();
}

void GeoDataLoaderThread::run() {
    if (mFileName.isEmpty() || mFixes == nullptr) {
        emit(loadingFinished(mFileName, false, tr("No file to load")));
        return;
    }

    bool ok = false;
    std::string err_str;

    {
        QFileInfo file_info(mFileName);
        mFixes->clear();
        auto suffix = file_info.suffix().toLower();
        if (suffix == "gpx") {
            ok = GpxParser::parseFile(mFileName.toStdString().c_str(),
                                      mFixes, &err_str);
        } else if (suffix == "kml") {
            ok = KmlParser::parseFile(mFileName.toStdString().c_str(),
                                      mFixes, &err_str);
        } else {
            err_str = tr("Unknown file type").toStdString();
        }
    }

    auto err_qstring = QString::fromStdString(err_str);
    emit(loadingFinished(mFileName, ok, err_qstring));
}

GeoDataLoaderThread* GeoDataLoaderThread::newInstance(const QObject* handler,
                                                      const char* started_slot,
                                                      const char* finished_slot) {
    GeoDataLoaderThread* new_instance = new GeoDataLoaderThread();
    connect(new_instance, SIGNAL(started()), handler, started_slot);
    connect(new_instance, SIGNAL(loadingFinished(QString, bool, QString)), handler, finished_slot);

    // Make sure new_instance gets cleaned up after the thread exits.
    connect(new_instance, &QThread::finished, new_instance, &QObject::deleteLater);

    return new_instance;
}

// Determine the bearing from a starting location to an ending location.
// The ouput and all inputs are in degrees.
// The output is +/-180 with 0 = north, 90 = east, etc.
static double getHeading(double startLat, double startLon, double endLat, double endLon) {

    // The calculation for the initial bearing is
    //    aa = cos(latEnd) * sin(delta lon)
    //    bb = cos(latStart) * sin(latEnd) - sin(latStart) * cos(latEnd) * cos(delta lon)
    //    bearing = atan2(aa, bb)

    // We need to do the calculations in radians

    double startLatRadians = startLat * M_PI / 180.0;
    double startLonRadians = startLon * M_PI / 180.0;
    double endLatRadians   = endLat   * M_PI / 180.0;
    double endLonRadians   = endLon   * M_PI / 180.0;

    double deltaLonRadians = endLonRadians - startLonRadians;

    double aa = cos(endLatRadians) * sin(deltaLonRadians);
    double bb =   ( cos(startLatRadians) * sin(endLatRadians) )
                - ( sin(startLatRadians) * cos(endLatRadians) * cos(deltaLonRadians) );

    return (atan2(aa, bb) * 180.0 / M_PI);
}

// Determine the distance between two locations on earth.
// All inputs are in degrees.
// The output is in nautical miles(!)
//
// The Haversine formula is
//     a = sin^2(dLat/2) + cos(lat1) * cos(lat2) * sin^2(dLon/2)
//     c = 2 * atan2( sqrt(a), sqrt(1-a) )
//     dist = radius * c
static double getDistanceNm(double startLat, double startLon, double endLat, double endLon) {
    double startLatRadians = startLat * M_PI / 180.0;
    double startLonRadians = startLon * M_PI / 180.0;
    double endLatRadians   = endLat   * M_PI / 180.0;
    double endLonRadians   = endLon   * M_PI / 180.0;

    double deltaLatRadians = endLatRadians - startLatRadians;
    double deltaLonRadians = endLonRadians - startLonRadians;

    double sinDeltaLatBy2 = sin(deltaLatRadians / 2.0);
    double sinDeltaLonBy2 = sin(deltaLonRadians / 2.0);

    double termA =   sinDeltaLatBy2 * sinDeltaLatBy2
                   + cos(startLatRadians) * cos(endLatRadians) * sinDeltaLonBy2 * sinDeltaLonBy2;
    double termC = 2.0 * atan2(sqrt(termA), sqrt(1.0 - termA));

    constexpr double earthRadius = 3440.; // Nautical miles

    double dist = earthRadius * termC;

    return dist;
}
