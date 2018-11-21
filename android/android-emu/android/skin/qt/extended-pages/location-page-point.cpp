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

// Invoked when the user clicks on the map
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) {
    mLastLat = lat;
    mLastLng = lng;
    mLastAddr = address;
    // Make nothing selected now
    mUi->loc_pointList->setCurrentItem(nullptr);
    mSelectedPointName.clear();
    highlightPointListWidget();
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
    mSelectedPointName = fullPath.c_str();

    populatePointListWidget();
}

void LocationPage::on_loc_singlePoint_setLocationButton_clicked() {
    sendMostRecentUiLocation();
}

// Populate mPointList with the points that are found on disk
void LocationPage::scanForPoints() {

    mPointList.clear();

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
        listElement.protoFilePath = protoFilePath.c_str();
        listElement.logicalName = pointMetadata->logical_name().c_str();
        listElement.description = pointMetadata->description().c_str();
        listElement.latitude = pointMetadata->latitude();
        listElement.longitude = pointMetadata->longitude();
        listElement.address = pointMetadata->address().c_str();

        mPointList.append(listElement);
    }
}

// Update mPointList and populate the UI list of points
void LocationPage::populatePointListWidget() {
    // Get the saved points
    scanForPoints();

    // Disable sorting while we're updating the table
    mUi->loc_pointList->setSortingEnabled(false);

    // Set the dotdotdot column to take the available space. This
    // prevents the widget from scrolling horizontally due to an
    // unreasonably wide final column.
    mUi->loc_pointList->setColumnWidth(1,
            mUi->loc_pointList->width() - mUi->loc_pointList->columnWidth(0));

    int nItems = mPointList.size();
    mUi->loc_pointList->setRowCount(nItems);

    for (int idx = 0; idx < nItems; idx++) {
        mUi->loc_pointList->setItem(idx, 0, new PointWidgetItem(&mPointList[idx]));
        mUi->loc_pointList->setItem(idx, 1, new QTableWidgetItem());
    }

    // All done updating. Enable sorting now.
    mUi->loc_pointList->sortByColumn(0, Qt::AscendingOrder);
    mUi->loc_pointList->setSortingEnabled(true);

    // If the list is empty, show an overlay saying that.
    mUi->loc_noSavedPoints_mask->setVisible(nItems <= 0);

    // Highlight the selected row
    highlightPointListWidget();
}

// Update the appearance of the UI list of points to
// highlight the row that is selected
void LocationPage::highlightPointListWidget() {
    // Update the QTableWidgetItems that are associated
    // with this widget
    for (int row = 0; row < mUi->loc_pointList->rowCount(); row++) {
        PointWidgetItem* pointItem = (PointWidgetItem*)mUi->loc_pointList->takeItem(row, 0);
        bool isSelected = (mSelectedPointName == pointItem->pointElement->protoFilePath);
        mPointItemBuilder->highlightPointWidgetItem(pointItem, isSelected);
        mUi->loc_pointList->setItem(row, 0, pointItem);

        QTableWidgetItem* dotDotItem = mUi->loc_pointList->takeItem(row, 1);
        mPointItemBuilder->highlightDotDotWidgetItem(dotDotItem, isSelected);
        mUi->loc_pointList->setItem(row, 1, dotDotItem);

        if (isSelected) {
            mUi->loc_pointList->setCurrentItem(pointItem);
        }
    }
    mUi->loc_pointList->viewport()->repaint();
}

void LocationPage::on_loc_pointList_cellClicked(int row, int column) {
    mUi->loc_pointList->setCurrentCell(row, 0, QItemSelectionModel::Rows);
    if (column == 1) {
        QMenu* popMenu = new QMenu(this);
        QAction* editAction   = popMenu->addAction(tr("Edit"));
        QAction* deleteAction = popMenu->addAction(tr("Delete"));

        QAction* theAction = popMenu->exec(QCursor::pos());
        if (theAction == editAction) {
            editPoint(row);
        } else if (theAction == deleteAction) {
            deletePoint(row);
        }
    }
}

void LocationPage::on_loc_pointList_itemSelectionChanged() {
    int selectedRow = mUi->loc_pointList->currentRow();
    if (selectedRow < 0) return;

    mUi->loc_pointList->setCurrentCell(selectedRow, 0, QItemSelectionModel::Rows);

    auto widgetItem = (PointWidgetItem*)(mUi->loc_pointList->item(selectedRow, 0));
    auto pointElement = widgetItem->pointElement;

    if (mSelectedPointName == pointElement->protoFilePath) {
        // No change in the selection
        if (selectedRow >= 0) {
            mUi->loc_pointList->setCurrentCell(selectedRow, 0); // (Helps if the user drags the selection)
            mUi->loc_pointList->scrollToItem(mUi->loc_pointList->currentItem());
        }
        return;
    }

    mSelectedPointName = pointElement->protoFilePath;

    mLastLat = QString::number(pointElement->latitude, 'g', 12);
    mLastLng = QString::number(pointElement->longitude, 'g', 12);

    // Show the location, but do not send it to the device
    emit showLocation(mLastLat, mLastLng);

    // Redraw the table to show the new selection
    highlightPointListWidget();
}

void LocationPage::editPoint(int row) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    auto widgetItem = (PointWidgetItem*)(mUi->loc_pointList->item(row, 0));
    auto pointElement = widgetItem->pointElement;

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = pointElement->logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Address
    dialogLayout->addWidget(new QLabel(tr("Address")));
    QLineEdit* addrField = new QLineEdit(this);
    addrField->setText(pointElement->address);
    addrField->setReadOnly(true);
    dialogLayout->addWidget(addrField);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = pointElement->description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    // Latitude
    dialogLayout->addWidget(new QLabel(tr("Latitude")));
    QLineEdit* latitudeField = new QLineEdit(this);
    latitudeField->setText(QString::number(pointElement->latitude, 'g', 12));
    latitudeField->setReadOnly(true);
    dialogLayout->addWidget(latitudeField);

    // Longitude
    dialogLayout->addWidget(new QLabel(tr("Longitude")));
    QLineEdit* longitudeField = new QLineEdit(this);
    longitudeField->setText(QString::number(pointElement->longitude, 'g', 12));
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
        return;
    }

    QString newName = nameEdit->text();
    QString newDescription = descriptionEdit->toPlainText();

    if ((!newName.isEmpty() && newName != oldName) ||
        newDescription != oldDescription             )
    {
        // Something changed. Read the existing Protobuf,
        // update it, and write it back out.
        QApplication::setOverrideCursor(Qt::WaitCursor);

        android::location::Point point(pointElement->protoFilePath.toStdString().c_str());

        const emulator_location::PointMetadata* oldPointMetadata = point.getProtoInfo();
        if (oldPointMetadata == nullptr) return;

        emulator_location::PointMetadata pointMetadata(*oldPointMetadata);

        if (!newName.isEmpty()) {
            pointMetadata.set_logical_name(newName.toStdString().c_str());
        }
        pointMetadata.set_description(newDescription.toStdString().c_str());

        writePointProtobufFullPath(pointElement->protoFilePath, pointMetadata);
        populatePointListWidget();
        QApplication::restoreOverrideCursor();
    }
}

void LocationPage::deletePoint(int row) {
    if (row < 0 || row >= mPointList.size()) {
        return;
    }
    auto widgetItem = (PointWidgetItem*)(mUi->loc_pointList->item(row, 0));
    auto thisPoint = widgetItem->pointElement;

    QMessageBox msgBox(QMessageBox::Warning,
                       tr("Delete point"),
                       tr("Do you want to permanently delete<br>point \"%1\"?")
                               .arg(thisPoint->logicalName),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        std::string protobufName = thisPoint->protoFilePath.toStdString();
        android::base::StringView dirName;
        bool haveDirName = android::base::PathUtils::split(protobufName,
                                                           &dirName,
                                                           nullptr /* base name */);
        if (haveDirName) {
            path_delete_dir(dirName.str().c_str());
            mSelectedPointName.clear();
            mUi->loc_pointList->setCurrentItem(nullptr);
            populatePointListWidget();
        }

        QApplication::restoreOverrideCursor();
    }
}

void
LocationPage::PointItemBuilder::highlightPointWidgetItem(LocationPage::PointWidgetItem* theItem,
                                                         bool isSelected)
{
    QColor foregroundColor = getColorForCurrentTheme(Ui::THEME_TEXT_COLOR);
    QColor backgroundColor = isSelected ? getColorForCurrentTheme(Ui::TABLE_SELECTED_VAR)
                                        : getColorForCurrentTheme(Ui::TAB_BKG_COLOR_VAR);

    QPixmap basePixmap(mFieldWidth, mFieldHeight);
    basePixmap.fill(backgroundColor);
    QPainter painter(&basePixmap);
    painter.setPen(foregroundColor);

    QFont baseFont = painter.font();
    int baseFontHeight = painter.fontInfo().pointSize();

    QFont bigFont = baseFont;
    bigFont.setPointSize(bigFont.pointSize() + 2);
    painter.setFont(bigFont);
    int bigFontHeight = painter.fontInfo().pointSize();

    int vertPosition = (mFieldHeight - (bigFontHeight + TEXT_SEPARATION + baseFontHeight)) / 2;
    vertPosition += bigFontHeight;
    painter.drawText(HORIZ_PADDING, vertPosition, theItem->pointElement->logicalName);

    painter.setFont(baseFont);
    vertPosition += bigFontHeight + TEXT_SEPARATION;
    painter.drawText(HORIZ_PADDING, vertPosition, theItem->pointElement->address);

    theItem->setIcon(basePixmap);
}

void LocationPage::PointItemBuilder::highlightDotDotWidgetItem(QTableWidgetItem* dotDotItem, bool isSelected) {
    QColor backgroundColor = isSelected ? getColorForCurrentTheme(Ui::TABLE_SELECTED_VAR)
                                        : getColorForCurrentTheme(Ui::TAB_BKG_COLOR_VAR);

    QPixmap basePixmap(ICON_SIZE, ICON_SIZE);
    basePixmap.fill(backgroundColor);
    QPainter painter(&basePixmap);

    QPixmap dotDotIcon = getIconForCurrentTheme("more_vert").pixmap(ICON_SIZE, ICON_SIZE);

    painter.drawPixmap(QRect(0, 0, ICON_SIZE, ICON_SIZE), dotDotIcon);

    dotDotItem->setIcon(basePixmap);
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
    getDeviceLocation(&initialLat, &initialLng, &unusedAlt, &unusedVelocity, &unusedHeading);

    // Set up two web engines: one for the Points page and one for the Routes page
    for (int webEnginePageIdx = 0; webEnginePageIdx < 2; webEnginePageIdx++) {
        bool isPoint = (webEnginePageIdx == 0);
        QWebEnginePage* webEnginePage = isPoint ? mUi->loc_pointWebEngineView->page()
                                                : mUi->loc_routeWebEngineView->page();
        QFile webChannelJsFile(":/html/js/qwebchannel.js");
        if(!webChannelJsFile.open(QIODevice::ReadOnly)) {
            qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
            continue;
        }
        QByteArray webChannelJs = webChannelJsFile.readAll();

        // Send the current location to each page
        webChannelJs.append(
                "\n"
                "var initialLat = '");
        webChannelJs.append(QString::number(initialLat, 'g', 12));
        webChannelJs.append(
                "'; var initialLng = '");
        webChannelJs.append(QString::number(initialLng, 'g', 12));
        webChannelJs.append(
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
            webChannelJs.append(
                        "channel.objects.emulocationserver.showLocation.connect(function(lat, lng) {"
                            "if (showPendingLocation) showPendingLocation(lat, lng);"
                        "});");
        } else {
            // Define Routes-specific interfaces
            webChannelJs.append(
                        "channel.objects.emulocationserver.travelModeChanged.connect(function(mode) {"
                            "if (setTravelMode) setTravelMode(mode);"
                        "});"
                        "channel.objects.emulocationserver.showRouteOnMap.connect(function(routeJson) {"
                            "if (setRouteOnMap) setRouteOnMap(routeJson);"
                        "});");
        }
        webChannelJs.append(
                    "});"
                    "if (typeof setDeviceLocation != 'undefined' && setDeviceLocation != null) {"
                        "setDeviceLocation(initialLat, initialLng);"
                    "}"
                "}");

        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        webEnginePage->scripts().insert(script);

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
        mWebChannel->registerObject(QStringLiteral("emulocationserver"), this);

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
                webEnginePage->setHtml(htmlByteArray);
            }
        }
    }
}


#else // !USE_WEBENGINE  These are the stubs for when we don't have WebEngine
void LocationPage::sendLocation(const QString& lat, const QString& lng, const QString& address) { }
void LocationPage::on_loc_savePoint_clicked() { }
void LocationPage::on_loc_singlePoint_setLocationButton_clicked() { }
void LocationPage::scanForPoints() { }
void LocationPage::populatePointListWidget() { }
void LocationPage::highlightPointListWidget() { }
void LocationPage::on_loc_pointList_cellClicked(int row, int column) { }
void LocationPage::on_loc_pointList_itemSelectionChanged() { }
void LocationPage::editPoint(int row) { }
void LocationPage::deletePoint(int row) { }
void LocationPage::PointItemBuilder::highlightPointWidgetItem(LocationPage::PointWidgetItem* theItem,
                                                              bool isSelected) { }
void LocationPage::PointItemBuilder::highlightDotDotWidgetItem(QTableWidgetItem* dotDotItem,
                                                               bool isSelected) { }
std::string LocationPage::writePointProtobufByName(
        const QString& pointFormalName,
        const emulator_location::PointMetadata& protobuf) { return ""; }
void LocationPage::writePointProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::PointMetadata& protobuf) { }
void LocationPage::setUpWebEngine() { }
#endif // !USE_WEBENGINE
