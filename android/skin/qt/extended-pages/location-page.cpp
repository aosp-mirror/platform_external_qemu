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

#include "android/emulation/control/location_agent.h"
#include "android/gps/GpxParser.h"
#include "android/gps/KmlParser.h"
#include "android/settings-agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include <QFileDialog>
#include <QSettings>
#include <unistd.h>

LocationPage::LocationPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::LocationPage),
    mNowLoadingGeoData(false),
    mGeoDataLoadingStopRequested(false)
{
    mUi->setupUi(this);
    mNowPlaying = false;
    mTimer.setSingleShot(true);
    mUi->loc_altitudeInput->setValidator(&mAltitudeValidator);
    mUi->loc_latitudeInput->setMinValue(-90.0);
    mUi->loc_latitudeInput->setMaxValue(90.0);
    QObject::connect(&mTimer, &QTimer::timeout, this, &LocationPage::timeout);
    setButtonEnabled(mUi->loc_playStopButton, getSelectedTheme(), false);

    // Restore previous values.
    QSettings settings;
    mUi->loc_altitudeInput->setText(
            settings.value(Ui::Settings::LOCATION_ALTITUDE, "0.0").toString());
    mUi->loc_longitudeInput->setValue(
            settings.value(Ui::Settings::LOCATION_LONGITUDE, 0.0).toDouble());
    mUi->loc_latitudeInput->setValue(
            settings.value(Ui::Settings::LOCATION_LATITUDE, 0.0).toDouble());
    mUi->loc_playbackSpeed->setCurrentIndex(
            settings.value(Ui::Settings::LOCATION_PLAYBACK_SPEED, 0).toInt());
    QString location_data_file =
        settings.value(Ui::Settings::LOCATION_PLAYBACK_FILE, "").toString();
    mGeoDataLoader = GeoDataLoaderThread::newInstance(
            this,
            SLOT(geoDataThreadStarted()),
            SLOT(startupGeoDataThreadFinished(QString, bool, QString)));
    mGeoDataLoader->loadGeoDataFromFile(location_data_file, &mGpsFixesArray);

}

LocationPage::~LocationPage() {
    if (mGeoDataLoader != nullptr) {
        // If there's a loader thread running in the background,
        // ignore all signals from it and wait for it to finish.
        mGeoDataLoader->blockSignals(true);
        mGeoDataLoader->wait();
    }
}

void LocationPage::setLocationAgent(const QAndroidLocationAgent* agent) {
    mLocationAgent = agent;
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
    bool cellOK = validateCell(mUi->loc_pathTable, row, col);
    QColor normalColor =
        getSelectedTheme() == SETTINGS_THEME_LIGHT ? Qt::black : Qt::white;
    QColor newColor = (cellOK ? normalColor : Qt::red);
    mUi->loc_pathTable->item(row, col)->setForeground(QBrush(newColor));
}

void LocationPage::on_loc_playStopButton_clicked() {
    if (mNowPlaying) {
        locationPlaybackStop();
    } else {
        locationPlaybackStart();
    }
}

void LocationPage::on_loc_decimalModeSwitch_toggled(bool checked) {
    if (checked) {
        mUi->loc_latitudeInput->setInputMode(AngleInputWidget::InputMode::Decimal);
        mUi->loc_longitudeInput->setInputMode(AngleInputWidget::InputMode::Decimal);
    }
}

void LocationPage::on_loc_sexagesimalModeSwitch_toggled(bool checked) {
    if (checked) {
        mUi->loc_latitudeInput->setInputMode(AngleInputWidget::InputMode::Sexagesimal);
        mUi->loc_longitudeInput->setInputMode(AngleInputWidget::InputMode::Sexagesimal);
    }
}

void LocationPage::on_loc_sendPointButton_clicked() {
    if (mLocationAgent == nullptr || mLocationAgent->gpsCmd == nullptr) {
        return;
    }
    timeval timeVal = {};
    gettimeofday(&timeVal, nullptr);
    mLocationAgent->gpsCmd(mUi->loc_latitudeInput->value(),
                           mUi->loc_longitudeInput->value(),
                           mUi->loc_altitudeInput->text().toDouble(),
                           4,
                           &timeVal);
}

void LocationPage::on_loc_longitudeInput_valueChanged(double value) {
    QSettings settings;
    settings.setValue(Ui::Settings::LOCATION_LONGITUDE, value);
}

void LocationPage::on_loc_latitudeInput_valueChanged(double value) {
    QSettings settings;
    settings.setValue(Ui::Settings::LOCATION_LATITUDE, value);
}

void LocationPage::on_loc_altitudeInput_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::LOCATION_ALTITUDE, mUi->loc_altitudeInput->text());
}

void LocationPage::on_loc_playbackSpeed_currentIndexChanged(int index) {
    QSettings settings;
    settings.setValue(Ui::Settings::LOCATION_PLAYBACK_SPEED, index);
}

bool LocationPage::validateCell(QTableWidget *table, int row, int col)
{

    QTableWidgetItem *theItem;
    double cellValue;
    bool cellOK;

    // The entry is a number. Check its range.
    switch (col) {
        case 0: // Delay
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            // Except for the first entry, the delay must be >= 1 millisecond
            cellOK &= (row == 0  ||  cellValue >= 0.001);
            break;
        case 1: // Latitude
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK &= (cellValue >=   -90.0  &&  cellValue <=    90.0);
            break;
        case 2: // Longitude
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK = (cellValue >=  -180.0  &&  cellValue <=   180.0);
            break;
        case 3: // Elevation
            theItem = table->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK = (cellValue >= -1000.0  &&  cellValue <= 10000.0);
            break;
        default:
            // Name, description: Anything is OK
            cellOK = true;
            break;
    }
    return cellOK;
}


static QTableWidgetItem* itemForTable(const QString& text) {
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

void LocationPage::populateTable(GpsFixArray* fixes)
{
    // Delete all rows in table
    mUi->loc_pathTable->setRowCount(0);

    // Special case, the first row will have delay 0

    time_t previousTime = fixes->at(0).time;
    mUi->loc_pathTable->setRowCount(fixes->size());
    mUi->loc_pathTable->blockSignals(true);
    for (unsigned i = 0; !mGeoDataLoadingStopRequested && i < fixes->size(); i++) {
        GpsFix &fix = fixes->at(i);
        time_t delay = fix.time - previousTime; // In seconds

        // Ensure all other delays are > 0, even if multiple points have the same timestamp
        if (delay == 0 && i != 0) {
            delay = 2;
        }

        mUi->loc_pathTable->setItem(i, 0, itemForTable(QString::number(delay)));
        mUi->loc_pathTable->setItem(i, 1, itemForTable(QString::fromStdString(fix.latitude)));
        mUi->loc_pathTable->setItem(i, 2, itemForTable(QString::fromStdString(fix.longitude)));
        mUi->loc_pathTable->setItem(i, 3, itemForTable(QString::fromStdString(fix.elevation)));
        mUi->loc_pathTable->setItem(i, 4, itemForTable(QString::fromStdString(fix.name)));
        mUi->loc_pathTable->setItem(i, 5, itemForTable(QString::fromStdString(fix.description)));

        // If the fixes array contains a lot of elements, this loop can cause
        // a lag in the UI. Just make sure we let the application handle UI 
        // events for every few rows we add.
        if (i % 100 == 0) {
            qApp->processEvents();
        }
        previousTime = fix.time;
    }
    mUi->loc_pathTable->blockSignals(false);
    setButtonEnabled(mUi->loc_playStopButton, getSelectedTheme(), true);
}

void LocationPage::locationPlaybackStart()
{
    if (mUi->loc_pathTable->rowCount() <= 0) {
        return;
    }

    // Validate all the values in the table.
    for (int row = 0; row < mUi->loc_pathTable->rowCount(); row++) {
        for (int col = 0; col < mUi->loc_pathTable->columnCount(); col++) {
            if (!validateCell(mUi->loc_pathTable, row, col)) {
                showErrorDialog(tr("The table contains errors.<br>No locations were sent."),
                                tr("GPS Playback"));
                mUi->loc_pathTable->scrollToItem(mUi->loc_pathTable->item(row, 0));
                return;
            }
        }
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

    // Get the data for the point we're about to send.
    theItem = mUi->loc_pathTable->item(mRowToSend, 1);
    double lat = theItem->text().toDouble(&cellOK);
    theItem = mUi->loc_pathTable->item(mRowToSend, 2);
    double lon = theItem->text().toDouble(&cellOK);
    theItem = mUi->loc_pathTable->item(mRowToSend, 3);
    double alt = theItem->text().toDouble(&cellOK);
    int nSats = 4;  // Just say that we see 4 GPS satellites
    timeval timeVal = {};
    gettimeofday(&timeVal, nullptr);

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

    // Send the command.
    if (mLocationAgent  && mLocationAgent->gpsCmd) {
        mLocationAgent->gpsCmd(lat, lon, alt, nSats, &timeVal);
    }

    // Go on to the next row
    mRowToSend++;
    if (mRowToSend >= mUi->loc_pathTable->rowCount()) {
        // No more to send. Same as clicking Stop.
        locationPlaybackStop();
    } else {
        // Set a timer for when this row should be sent
        theItem = mUi->loc_pathTable->item(mRowToSend, 0);
        double dTime = theItem->text().toDouble();
        int mSec = dTime * 1000.0;
        if (mSec < 0) mSec = 0;
        mTimer.setInterval(
                mSec / static_cast<double>(mUi->loc_playbackSpeed->currentIndex() + 1));
        mTimer.start();
    }
}

void LocationPage::geoDataThreadStarted() {
    mUi->loc_pathTable->setRowCount(0);
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
    if (ok) {
        QSettings settings;
        settings.setValue(Ui::Settings::LOCATION_PLAYBACK_FILE, file_name);
        populateTable(&mGpsFixesArray);
    } else if (!ignore_error) {
        showErrorDialog(error_message, tr("Geo Data Parser"));
    }
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->loc_GpxKmlButton, theme, true);
    setButtonEnabled(mUi->loc_playStopButton, theme, true);
    mNowLoadingGeoData = false;
    emit(geoDataLoadingFinished());
   
}

void LocationPage::startupGeoDataThreadFinished(QString file_name, bool ok, QString error_message) {
    // on startup, we silently ignore the previously remebered geo data file being
    // missing or malformed.
    finishGeoDataLoading(file_name, ok, error_message, true);
}

void LocationPage::geoDataThreadFinished(QString file_name, bool ok, QString error_message) {
    finishGeoDataLoading(file_name, ok, error_message, false);
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
