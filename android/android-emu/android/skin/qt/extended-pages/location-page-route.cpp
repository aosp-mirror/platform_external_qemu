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

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/StringView.h"
#include "android/emulation/ConfigDirs.h"
#include "android/location/Route.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/extended-pages/location-route-playback-item.h"
#include "android/utils/path.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QtConcurrent/QtConcurrentRun>

#include <fstream>

static const char JSON_FILE_NAME[]  = "route.json";
static const char PROTO_FILE_NAME[] = "route_metadata.pb";

namespace {
using RouteListElement = LocationPage::RouteListElement;

class RouteItemBuilder {
public:
    RouteItemBuilder(QListWidget* listWidget) :
        mListWidget(listWidget)
    {
    }

    void addRoute(RouteListElement&& p, LocationPage* const locationPage) {
        RouteWidgetItem* routeWidgetItem = new RouteWidgetItem(std::move(p), mListWidget);
        locationPage->connect(routeWidgetItem,
                SIGNAL(editButtonClickedSignal(CCListItem*)), locationPage,
                SLOT(routeWidget_editButtonClicked(CCListItem*)));
        mListWidget->setCurrentItem(routeWidgetItem->listWidgetItem());
    }

private:
    QListWidget* mListWidget = nullptr;
};

RouteWidgetItem* getItemWidget(QListWidget* list,
                               QListWidgetItem* listItem) {
    return qobject_cast<RouteWidgetItem*>(list->itemWidget(listItem));
}
}  // namespace

// static
QString LocationPage::toJsonString(const GpsFixArray* arr) {
    QString ret;

    if (arr == nullptr || arr->size() == 0) {
        return ret;
    }
    // The first point will always have 0 sec delay.
    time_t prevTime = (*arr)[0].time;

    ret.append("{\"path\":[");
    for (int i = 0; i < arr->size(); ++i) {
        const GpsFix& fix = (*arr)[i];
        time_t delay = fix.time - prevTime;
        // Ensure all other delays are > 0, even if multiple points have
        // the same timestamp.
        if (delay == 0 && i != 0) {
            delay = 2;
        }
        ret.append(QString("{\"lat\":%1,\"lng\":%2,\"elevation\":%3,\"delay_sec\":%4},")
                .arg(QString::number(fix.latitude))
                .arg(QString::number(fix.longitude))
                .arg(QString::number(fix.elevation))
                .arg(QString::number(delay)));
    }
    // Remove the last comma
    ret.chop(1);
    ret.append("]}");

    return ret;
}

void LocationPage::stylePopupMenu(QMenu* popMenu) {
    popMenu->setMinimumWidth(100);
    popMenu->setStyleSheet("QMenu::item {padding: 4px;}"
        "QMenu::item:selected {background-color: #4285f4;}");
}

// Populate the saved routes list with the routes that are found on disk
void LocationPage::scanForRoutes() {
    // Get the directory
    std::string locationsDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "routes");

    if (!path_is_dir(locationsDirectoryName.c_str())) {
        // No "locations/routes" directory
        return;
    }

    // Find all the subdirectories in the locations directory
    QDir locationsDir(locationsDirectoryName.c_str());
    locationsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList routeList(locationsDir.entryList());

    mUi->loc_routeList->setSortingEnabled(false);
    mUi->loc_routeList->blockSignals(true);
    RouteItemBuilder builder(mUi->loc_routeList);
    // Look at all the directories and create an entry for each valid route
    for (const QString& routeName : routeList) {
        // Read the route protobuf
        std::string protoFilePath = android::base::PathUtils::
                join(locationsDirectoryName.c_str(),
                     routeName.toStdString().c_str(),
                     PROTO_FILE_NAME);

        android::location::Route route(protoFilePath.c_str());

        const emulator_location::RouteMetadata* routeMetadata = route.getProtoInfo();

        if (routeMetadata == nullptr) continue;

        RouteListElement listElement;
        listElement.protoFilePath = QString::fromStdString(protoFilePath);
        listElement.logicalName   = QString::fromStdString(routeMetadata->logical_name());
        listElement.description   = QString::fromStdString(routeMetadata->description());
        listElement.modeIndex     = routeMetadata->mode_of_travel();
        listElement.numPoints     = routeMetadata->number_of_points();
        listElement.duration      = routeMetadata->duration();
        listElement.jsonFormat    = routeMetadata->json_format();

        builder.addRoute(std::move(listElement), this);
    }
    // All done updating. Enable sorting now.
    mUi->loc_routeList->setSortingEnabled(true);
    mUi->loc_routeList->blockSignals(false);
    mUi->loc_routeList->setCurrentItem(nullptr);

    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedRoutes_mask->setVisible(mUi->loc_routeList->count() <= 0);
}

void LocationPage::routeWidget_editButtonClicked(CCListItem* listItem) {
    mUi->loc_routeList->blockSignals(true);
    auto* routeWidgetItem = reinterpret_cast<RouteWidgetItem*>(listItem);
    QMenu* popMenu = new QMenu(this);
    stylePopupMenu(popMenu);
    QAction* editAction   = popMenu->addAction(tr("Edit"));
    QAction* deleteAction = popMenu->addAction(tr("Delete"));

    QAction* theAction = popMenu->exec(QCursor::pos());
    if (theAction == editAction && editRoute(routeWidgetItem->routeElement())) {
        // We don't need to send any updates to the map since we aren't editing any
        // of the routing points.
        routeWidgetItem->refresh();
    } else if (theAction == deleteAction) {
        auto& routeElement = routeWidgetItem->routeElement();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QMessageBox msgBox(QMessageBox::Warning,
                           tr("Delete route"),
                           tr("Do you want to permanently delete<br>route \"%1\"?")
                                   .arg(routeElement.logicalName),
                           QMessageBox::Cancel,
                           this);
        QPushButton *deleteButton = msgBox.addButton(QMessageBox::Apply);
        deleteButton->setText(tr("Delete"));

        int selection = msgBox.exec();

        if (selection == QMessageBox::Apply) {
            deleteRoute(routeWidgetItem);
            mPrevSelectedRoutes.clear();
        }
        QApplication::restoreOverrideCursor();
    }
    mUi->loc_routeList->blockSignals(false);
}

void LocationPage::updateRouteWidgetItemsColor() {
    // Deselect everything selected previously
    for (auto* item : mPrevSelectedRoutes) {
        auto* routeWidgetItem = getItemWidget(mUi->loc_routeList, item);
        if (routeWidgetItem != nullptr) {
            routeWidgetItem->setSelected(false);
            routeWidgetItem->refresh();
        }
    }
    // Select everything in the selected items list
    for (auto* item : mUi->loc_routeList->selectedItems()) {
        auto* routeWidgetItem = getItemWidget(mUi->loc_routeList, item);
        if (routeWidgetItem != nullptr) {
            routeWidgetItem->setSelected(true);
            routeWidgetItem->refresh();
        }
    }
    mPrevSelectedRoutes = mUi->loc_routeList->selectedItems();
}

void LocationPage::on_loc_routeList_itemSelectionChanged() {
    UiState newState = mUi->loc_routeList->selectedItems().size() > 1 ?
                       UiState::Deletion :
                       UiState::Default;
    updateRouteWidgetItemsColor();

    if (mRouteState == UiState::Deletion &&
        mUi->loc_routeList->selectedItems().size() != 1) {
        // Either no routes selected, or in deletion mode, either of which
        // means the map shouldn't be set to anything.
        emit mMapBridge->showRouteOnMap("");
    }
    if (mUi->loc_routeList->selectedItems().size() == 1) {
        // show the location on the map, but do not send it to the device
        auto* item = getItemWidget(mUi->loc_routeList,
                                   mUi->loc_routeList->selectedItems()[0]);
        auto& routeElement = item->routeElement();
        mRouteJson = ""; // Forget any unsaved route we may have received

        setLoadingOverlayVisible(true, tr("Loading Saved Route..."));
        mRouteTravelMode = routeElement.modeIndex;
        mRouteNumPoints = routeElement.numPoints;
        mUi->loc_playRouteButton->setEnabled(false);
        mRouteSender.reset(RouteSenderThread::newInstance(
                this,
                SLOT(routeSendingFinished(bool))));
        mRouteSender->sendRouteToMap(&routeElement, mMapBridge.get());
    }

    // Disable the edit buttons on the saved points if in deletion mode
    for (int i = 0; i < mUi->loc_routeList->count(); ++i) {
        auto item = mUi->loc_routeList->item(i);
        auto* routeWidgetItem = getItemWidget(mUi->loc_routeList, item);
        if (routeWidgetItem != nullptr) {
            routeWidgetItem->setEditButtonEnabled(newState == UiState::Default);
        }
    }
    mUi->loc_playRouteButton->setText(
            newState == UiState::Default ?
                      tr("PLAY ROUTE") :
                      tr("DELETE ITEMS"));
    mRouteState = newState;
}

bool LocationPage::editRoute(RouteListElement& routeElement, bool isNewRoute) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = routeElement.logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = routeElement.description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    QApplication::restoreOverrideCursor();

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
            isNewRoute ? QDialogButtonBox::Save :
                         QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                         Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(tr("Edit route"));
    editDialog.setLayout(dialogLayout);


    int selection = editDialog.exec();
    if (selection == QDialog::Rejected) {
        return false;
    }

    QString newName = nameEdit->text();
    QString newDescription = descriptionEdit->toPlainText();

    if ((newName.isEmpty() || newName == oldName) &&
        newDescription == oldDescription)
    {
        return false;
    }
    // Something changed. Read the existing Protobuf,
    // update it, and write it back out.
    QApplication::setOverrideCursor(Qt::WaitCursor);

    android::location::Route route(routeElement.protoFilePath.toStdString().c_str());

    const emulator_location::RouteMetadata* oldRouteMetadata = route.getProtoInfo();
    if (oldRouteMetadata == nullptr) return false;

    emulator_location::RouteMetadata routeMetadata(*oldRouteMetadata);

    if (!newName.isEmpty()) {
        routeMetadata.set_logical_name(newName.toStdString().c_str());
        routeElement.logicalName = newName;
    }
    routeMetadata.set_description(newDescription.toStdString().c_str());
    routeElement.description = newDescription;

    writeRouteProtobufFullPath(routeElement.protoFilePath, routeMetadata);
    QApplication::restoreOverrideCursor();
    return true;
}

void LocationPage::deleteSelectedRoutes() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QMessageBox msgBox(QMessageBox::Warning,
                       tr("Delete selected routes"),
                       tr("Do you want to permanently delete the selected routes?"),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        mPrevSelectedRoutes.clear();
        QList<QListWidgetItem*> selectedItems =
                mUi->loc_routeList->selectedItems();
        for (int i = 0; i < selectedItems.size(); ++i) {
            auto* routeItem = getItemWidget(mUi->loc_routeList,
                                            selectedItems[i]);
            deleteRoute(routeItem);
        }
        emit mMapBridge->showRouteOnMap("");
        mUi->loc_playRouteButton->setEnabled(false);
    }
    QApplication::restoreOverrideCursor();
}

void LocationPage::deleteRoute(RouteWidgetItem* item) {
    auto& routeElement = item->routeElement();
    std::string protobufName = routeElement.protoFilePath.toStdString();
    android::base::StringView dirName;
    bool haveDirName = android::base::PathUtils::split(protobufName,
                                                       &dirName,
                                                       nullptr /* base name */);
    if (haveDirName) {
        path_delete_dir(dirName.str().c_str());
    }
    mUi->loc_routeList->setCurrentItem(nullptr);
    item->removeFromListWidget();
    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedRoutes_mask->setVisible(mUi->loc_routeList->count() == 0);
}

// Display the details of the not-yet-saved route
void LocationPage::showPendingRouteDetails() {
    if (mRouteNumPoints <= 0) {
        return;
    }

    QString modeString;
    switch (mRouteTravelMode) {
        default:
        case 0:  modeString = tr("Driving");    break;
        case 1:  modeString = tr("Walking");    break;
        case 2:  modeString = tr("Bicycling");  break;
        case 3:  modeString = tr("Transit");    break;
    }
    int durationSeconds = mRouteTotalTime + 0.5;
    int durationHours = durationSeconds / (60 * 60);
    durationSeconds -= durationHours * (60 * 60);
    int durationMinutes = durationSeconds / 60;
    durationSeconds -= durationMinutes * 60;
    QString durationString = (durationHours > 0)   ? tr("%1h %2m %3s")
                                                     .arg(durationHours)
                                                     .arg(durationMinutes)
                                                     .arg(durationSeconds)
                                                   :
                             (durationMinutes > 0) ? tr("%1m %2s")
                                                     .arg(durationMinutes)
                                                     .arg(durationSeconds)
                                                   :
                                                     tr("%1s")
                                                     .arg(durationSeconds);
    QString infoString = tr("<b>Unsaved route</b><br>"
                            "&nbsp;&nbsp;Duration: <b>%1</b><br>"
                            "&nbsp;&nbsp;Mode: <b>%2</b><br>"
                            "&nbsp;&nbsp;Number of points: <b>%3</b>")
                          .arg(durationString)
                          .arg(modeString)
                          .arg(mRouteNumPoints);
}

void MapBridge::sendFullRouteToEmu(int numPoints, double durationSeconds, const QString& routeJson, const QString& mode) {
    mLocationPage->sendFullRouteToEmu(numPoints, durationSeconds, routeJson, mode);
}

void MapBridge::onSavedRouteDrawn() {
    mLocationPage->onSavedRouteDrawn();
}

// Invoked by the Maps javascript when a route has been created
void LocationPage::sendFullRouteToEmu(int numPoints, double durationSeconds, const QString& routeJson, const QString& mode) {
    mRouteNumPoints = numPoints;
    if (mRouteNumPoints > 0) {
        mRouteTotalTime = durationSeconds;
        mRouteCreationTime = QDateTime::currentDateTime();
        mRouteTravelMode = RouteWidgetItem::travelModeToInt(mode);
        mRouteJson = routeJson;
    }

    mUi->loc_routeList->setCurrentItem(nullptr);
    showPendingRouteDetails();
    mUi->loc_playRouteButton->setEnabled(mRouteNumPoints > 0);
}

void LocationPage::onSavedRouteDrawn() {
    setLoadingOverlayVisible(false);
}

void MapBridge::saveRoute() {
    emit mLocationPage->signal_saveRoute();
}

void LocationPage::map_saveRoute() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Create the protobuf describing this route
    QString routeName("route_" + mRouteCreationTime.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::RouteMetadata routeMetadata;
    routeMetadata.set_logical_name(routeName.toStdString().c_str());
    routeMetadata.set_creation_time(mRouteCreationTime.toMSecsSinceEpoch() / 1000LL);
    routeMetadata.set_mode_of_travel((emulator_location::RouteMetadata_Mode)mRouteTravelMode);
    routeMetadata.set_number_of_points(mRouteNumPoints);
    routeMetadata.set_duration((int)(mRouteTotalTime + 0.5));
    // TODO: move all of this stuff outside of the Qt code. We are susceptible to getting broken
    // if Google maps decides to change the format of the Json data.
    routeMetadata.set_json_format(emulator_location::RouteMetadata_JsonFormat_GOOGLEMAPS);
    std::string protoPath = writeRouteProtobufByName(routeName, routeMetadata);

    // Write the JSON to a file
    writeRouteJsonFile(protoPath, mRouteJson.toStdString());
    QApplication::restoreOverrideCursor();

    // Add the new route to the list
    RouteListElement listElement;
    listElement.protoFilePath = QString::fromStdString(protoPath);
    listElement.logicalName   = QString::fromStdString(routeMetadata.logical_name());
    listElement.description   = QString::fromStdString(routeMetadata.description());
    listElement.modeIndex     = routeMetadata.mode_of_travel();
    listElement.numPoints     = routeMetadata.number_of_points();
    listElement.duration      = routeMetadata.duration();
    listElement.jsonFormat    = routeMetadata.json_format();

    // Open a dialog to let the user change the name of the route
    editRoute(listElement, true);

    RouteItemBuilder builder(mUi->loc_routeList);
    builder.addRoute(std::move(listElement), this);
    mUi->loc_noSavedRoutes_mask->setVisible(false);

}

void LocationPage::saveGpxKmlRoute(const QString& gpxKmlFilename, const QString& jsonString) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QDateTime creationTime = QDateTime::currentDateTime();
    // Create the protobuf describing this route
    QString routeName("route_" + creationTime.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::RouteMetadata routeMetadata;
    routeMetadata.set_logical_name(routeName.toStdString().c_str());
    routeMetadata.set_description(gpxKmlFilename.toStdString().c_str());
    routeMetadata.set_creation_time(creationTime.toMSecsSinceEpoch() / 1000LL);
    // TODO: move all of this stuff outside of the Qt code. We are susceptible to getting broken
    // if Google maps decides to change the format of the Json data.
    routeMetadata.set_json_format(emulator_location::RouteMetadata_JsonFormat_GPXKML);
    std::string protoPath = writeRouteProtobufByName(routeName, routeMetadata);

    // Write the JSON to a file
    writeRouteJsonFile(protoPath, jsonString.toStdString());

    // Add the new route to the list
    RouteListElement listElement;
    listElement.protoFilePath = QString::fromStdString(protoPath);
    listElement.logicalName   = QString::fromStdString(routeMetadata.logical_name());
    listElement.description   = QString::fromStdString(routeMetadata.description());
    listElement.jsonFormat    = routeMetadata.json_format();

    RouteItemBuilder builder(mUi->loc_routeList);
    builder.addRoute(std::move(listElement), this);
    mUi->loc_noSavedRoutes_mask->setVisible(false);
    QApplication::restoreOverrideCursor();
}

// Write a protobuf into the specified directory.
// This code determines the parent directory. The
// full path of output file is returned.
std::string LocationPage::writeRouteProtobufByName(
        const QString& routeFormalName,
        const emulator_location::RouteMetadata& protobuf)
{
    // Get the directory to hold the protobuf
    std::string protoFileDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "routes",
                 routeFormalName.toStdString().c_str());

    // Ensure that the directory exists
    if (path_mkdir_if_needed(protoFileDirectoryName.c_str(), 0755)) {
        // Fail
        return std::string();
    }

    // The path exists now.
    // Construct the full file path
    std::string protoFilePath = android::base::PathUtils::
            join(protoFileDirectoryName.c_str(), PROTO_FILE_NAME);

    writeRouteProtobufFullPath(protoFilePath.c_str(), protobuf);
    return protoFilePath;
}

void LocationPage::writeRouteProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::RouteMetadata& protobuf)
{
    std::ofstream outStream(protoFullPath.toStdString().c_str(), std::ofstream::binary);
    protobuf.SerializeToOstream(&outStream);
}

QString LocationPage::readRouteJsonFile(const QString& pathOfProtoFile) {
    // Read a text file with the JSON that we received
    // from Google Maps
    QString fullContents;
    char* protoDirName = path_dirname(pathOfProtoFile.toStdString().c_str());
    if (protoDirName) {
        char* jsonFileName = path_join(protoDirName, JSON_FILE_NAME);
        QFile jsonFile(jsonFileName);
        if (jsonFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream jsonStream(&jsonFile);
            fullContents = jsonStream.readAll();
            jsonFile.close();
        }
        free(jsonFileName);
        free(protoDirName);
    }
    return fullContents;
}

void LocationPage::writeRouteJsonFile(const std::string& pathOfProtoFile,
                                      const std::string& jsonString) {
    // Write a text file with the JSON that we received
    // from Google Maps
    char* protoDirName = path_dirname(pathOfProtoFile.c_str());
    if (protoDirName) {
        char* jsonFileName = path_join(protoDirName, JSON_FILE_NAME);
        QFile jsonFile(jsonFileName);
        if (jsonFile.open(QFile::WriteOnly | QFile::Text)) {
            jsonFile.write(jsonString.c_str());
            jsonFile.write("\n");
            jsonFile.close();
        }
        free(jsonFileName);
        free(protoDirName);
    }
}

#ifdef USE_WEBENGINE
void LocationPage::routeSendingFinished(bool ok) {
    mRouteSender.reset();
    mUi->loc_playRouteButton->setEnabled(ok);
    // Wait until the route has drawn on the map before hiding the overlay.
    // When onSavedRouteDrawn() gets called.
}

void LocationPage::routes_updateTheme() {
    for (int i = 0; i < mUi->loc_routeList->count(); ++i) {
        RouteWidgetItem* item = getItemWidget(mUi->loc_routeList, mUi->loc_routeList->item(i));
        item->refresh();
    }
    for (int i = 0; i < mUi->loc_routePlayingList->count(); ++i) {
        RoutePlaybackWaypointItem* item = qobject_cast<RoutePlaybackWaypointItem*>(
                mUi->loc_routePlayingList->itemWidget(mUi->loc_routePlayingList->item(i)));
        item->refresh();
    }
}

void LocationPage::geoDataThreadStarted_v2() {
    {
        QSignalBlocker blocker(mUi->loc_routeList);
    }

    SettingsTheme theme = getSelectedTheme();

    // Prevent the user from initiating a load gpx/kml while another load is already
    // in progress
    setButtonEnabled(mUi->loc_importGpxKmlButton, theme, false);
    setButtonEnabled(mUi->loc_importGpxKmlButton_route, theme, false);
    setButtonEnabled(mUi->loc_playRouteButton, theme, false);
    mNowLoadingGeoData = true;
}

void LocationPage::geoDataThreadFinished_v2(QString file_name, bool ok, QString error_message) {
    finishGeoDataLoading_v2(file_name, ok, error_message, false);
}

void LocationPage::finishGeoDataLoading_v2(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error) {
    mGeoDataLoader.reset();
    setLoadingOverlayVisible(false);
    if (!ok) {
        if (!ignore_error) {
            showErrorDialog(error_message, tr("Geo Data Parser"));
        }
        SettingsTheme theme = getSelectedTheme();
        setButtonEnabled(mUi->loc_importGpxKmlButton, theme, true);
        setButtonEnabled(mUi->loc_importGpxKmlButton_route, theme, true);
        setButtonEnabled(mUi->loc_playRouteButton, theme, true);
        mNowLoadingGeoData = false;
        return;
    }
    const GpsFixArray& fixes = mGpsFixesArray;
    if (fixes.size() == 1) {
        mUi->locationTabs->setCurrentIndex(0);
        savePoint(fixes[0].latitude,
                  fixes[0].longitude,
                  fixes[0].elevation,
                  "");
    } else {
        // TODO: toJsonString may take a long time. Make it async.
        QString gpxKmlRouteJson = toJsonString(&fixes);
        mUi->locationTabs->setCurrentIndex(1);
        saveGpxKmlRoute(file_name, gpxKmlRouteJson);
    }
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->loc_importGpxKmlButton, theme, true);
    setButtonEnabled(mUi->loc_importGpxKmlButton_route, theme, true);
    setButtonEnabled(mUi->loc_playRouteButton, theme, true);
}

#endif // USE_WEBENGINE
