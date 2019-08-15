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
#include "android/skin/qt/stylesheet.h"
#include "android/utils/path.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
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
        qDebug() << QString("Couldn't open %1: %2").arg(qrcFile).arg(jsFile.errorString());
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
    QDateTime now = QDateTime::currentDateTime();
    QString pointName("point_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::PointMetadata ptMetadata;
    QString logicalName = pointName;
    ptMetadata.set_logical_name(logicalName.toStdString());
    ptMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    ptMetadata.set_latitude(mLastLat.toDouble());
    ptMetadata.set_longitude(mLastLng.toDouble());
    ptMetadata.set_altitude(0.0);
    ptMetadata.set_address(mLastAddr.toStdString());

    std::string fullPath = writePointProtobufByName(pointName, ptMetadata);

    // Add the new point to the list
    PointListElement listElement;
    listElement.protoFilePath = QString::fromStdString(fullPath);
    listElement.logicalName = QString::fromStdString(ptMetadata.logical_name());
    listElement.description = QString::fromStdString(ptMetadata.description());
    listElement.latitude = ptMetadata.latitude();
    listElement.longitude = ptMetadata.longitude();
    listElement.address = QString::fromStdString(ptMetadata.address());

    PointItemBuilder builder(mUi->loc_pointList);
    builder.addPoint(std::move(listElement), this);

    mUi->loc_noSavedPoints_mask->setVisible(false);
}

// Invoked when the user clicks on the map
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) {
    mLastLat = lat;
    mLastLng = lng;
    mLastAddr = address;
    // Make nothing selected now
    mUi->loc_pointList->setCurrentItem(nullptr);
}

void LocationPage::on_loc_savePoint_clicked() {
    QDateTime now = QDateTime::currentDateTime();
    QString pointName("point_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::PointMetadata ptMetadata;
    QString logicalName = pointName;
    ptMetadata.set_logical_name(logicalName.toStdString());
    ptMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    ptMetadata.set_latitude(mLastLat.toDouble());
    ptMetadata.set_longitude(mLastLng.toDouble());
    ptMetadata.set_altitude(0.0);
    ptMetadata.set_address(mLastAddr.toStdString());

    std::string fullPath = writePointProtobufByName(pointName, ptMetadata);

    // Add the new point to the list
    PointListElement listElement;
    listElement.protoFilePath = QString::fromStdString(fullPath);
    listElement.logicalName = QString::fromStdString(ptMetadata.logical_name());
    listElement.description = QString::fromStdString(ptMetadata.description());
    listElement.latitude = ptMetadata.latitude();
    listElement.longitude = ptMetadata.longitude();
    listElement.address = QString::fromStdString(ptMetadata.address());

    PointItemBuilder builder(mUi->loc_pointList);
    builder.addPoint(std::move(listElement), this);
}

void LocationPage::on_loc_singlePoint_setLocationButton_clicked() {
    sendMostRecentUiLocation();
}

// Populate the QListWidget with the points that are found on disk
void LocationPage::scanForPoints() {
    mUi->loc_pointList->clear();
    // Disable sorting while we're updating the table
    mUi->loc_pointList->setSortingEnabled(false);

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
    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedPoints_mask->setVisible(mUi->loc_pointList->count() == 0);
}

void LocationPage::on_loc_pointList_currentItemChanged(QListWidgetItem* current,
                                                       QListWidgetItem* previous) {
    if (previous) {
        PointWidgetItem* item = getItemWidget(mUi->loc_pointList, previous);
        if (item != nullptr) {
            item->setSelected(false);
        }
    }
    if (current) {
        PointWidgetItem* item = getItemWidget(mUi->loc_pointList, current);
        if (item != nullptr) {
            item->setSelected(true);
            // update the selected point
            auto& pointElement = item->pointElement();
            // show the location on the map, but do not send it to the device
            emit mMapBridge->showLocation(QString::number(pointElement.latitude, 'g', 12),
                                          QString::number(pointElement.longitude, 'g', 12),
                                          pointElement.address);
        }
    }
}

void LocationPage::pointWidget_editButtonClicked(CCListItem* listItem) {
    auto* pointWidgetItem = reinterpret_cast<PointWidgetItem*>(listItem);
    QMenu* popMenu = new QMenu(this);
    QAction* editAction   = popMenu->addAction(tr("Edit"));
    QAction* deleteAction = popMenu->addAction(tr("Delete"));

    mUi->loc_pointList->setCurrentItem(pointWidgetItem->listWidgetItem());
    QAction* theAction = popMenu->exec(QCursor::pos());
    if (theAction == editAction && editPoint(pointWidgetItem->pointElement())) {
        pointWidgetItem->refresh();
        auto& pointElement = pointWidgetItem->pointElement();
        // show the location on the map, but do not send it to the device
        emit mMapBridge->showLocation(QString::number(pointElement.latitude, 'g', 12),
                                      QString::number(pointElement.longitude, 'g', 12),
                                      pointElement.address);
    } else if (theAction == deleteAction && deletePoint(pointWidgetItem->pointElement())) {
        mUi->loc_pointList->setCurrentItem(nullptr);
        pointWidgetItem->removeFromListWidget();
        // If the list is empty, show an overlay saying that.
        mUi->loc_noSavedPoints_mask->setVisible(mUi->loc_pointList->count() == 0);
        emit mMapBridge->resetPointsMap();
    }
}

bool LocationPage::editPoint(PointListElement& pointElement) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = pointElement.logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Address
    dialogLayout->addWidget(new QLabel(tr("Address")));
    QLineEdit* addrField = new QLineEdit(this);
    addrField->setText(pointElement.address);
    addrField->setReadOnly(true);
    dialogLayout->addWidget(addrField);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = pointElement.description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    // Latitude
    dialogLayout->addWidget(new QLabel(tr("Latitude")));
    QLineEdit* latitudeField = new QLineEdit(this);
    latitudeField->setText(QString::number(pointElement.latitude, 'g', 12));
    latitudeField->setReadOnly(true);
    dialogLayout->addWidget(latitudeField);

    // Longitude
    dialogLayout->addWidget(new QLabel(tr("Longitude")));
    QLineEdit* longitudeField = new QLineEdit(this);
    longitudeField->setText(QString::number(pointElement.longitude, 'g', 12));
    longitudeField->setReadOnly(true);
    dialogLayout->addWidget(longitudeField);

    // OK and Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(tr("Edit point"));
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

        writePointProtobufFullPath(pointElement.protoFilePath, pointMetadata);
        QApplication::restoreOverrideCursor();
    }

    return true;
}

bool LocationPage::deletePoint(const PointListElement& pointElement) {
    bool ret = false;
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
        QApplication::setOverrideCursor(Qt::WaitCursor);
        std::string protobufName = pointElement.protoFilePath.toStdString();
        android::base::StringView dirName;
        bool haveDirName = android::base::PathUtils::split(protobufName,
                                                           &dirName,
                                                           nullptr /* base name */);
        if (haveDirName) {
            path_delete_dir(dirName.str().c_str());
            ret = true;
        }

        QApplication::restoreOverrideCursor();
    }

    return ret;
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
    double initialLat, initialLng, unusedAlt, unusedVelocity, unusedHeading;
    mMapBridge.reset(new MapBridge(this));
    getDeviceLocation(&initialLat, &initialLng, &unusedAlt, &unusedVelocity, &unusedHeading);

    QObject::connect(this, &LocationPage::signal_saveRoute,
                     this, &LocationPage::map_saveRoute,
                     Qt::QueuedConnection);

    // Set up two web engines: one for the Points page and one for the Routes page
    for (int webEnginePageIdx = 0; webEnginePageIdx < 2; webEnginePageIdx++) {
        bool isPoint = (webEnginePageIdx == 0);
        QWebEnginePage* webEnginePage = isPoint ? mUi->loc_pointWebEngineView->page()
                                                : mUi->loc_routeWebEngineView->page();

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
                "var wsUri = 'ws://localhost:12345';"
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
                        "channel.objects.emulocationserver.showRouteOnMap.connect(function(routeJson) {"
                            "if (setRouteOnMap) setRouteOnMap(routeJson);"
                        "});");
            appendString.append(
                        "channel.objects.emulocationserver.showRoutePlaybackOverlay.connect(function(visible) {"
                            "if (showRoutePlaybackOverlay) showRoutePlaybackOverlay(visible);"
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

        mServer.reset(new QWebSocketServer(QStringLiteral("QWebChannel Standalone Example Server"),
                            QWebSocketServer::NonSecureMode));
        if (!mServer->listen(QHostAddress::LocalHost, 12345)) {
            printf("Unable to open web socket port 12345.");
        }

        // wrap WebSocket clients in QWebChannelAbstractTransport objects
        mClientWrapper.reset(new WebSocketClientWrapper(mServer.get()));

        // setup the channel
        mWebChannel.reset(new QWebChannel(webEnginePage));
        QObject::connect(mClientWrapper.get(), &WebSocketClientWrapper::clientConnected,
                         mWebChannel.get(), &QWebChannel::connectTo);

        // setup the dialog and publish it to the QWebChannel
        mWebChannel->registerObject(QStringLiteral("emulocationserver"), mMapBridge.get());

        // Get the HTML file (either 'index.html' or route.html')
        QFile indexHtmlFile(isPoint ? ":/html/index.html" : ":/html/route.html");

        // Insert the Maps API key into the HTML and publish
        // that to QWebChannel
        if (indexHtmlFile.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray htmlByteArray = indexHtmlFile.readAll();
            if (!htmlByteArray.isEmpty()) {
                // Change the placeholder to the real Maps API key
                htmlByteArray.replace("YOUR_API_KEY", mMapsApiKey.toUtf8());
                // Publish it
                webEnginePage->setHtml(htmlByteArray, QUrl(isPoint ? "qrc:/html/index.html" : "qrc:/html/route.html"));
            }
        }
    }
}


#else // !USE_WEBENGINE  These are the stubs for when we don't have WebEngine
void MapBridge::map_savePoint() { }
void MapBridge::sendLocation(const QString& lat, const QString& lng, const QString& address) { }
void LocationPage::map_savePoint() { }
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) { }
void LocationPage::on_loc_savePoint_clicked() { }
void LocationPage::on_loc_singlePoint_setLocationButton_clicked() { }
void LocationPage::scanForPoints() { }
void LocationPage::on_loc_pointList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) { }
bool LocationPage::editPoint(PointListElement& pointElement) { }
bool LocationPage::deletePoint(const PointListElement& pointElement) { }
std::string LocationPage::writePointProtobufByName(
        const QString& pointFormalName,
        const emulator_location::PointMetadata& protobuf) { return ""; }
void LocationPage::writePointProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::PointMetadata& protobuf) { }
void LocationPage::setUpWebEngine() { }
void LocationPage::pointWidget_editButtonClicked(CCListItem* listItem) { }
#endif // !USE_WEBENGINE
