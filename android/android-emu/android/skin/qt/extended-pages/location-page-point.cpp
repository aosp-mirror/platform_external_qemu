// Copyright (C) 2018 The Android Open Source Project
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

#ifdef USE_WEBENGINE // This entire file is stubbed out if we don't have WebEngine

#include "android/base/files/PathUtils.h"
#include "android/base/StringView.h"
#include "android/emulation/ConfigDirs.h"
#include "android/location/Point.h"
#include "android/metrics/MetricsReporter.h"
#include "studio_stats.pb.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/logging-category.h"
#include "android/utils/path.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMovie>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSettings>
#include <QWebEngineScriptCollection>

#include <fstream>

static const char PROTO_FILE_NAME[] = "point_metadata.pb";

namespace {
using PointListElement = LocationPage::PointListElement;

class PointItemBuilder {
public:
    PointItemBuilder(QListWidget* listWidget) :
        mListWidget(listWidget)
    {
    }

    void addPoint(PointListElement&& p, LocationPage* const locationPage) {
        PointWidgetItem* pointWidgetItem = new PointWidgetItem(std::move(p), mListWidget);
        locationPage->connect(pointWidgetItem,
                SIGNAL(editButtonClickedSignal(CCListItem*)), locationPage,
                SLOT(pointWidget_editButtonClicked(CCListItem*)));
        mListWidget->setCurrentItem(pointWidgetItem->listWidgetItem());
    }

private:
    QListWidget* mListWidget = nullptr;
};

PointWidgetItem* getItemWidget(QListWidget* list,
                               QListWidgetItem* listItem) {
    return qobject_cast<PointWidgetItem*>(list->itemWidget(listItem));
}

bool addJavascriptFromResource(QWebEnginePage* const webEnginePage,
                               QString qrcFile,
                               QString scriptName,
                               const QString& appendString) {
    QFile jsFile(qrcFile);
    QWebEngineScript script;
    if(!jsFile.open(QIODevice::ReadOnly)) {
        qCDebug(emu) << QString("Couldn't open %1: %2").arg(qrcFile).arg(jsFile.errorString());
        return false;
    }

    QByteArray jscode = jsFile.readAll();
    jscode.append(appendString);
    script.setSourceCode(jscode);
    script.setName(scriptName);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setRunsOnSubFrames(false);
    webEnginePage->scripts().insert(script);
    return true;
}
}  // namespace

void MapBridge::map_savePoint() {
    mLocationPage->map_savePoint();
}

void MapBridge::sendLocation(const QString& lat, const QString& lng, const QString& address) {
    mLocationPage->sendLocation(lat, lng, address);
}

// Invoked when the user saves a point on the map
void LocationPage::map_savePoint() {
    savePoint(mLastLat.toDouble(), mLastLng.toDouble(), 0.0, mLastAddr);
}

void LocationPage::savePoint(double lat, double lng, double elevation, const QString& addr) {
    QDateTime now = QDateTime::currentDateTime();
    QString pointName("point_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::PointMetadata ptMetadata;
    QString logicalName = pointName;
    ptMetadata.set_logical_name(logicalName.toStdString());
    ptMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    ptMetadata.set_latitude(lat);
    ptMetadata.set_longitude(lng);
    ptMetadata.set_altitude(elevation);
    ptMetadata.set_address(addr.toStdString());

    std::string fullPath = writePointProtobufByName(pointName, ptMetadata);

    // Add the new point to the list
    PointListElement listElement;
    listElement.protoFilePath = QString::fromStdString(fullPath);
    listElement.logicalName = QString::fromStdString(ptMetadata.logical_name());
    listElement.description = QString::fromStdString(ptMetadata.description());
    listElement.latitude = ptMetadata.latitude();
    listElement.longitude = ptMetadata.longitude();
    listElement.address = QString::fromStdString(ptMetadata.address());

    // Open a dialog to let the user change the name if they want.
    editPoint(listElement, true);

    PointItemBuilder builder(mUi->loc_pointList);
    builder.addPoint(std::move(listElement), this);

    mUi->loc_noSavedPoints_mask->setVisible(false);
}

// Invoked when the user clicks on the map
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) {
    mLastLat = lat;
    mLastLng = lng;
    mLastAddr = address;
    validateCoordinates();
    // Make nothing selected now
    mUi->loc_pointList->setCurrentItem(nullptr);
}

void LocationPage::on_loc_singlePoint_setLocationButton_clicked() {
    switch (mPointState) {
    case UiState::Default:
        sendMostRecentUiLocation();
        ++mSetLocCount;
        mUi->loc_pointList->setCurrentItem(nullptr);
        mUi->loc_singlePoint_setLocationButton->setEnabled(false);
        break;
    case UiState::Deletion:
        deleteSelectedPoints();
        break;
    }
}

void LocationPage::deleteSelectedPoints() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QMessageBox msgBox(QMessageBox::Warning,
                       tr("Delete selected points"),
                       tr("Do you want to permanently delete the selected points?"),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        mPrevSelectedPoints.clear();
        QList<QListWidgetItem*> selectedItems =
                mUi->loc_pointList->selectedItems();
        for (int i = 0; i < selectedItems.size(); ++i) {
            auto* pointItem = getItemWidget(mUi->loc_pointList,
                                            selectedItems[i]);
            deletePoint(pointItem);
        }
        emit mMapBridge->resetPointsMap();
        mUi->loc_singlePoint_setLocationButton->setEnabled(false);
    }
    QApplication::restoreOverrideCursor();
}

// Populate the QListWidget with the points that are found on disk
void LocationPage::scanForPoints() {
    mUi->loc_pointList->clear();
    // Disable sorting while we're updating the table
    mUi->loc_pointList->setSortingEnabled(false);
    // Block currentItemChange signals until items are added
    mUi->loc_pointList->blockSignals(true);

    // Get the directory
    std::string pointDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "points");

    if (!path_is_dir(pointDirectoryName.c_str())) {
        // No "locations/points" directory
        return;
    }

    // Find all the subdirectories in the locations directory
    QDir pointDir(pointDirectoryName.c_str());
    pointDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList pointList(pointDir.entryList());

    PointItemBuilder builder(mUi->loc_pointList);
    // Look at all the directories and create an entry for each valid point
    for (const QString& pointName : pointList) {
        // Read the point protobuf
        std::string protoFilePath = android::base::PathUtils::
                join(pointDirectoryName.c_str(),
                     pointName.toStdString().c_str(),
                     PROTO_FILE_NAME);

        android::location::Point point(protoFilePath.c_str());

        const emulator_location::PointMetadata* pointMetadata = point.getProtoInfo();

        if (pointMetadata == nullptr) continue;

        PointListElement listElement;
        listElement.protoFilePath = QString::fromStdString(protoFilePath);
        listElement.logicalName = QString::fromStdString(pointMetadata->logical_name());
        listElement.description = QString::fromStdString(pointMetadata->description());
        listElement.latitude = pointMetadata->latitude();
        listElement.longitude = pointMetadata->longitude();
        listElement.address = QString::fromStdString(pointMetadata->address());

        builder.addPoint(std::move(listElement), this);
    }

    // All done updating. Enable sorting now.
    mUi->loc_pointList->setSortingEnabled(true);
    mUi->loc_pointList->setCurrentItem(nullptr);
    mUi->loc_pointList->blockSignals(false);
    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedPoints_mask->setVisible(mUi->loc_pointList->count() == 0);
}

void LocationPage::updatePointWidgetItemsColor() {
    // Deselect everything selected previously
    for (auto* item : mPrevSelectedPoints) {
        auto* pointWidgetItem = getItemWidget(mUi->loc_pointList, item);
        if (pointWidgetItem != nullptr) {
            pointWidgetItem->setSelected(false);
        }
    }
    // Select everything in the selected items list
    for (auto* item : mUi->loc_pointList->selectedItems()) {
        auto* pointWidgetItem = getItemWidget(mUi->loc_pointList, item);
        if (pointWidgetItem != nullptr) {
            pointWidgetItem->setSelected(true);
        }
    }
    mPrevSelectedPoints = mUi->loc_pointList->selectedItems();
}

void LocationPage::on_loc_pointList_itemSelectionChanged() {
    UiState newState = mUi->loc_pointList->selectedItems().size() > 1 ?
                       UiState::Deletion :
                       UiState::Default;
    updatePointWidgetItemsColor();

    if (mPointState == UiState::Deletion &&
        mUi->loc_pointList->selectedItems().size() != 1) {
        // Either no points selected, or in deletion mode, either of which
        // means the map shouldn't be set to anything.
        emit mMapBridge->resetPointsMap();
    }
    if (mUi->loc_pointList->selectedItems().size() == 1) {
        // show the location on the map, but do not send it to the device
        auto* item = getItemWidget(mUi->loc_pointList,
                                   mUi->loc_pointList->selectedItems()[0]);
        auto& pointElement = item->pointElement();
        mLastLat = QString::number(pointElement.latitude, 'g', 12);
        mLastLng = QString::number(pointElement.longitude, 'g', 12);
        mLastAddr = pointElement.address;
        emit mMapBridge->showLocation(mLastLat, mLastLng, mLastAddr);
        validateCoordinates();
    }

    // Disable the edit buttons on the saved points if in deletion mode
    for (int i = 0; i < mUi->loc_pointList->count(); ++i) {
        auto item = mUi->loc_pointList->item(i);
        auto* pointWidgetItem = getItemWidget(mUi->loc_pointList, item);
        if (pointWidgetItem != nullptr) {
            pointWidgetItem->setEditButtonEnabled(newState == UiState::Default);
        }
    }
    mUi->loc_singlePoint_setLocationButton->setText(
            newState == UiState::Default ?
                      tr("SET LOCATION") :
                      tr("DELETE ITEMS"));
    mPointState = newState;
}

void LocationPage::pointWidget_editButtonClicked(CCListItem* listItem) {
    mUi->loc_pointList->blockSignals(true);
    auto* pointWidgetItem = reinterpret_cast<PointWidgetItem*>(listItem);
    QMenu* popMenu = new QMenu(this);
    stylePopupMenu(popMenu);
    QAction* startRouteAction = popMenu->addAction(tr("Start a route"));
    QAction* editAction   = popMenu->addAction(tr("Edit"));
    QAction* deleteAction = popMenu->addAction(tr("Delete"));

    mUi->loc_pointList->setCurrentItem(pointWidgetItem->listWidgetItem());
    QAction* theAction = popMenu->exec(QCursor::pos());
    if (theAction == startRouteAction) {
        // Switch to routes tab with the saved point
        auto& pointElement = pointWidgetItem->pointElement();
        emit mMapBridge->startRouteCreatorFromPoint(QString::number(pointElement.latitude, 'g', 12),
                                                    QString::number(pointElement.longitude, 'g', 12),
                                                    pointElement.address);
        mUi->locationTabs->setCurrentIndex(1);
    } else if (theAction == editAction && editPoint(pointWidgetItem->pointElement())) {
        pointWidgetItem->refresh();
        auto& pointElement = pointWidgetItem->pointElement();
        // show the location on the map, but do not send it to the device
        emit mMapBridge->showLocation(QString::number(pointElement.latitude, 'g', 12),
                                      QString::number(pointElement.longitude, 'g', 12),
                                      pointElement.address);
    } else if (theAction == deleteAction) {
        auto& pointElement = pointWidgetItem->pointElement();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QMessageBox msgBox(QMessageBox::Warning,
                           tr("Delete point"),
                           tr("Do you want to permanently delete<br>point \"%1\"?")
                                   .arg(pointElement.logicalName),
                           QMessageBox::Cancel,
                           this);
        QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
        deleteButton->setText(tr("Delete"));

        int selection = msgBox.exec();

        if (selection == QMessageBox::Ok) {
            mPrevSelectedPoints.clear();
            deletePoint(pointWidgetItem);
        }
        emit mMapBridge->resetPointsMap();
        QApplication::restoreOverrideCursor();
    }
    mUi->loc_pointList->blockSignals(false);
}

bool LocationPage::editPoint(PointListElement& pointElement, bool isNewPoint) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->setAlignment(Qt::AlignTop);

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = pointElement.logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Address
    dialogLayout->addWidget(new QLabel(tr("Address")));
    QLabel* addrField = new QLabel(this);
    addrField->setText(pointElement.address);
    addrField->setProperty("ColorGroup", "Property");
    dialogLayout->addWidget(addrField);

    // Latitude
    dialogLayout->addWidget(new QLabel(tr("Latitude")));
    QLabel* latitudeField = new QLabel(this);
    latitudeField->setText(QString::number(pointElement.latitude, 'g', 12));
    latitudeField->setProperty("ColorGroup", "Property");
    dialogLayout->addWidget(latitudeField);

    // Longitude
    dialogLayout->addWidget(new QLabel(tr("Longitude")));
    QLabel* longitudeField = new QLabel(this);
    longitudeField->setText(QString::number(pointElement.longitude, 'g', 12));
    longitudeField->setProperty("ColorGroup", "Property");
    dialogLayout->addWidget(longitudeField);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = pointElement.description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    // OK and Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
            isNewPoint ? QDialogButtonBox::Ok :
                         QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                         Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(isNewPoint ?
            tr("Save route as") : tr("Edit point"));
    editDialog.setLayout(dialogLayout);

    QApplication::restoreOverrideCursor();

    int selection = editDialog.exec();

    if (selection == QDialog::Rejected) {
        return false;
    }

    QString newName = nameEdit->text();
    QString newDescription = descriptionEdit->toPlainText();

    if ((!newName.isEmpty() && newName != oldName) ||
        newDescription != oldDescription             )
    {
        // Something changed. Read the existing Protobuf,
        // update it, and write it back out.
        QApplication::setOverrideCursor(Qt::WaitCursor);

        android::location::Point point(pointElement.protoFilePath.toStdString().c_str());

        const emulator_location::PointMetadata* oldPointMetadata = point.getProtoInfo();
        if (oldPointMetadata == nullptr) return false;

        emulator_location::PointMetadata pointMetadata(*oldPointMetadata);

        if (!newName.isEmpty()) {
            pointMetadata.set_logical_name(newName.toStdString().c_str());
            pointElement.logicalName = newName;
        }
        pointMetadata.set_description(newDescription.toStdString().c_str());
        pointElement.description = newDescription;

        mLastLat = QString::number(pointElement.latitude, 'g', 12);
        mLastLng = QString::number(pointElement.longitude, 'g', 12);
        validateCoordinates();

        writePointProtobufFullPath(pointElement.protoFilePath, pointMetadata);
        QApplication::restoreOverrideCursor();
    }

    return true;
}

void LocationPage::deletePoint(PointWidgetItem* item) {
    auto& pointElement = item->pointElement();
    std::string protobufName = pointElement.protoFilePath.toStdString();
    android::base::StringView dirName;
    bool haveDirName = android::base::PathUtils::split(protobufName,
                                                       &dirName,
                                                       nullptr /* base name */);
    if (haveDirName) {
        path_delete_dir(dirName.str().c_str());
    }

    mUi->loc_pointList->setCurrentItem(nullptr);
    item->removeFromListWidget();
    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedPoints_mask->setVisible(mUi->loc_pointList->count() == 0);
}

// Write a protobuf into the specified directory.
// This code determines the parent directory. The
// full path of output file is returned.
std::string LocationPage::writePointProtobufByName(
        const QString& pointFormalName,
        const emulator_location::PointMetadata& protobuf)
{
    // Get the directory to hold the protobuf
    std::string protoFileDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "points",
                 pointFormalName.toStdString().c_str());

    // Ensure that the directory exists
    if (path_mkdir_if_needed(protoFileDirectoryName.c_str(), 0755)) {
        // Fail
        return std::string();
    }

    // The path exists now.
    // Construct the full file path
    std::string protoFilePath = android::base::PathUtils::
            join(protoFileDirectoryName.c_str(), PROTO_FILE_NAME);

    writePointProtobufFullPath(protoFilePath.c_str(), protobuf);
    return protoFilePath;
}

void LocationPage::writePointProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::PointMetadata& protobuf)
{
    std::ofstream outStream(protoFullPath.toStdString().c_str(), std::ofstream::binary);
    protobuf.SerializeToOstream(&outStream);
}

void LocationPage::setUpWebEngine() {
    if (mMapsApiKey.isEmpty()) {
        fallbackToOfflineUi();
        return;
    }

    double initialLat, initialLng, unusedAlt, unusedVelocity, unusedHeading;
    mMapBridge.reset(new MapBridge(this));
    getDeviceLocation(&initialLat, &initialLng, &unusedAlt, &unusedVelocity, &unusedHeading);
    mUi->loc_singlePoint_setLocationButton->setEnabled(false);
    mUi->loc_pointList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mUi->loc_routeList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QObject::connect(this, &LocationPage::signal_saveRoute,
                     this, &LocationPage::map_saveRoute,
                     Qt::QueuedConnection);

    mServer.reset(new QWebSocketServer(QStringLiteral("QWebChannel Standalone Example Server"),
                        QWebSocketServer::NonSecureMode));
    if (!mServer->listen(QHostAddress::LocalHost)) {
        // TODO: error handling needed
    }

    // wrap WebSocket clients in QWebChannelAbstractTransport objects
    mClientWrapper.reset(new WebSocketClientWrapper(mServer.get()));

    // setup the channel
    mWebChannel.reset(new QWebChannel());
    QObject::connect(mClientWrapper.get(), &WebSocketClientWrapper::clientConnected,
                     mWebChannel.get(), &QWebChannel::connectTo);

    // setup the dialog and publish it to the QWebChannel
    mWebChannel->registerObject(QStringLiteral("emulocationserver"), mMapBridge.get());


    // Set up two web engines: one for the Points page and one for the Routes page
    for (int webEnginePageIdx = 0; webEnginePageIdx < 2; webEnginePageIdx++) {
        bool isPoint = (webEnginePageIdx == 0);
        QWebEnginePage* webEnginePage = isPoint ? mUi->loc_pointWebEngineView->page()
                                                : mUi->loc_routeWebEngineView->page();

        connect(webEnginePage, SIGNAL(loadFinished(bool)), this, SLOT(onWebPageLoadFinished(bool)));

        QString appendString;

        // Send the current location to each page
        appendString.append(
                "\n"
                "var initialLat = '");
        appendString.append(QString::number(initialLat, 'g', 12));
        appendString.append(
                "'; var initialLng = '");
        appendString.append(QString::number(initialLng, 'g', 12));
        appendString.append(
                "';\n"
                "var wsUri = 'ws://localhost:");
        appendString.append(QString::number(mServer->serverPort()));
        appendString.append(
                "';"
                "var socket = new WebSocket(wsUri);"
                "socket.onclose = function() {"
                    "console.error('web channel closed');"
                "};"
                "socket.onerror = function(error) {"
                    "console.error('web channel error: ' + error);"
                "};"
                "socket.onopen = function() {"
                    "window.channel = new QWebChannel(socket, function(channel) {"
                        "channel.objects.emulocationserver.locationChanged.connect(function(lat, lng) {"
                            "if (setDeviceLocation) setDeviceLocation(lat, lng);"
                        "});");
        if (isPoint) {
            // Define Points-specific interfaces
            appendString.append(
                        "channel.objects.emulocationserver.showLocation.connect(function(lat, lng, addr) {"
                            "if (showPendingLocation) showPendingLocation(lat, lng, addr);"
                        "});");
            appendString.append(
                        "channel.objects.emulocationserver.resetPointsMap.connect(function() {"
                            "if (resetPointsMap) resetPointsMap();"
                        "});");
        } else {
            // Define Routes-specific interfaces
            appendString.append(
                        "channel.objects.emulocationserver.showRouteOnMap.connect(function(routeJson, isSavedRoute) {"
                            "if (setRouteOnMap) setRouteOnMap(routeJson, isSavedRoute);"
                        "});");
            appendString.append(
                        "channel.objects.emulocationserver.showRoutePlaybackOverlay.connect(function(visible) {"
                            "if (showRoutePlaybackOverlay) showRoutePlaybackOverlay(visible);"
                        "});");
            appendString.append(
                        "channel.objects.emulocationserver.startRouteCreatorFromPoint.connect(function(lat, lng, addr) {"
                            "if (startRouteCreatorFromPoint) startRouteCreatorFromPoint(lat, lng, addr);"
                        "});");
            appendString.append(
                        "channel.objects.emulocationserver.showGpxKmlRouteOnMap.connect(function(routeJson, title, subtitle) {"
                            "if (showGpxKmlRouteOnMap) showGpxKmlRouteOnMap(routeJson, title, subtitle);"
                        "});");
        }
        appendString.append(
                    "});"
                    "if (typeof setDeviceLocation != 'undefined' && setDeviceLocation != null) {"
                        "setDeviceLocation(initialLat, initialLng);"
                    "}"
                "}");

        if (!addJavascriptFromResource(webEnginePage,
                                       ":/html/js/qwebchannel.js",
                                       "qwebchannel.js",
                                       appendString)) {
            continue;
        }

        // Get the HTML file (either 'location-point.html' or 'location-route.html')
        QFile indexHtmlFile(isPoint ? ":/html/location-point.html" : ":/html/location-route.html");

        // Insert the Maps API key into the HTML and publish
        // that to QWebChannel
        if (indexHtmlFile.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray htmlByteArray = indexHtmlFile.readAll();
            if (!htmlByteArray.isEmpty()) {
                // Change the placeholder to the real Maps API key
                htmlByteArray.replace("YOUR_API_KEY", mMapsApiKey.toUtf8());
                // Publish it
                webEnginePage->setHtml(htmlByteArray, QUrl(isPoint ? "qrc:/html/location-point.html" : "qrc:/html/location-route.html"));
            }
        }
    }
}

void LocationPage::fallbackToOfflineUi() {
    mUi->locationTabs->clear();
    mUi->locationTabs->addTab(mOfflineTab, "");
    mUi->loc_latitudeInput->setMinValue(-90.0);
    mUi->loc_latitudeInput->setMaxValue(90.0);
    mTimer.setSingleShot(true);
    QObject::disconnect(&mTimer, &QTimer::timeout, 0, 0);
    QObject::connect(&mTimer, &QTimer::timeout, this, &LocationPage::timeout);
    QObject::connect(this, &LocationPage::locationUpdateRequired,
                     this, &LocationPage::updateDisplayedLocation);
    QObject::connect(this, &LocationPage::populateNextGeoDataChunk,
                     this, &LocationPage::populateTableByChunks,
                     Qt::QueuedConnection);

    setButtonEnabled(mUi->loc_playStopButton, getSelectedTheme(), false);

    // Set the GUI to the current values
    double curLat, curLon, curAlt, curVelocity, curHeading;
    getDeviceLocation(&curLat, &curLon, &curAlt, &curVelocity, &curHeading);
    updateDisplayedLocation(curLat, curLon, curAlt, curVelocity, curHeading);

    mUi->loc_altitudeInput->setText(QString::number(curAlt, 'f', 1));
    mUi->loc_speedInput->setText(QString::number(curVelocity, 'f', 1));
    mUi->loc_latitudeInput->setValue(curLat);
    mUi->loc_longitudeInput->setValue(curLon);

    QSettings settings;
    mUi->loc_playbackSpeed->setCurrentIndex(
            getLocationPlaybackSpeedFromSettings());
    mUi->loc_pathTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QString location_data_file =
        getLocationPlaybackFilePathFromSettings();
    mGeoDataLoader.reset(GeoDataLoaderThread::newInstance(
            this,
            SLOT(geoDataThreadStarted()),
            SLOT(startupGeoDataThreadFinished(QString, bool, QString))));
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

void LocationPage::points_updateTheme() {
    for (int i = 0; i < mUi->loc_pointList->count(); ++i) {
        PointWidgetItem* item = getItemWidget(mUi->loc_pointList, mUi->loc_pointList->item(i));
        item->refresh();
    }
    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner_transparent");
    if (movie->isValid()) {
        movie->start();
        mUi->loc_overlaySpinner->setMovie(movie);
    }
}

void LocationPage::setLoadingOverlayVisible(bool visible, QString text) {
    mUi->loc_loadingOverlay->setVisible(visible);
    mUi->loc_loadingOverlayLabel->setText(text);
}

void LocationPage::handle_importGpxKmlButton_clicked() {
    // Use dialog to get file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open GPX or KML File"), ".",
                                                    tr("GPX and KML files (*.gpx *.kml)"));

    if (fileName.isNull()) return;

    // Asynchronously parse the file with geo data.
    // If the file is big enough, parsing it synchronously will cause a noticeable
    // hiccup in the UI.
    setLoadingOverlayVisible(true, tr("Importing GPX/KML file..."));
    mGeoDataLoader.reset(GeoDataLoaderThread::newInstance(this,
                                                      SLOT(geoDataThreadStarted_v2()),
                                                      SLOT(geoDataThreadFinished_v2(QString, bool, QString))));
    mGeoDataLoader->loadGeoDataFromFile(fileName, &mGpsFixesArray);
}

// Called when closing emulator
void LocationPage::sendMetrics() {
    android_studio::EmulatorLocationV2 metrics;
    metrics.set_set_loc_count(mSetLocCount);
    metrics.set_play_route_count(mPlayRouteCount);
    android::metrics::MetricsReporter::get().report(
            [metrics](android_studio::AndroidStudioEvent* event) {
                event->mutable_emulator_details()
                        ->mutable_location_v2()
                        ->CopyFrom(metrics);
            });
}

void RouteSenderThread::sendRouteToMap(const LocationPage::RouteListElement* const routeElement,
                                       MapBridge* mapBridge) {
    mRouteElement = routeElement;
    mMapBridge = mapBridge;
    start();
}

void RouteSenderThread::run() {
    bool ok = true;

    const QString& routeJson = LocationPage::readRouteJsonFile(mRouteElement->protoFilePath);
    if (routeJson.length() <= 0) {
        ok = false;
    } else {
        switch (mRouteElement->jsonFormat) {
        case emulator_location::RouteMetadata_JsonFormat_GOOGLEMAPS:
            emit mMapBridge->showRouteOnMap(routeJson.toStdString().c_str());
            break;
        case emulator_location::RouteMetadata_JsonFormat_GPXKML:
            emit mMapBridge->showGpxKmlRouteOnMap(routeJson.toStdString().c_str(),
                                                  mRouteElement->logicalName,
                                                  "Route from GPX/KML file");
            break;
        default:
            LOG(WARNING) << "Route JsonFormat(" << mRouteElement->jsonFormat << ") is unknown. Can't display on map.";
            ok = false;
            break;
        }
    }

    emit(sendFinished(ok));
}

RouteSenderThread* RouteSenderThread::newInstance(const QObject* handler,
                                                  const char* finished_slot) {
    RouteSenderThread* new_instance = new RouteSenderThread();
    connect(new_instance, SIGNAL(sendFinished(bool)), handler, finished_slot);

    // Make sure new_instance gets cleaned up after the thread exits.
    connect(new_instance, &QThread::finished, new_instance, &QObject::deleteLater);

    return new_instance;
}

void LocationPage::keyPressEvent(QKeyEvent* e) {
    QWidget::keyPressEvent(e);

    switch (e->key()) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        switch (mUi->locationTabs->currentIndex()) {
        case 0:  // Single point tab
            if (mPointState == UiState::Deletion) {
                on_loc_singlePoint_setLocationButton_clicked();
            }
            break;
        case 1:  // Routes tab
            if (mRouteState == UiState::Deletion) {
                on_loc_playRouteButton_clicked();
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

#else // !USE_WEBENGINE  These are the stubs for when we don't have WebEngine
void MapBridge::map_savePoint() { }
void MapBridge::sendLocation(const QString& lat, const QString& lng, const QString& address) { }
void LocationPage::map_savePoint() { }
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) { }
void LocationPage::on_loc_singlePoint_setLocationButton_clicked() { }
void LocationPage::scanForPoints() { }
void LocationPage::on_loc_pointList_itemSelectionChanged() { }
bool LocationPage::editPoint(PointListElement& pointElement, bool isNewPoint) { return true; }
void LocationPage::deletePoint(PointWidgetItem* item) { }
std::string LocationPage::writePointProtobufByName(
        const QString& pointFormalName,
        const emulator_location::PointMetadata& protobuf) { return ""; }
void LocationPage::writePointProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::PointMetadata& protobuf) { }
void LocationPage::setUpWebEngine() { }
void LocationPage::fallbackToOfflineUi() { }
void LocationPage::pointWidget_editButtonClicked(CCListItem* listItem) { }
void LocationPage::points_updateTheme() { }
void LocationPage::handle_importGpxKmlButton_clicked() { }
void LocationPage::routes_updateTheme() { }
void LocationPage::geoDataThreadStarted_v2() { }
void LocationPage::geoDataThreadFinished_v2(QString file_name, bool ok, QString error_message) { }
void LocationPage::finishGeoDataLoading_v2(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error) { }
void LocationPage::setLoadingOverlayVisible(bool visible, QString text) { }
void RouteSenderThread::sendRouteToMap(const LocationPage::RouteListElement* const routeElement,
                                       MapBridge* mapBridge) { }
void RouteSenderThread::run() { }
RouteSenderThread* RouteSenderThread::newInstance(const QObject* handler,
                                                  const char* finished_slot) { return nullptr; }
void LocationPage::routeSendingFinished(bool ok) { }
void LocationPage::sendMetrics() { }
void LocationPage::deleteSelectedPoints() { }
void LocationPage::updatePointWidgetItemsColor() { }
#endif // !USE_WEBENGINE
