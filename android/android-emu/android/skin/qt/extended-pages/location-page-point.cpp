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
#include "android/base/StringView.h"
#include "android/emulation/ConfigDirs.h"
#include "android/location/Point.h"
#include "android/skin/qt/stylesheet.h"
#include "android/utils/path.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleValidator> // ??
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
//??#include <QWebEngineScript> // ??
#include <QWebEngineScriptCollection>

#include <fstream>

static const char PROTO_FILE_NAME[] = "point_metadata.pb";

// Invoked when the user clicks on the map
void LocationPage::sendLocation(const QString& lat, const QString& lng) {
//    qDebug() << "l-p-p: sendLocation(): Map clicked: lat=" << lat << ", lng=" << lng; // ??
    mLastLat = lat;
    mLastLng = lng;
    printf("l-p-p: sendLocation() received \"%s\", \"%s\"\n", //??
           lat.toStdString().c_str(), lng.toStdString().c_str()); // ??

    // Make nothing selected now
    mUi->pointList->setCurrentItem(nullptr);
    mSelectedPointName.clear();
    highlightPointListWidget();
}

void LocationPage::on_savePoint_clicked() {
    QDateTime now = QDateTime::currentDateTime();
    QString pointName("point_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::PointMetadata ptMetadata;
    QString logicalName = pointName;
    ptMetadata.set_logical_name(logicalName.toStdString());
    ptMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    ptMetadata.set_latitude(mLastLat.toDouble());
    ptMetadata.set_longitude(mLastLng.toDouble());
    ptMetadata.set_altitude(0.0);

    std::string fullPath = writePointProtobufByName(pointName, ptMetadata);
    mSelectedPointName = fullPath.c_str();

    scanForPoints();
    populatePointListWidget();
    highlightPointListWidget();
}

void LocationPage::on_singlePoint_setLocationButton_clicked() {
    sendMostRecentUiLocation();
}

// Populate mPointList with the points that are found on disk
void LocationPage::scanForPoints() {

//    printf("l-p-p: >>>>> Starting scanForPoints()\n"); // ??
    mPointList.clear();

    // Get the directory
    std::string pointDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "points");

    if (!path_is_dir(pointDirectoryName.c_str())) {
        // No "locations/points" directory
        printf("l-p-p: In scanForPoints(): No \"locations/points/\" directory\n"); // ??
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

        mPointList.append(listElement);
    }
//    printf("l-p-p: <<<<< Exit scanForPoints()\n"); // ??
}

// Populate the UI list of points from mPointList
void LocationPage::populatePointListWidget() {
    // Disable sorting while we're updating the table
    mUi->pointList->setSortingEnabled(false);

    // Set the dotdotdot column to take the available space. This
    // prevents the widget from scrolling horizontally due to an
    // unreasonably wide final column.
    // ?? The first time through, the widget width is not reported
    //    correctly. Thus this does not work correctly until the
    //    second time through.
//    printf("l.p.p Widths: col 0 %d, col 1 %d, widget %d\n", // ??
//           mUi->pointList->columnWidth(0), // ??
//           mUi->pointList->columnWidth(1), // ??
//           mUi->pointList->width()); // ??
    mUi->pointList->setColumnWidth(1,
            mUi->pointList->width() - mUi->pointList->columnWidth(0));

    int nItems = mPointList.size();
    mUi->pointList->setRowCount(nItems);

    for (int idx = 0; idx < nItems; idx++) {
//        printf("l-p-p: List %2d: %s\n", idx, mPointList[idx].logicalName.toStdString().c_str()); // ??
        mUi->pointList->setItem(idx, 0, new PointWidgetItem(&mPointList[idx]));
        mUi->pointList->setItem(idx, 1, new QTableWidgetItem());
    }

    // All done updating. Enable sorting now.
    mUi->pointList->sortByColumn(0, Qt::AscendingOrder);
    mUi->pointList->setSortingEnabled(true);

    // If the list is empty, show an overlay saying that.
    mUi->noSavedPoints_mask->setVisible(nItems <= 0);
}

// Update the UI list of points to highlight the
// row that is selected
void LocationPage::highlightPointListWidget() {
    // Update the QTableWidgetItems that are associated
    // with this widget
    for (int row = 0; row < mUi->pointList->rowCount(); row++) {
        PointWidgetItem* pointItem = (PointWidgetItem*)mUi->pointList->takeItem(row, 0);
        bool isSelected = (mSelectedPointName == pointItem->pointElement->protoFilePath);
        mPointItemBuilder->highlightPointWidgetItem(pointItem, isSelected);
        mUi->pointList->setItem(row, 0, pointItem);

        QTableWidgetItem* dotDotItem = mUi->pointList->takeItem(row, 1);
        mPointItemBuilder->highlightDotDotWidgetItem(dotDotItem, isSelected);
        mUi->pointList->setItem(row, 1, dotDotItem);

        if (isSelected) {
            mUi->pointList->setCurrentItem(pointItem);
        }
    }
    mUi->pointList->viewport()->repaint();
}

void LocationPage::on_pointList_cellClicked(int row, int column) {
//    printf("l-p-p: Cell clicked\n");
    mUi->pointList->setCurrentCell(row, 0, QItemSelectionModel::Rows);
    if (column == 1) {
        QMenu* popMenu = new QMenu(this);
        QAction* editAction   = popMenu->addAction(tr("Edit"));
        QAction* deleteAction = popMenu->addAction(tr("Delete"));
        QAction* otherAction  = popMenu->addAction(tr("Some other action"));

        QAction* theAction = popMenu->exec(QCursor::pos());
        if (theAction == editAction) {
            editPoint(row);
        } else if (theAction == deleteAction) {
            deletePoint(row);
        } else if (theAction == otherAction) {
            printf("Got SOME OTHER ACTION\n");
        }
    }
}

void LocationPage::on_pointList_itemSelectionChanged() {
    int selectedRow = mUi->pointList->currentRow();
//    printf("l-p-p: In itemSelectionChanged() for row %2d\n", selectedRow); // ??
    if (selectedRow < 0) return;

    mUi->pointList->setCurrentCell(selectedRow, 0, QItemSelectionModel::Rows);

    auto widgetItem = (PointWidgetItem*)(mUi->pointList->item(selectedRow, 0));
    auto pointElement = widgetItem->pointElement;

    if (mSelectedPointName == pointElement->protoFilePath) {
        // No change in the selection
        if (selectedRow >= 0) {
            mUi->pointList->setCurrentCell(selectedRow, 0); // (Helps if the user drags the selection)
            mUi->pointList->scrollToItem(mUi->pointList->currentItem());
        }
        return;
    }

    mSelectedPointName = pointElement->protoFilePath;
//    printf("l-p-p: Changed selection to %s\n", mSelectedPointName.toStdString().c_str()); // ??

    mLastLat = QString::number(pointElement->latitude, 'g', 12);
    mLastLng = QString::number(pointElement->longitude, 'g', 12);

    // Show the location, but do not send it to the device
    // ?? Has reduced precision!
    printf("l-p-p: Setting blue dot at \"%s\", \"%s\"\n", // ??
           mLastLat.toStdString().c_str(), mLastLng.toStdString().c_str()); // ??
    emit showLocation(mLastLat, mLastLng);

    // Redraw the table to show the new selection
    highlightPointListWidget();
}

void LocationPage::editPoint(int row) {
//    printf("In editPoint for row %d\n", row); // ??

// ?? Create a new validator class as a QDoubleValidator
//    and disable the Save button if the value is not good.

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    auto widgetItem = (PointWidgetItem*)(mUi->pointList->item(row, 0));
    auto pointElement = widgetItem->pointElement;

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = pointElement->logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = pointElement->description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);

    QPushButton *saveButton = buttonBox->addButton(QDialogButtonBox::Save);

#if 1 // ??
    // ?? Need validators for these.
    //    Perhaps have an "onTextChanged" listener and connect it to both
    //    Lat and Lng. In there, validate both fields and control the Save
    //    button.
    // Latitude
    dialogLayout->addWidget(new QLabel(tr("Latitude")));
    QLineEdit* latitudeEdit = new QLineEdit(this);
    QString oldLatText = QString::number(pointElement->latitude, 'g', 12);
    latitudeEdit->setText(oldLatText);
//??    latitudeEdit->setValidator(new LocationValidator(true, saveButton)); // ??
    dialogLayout->addWidget(latitudeEdit);

    // Longitude
    dialogLayout->addWidget(new QLabel(tr("Longitude")));
    QLineEdit* longitudeEdit = new QLineEdit(this);
    QString oldLngText = QString::number(pointElement->longitude, 'g', 12);
    longitudeEdit->setText(oldLngText);
//??    longitudeEdit->setValidator(new LocationValidator(false, saveButton)); // ??
    dialogLayout->addWidget(longitudeEdit);
#endif // ??

    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(tr("Edit point"));
    editDialog.setLayout(dialogLayout);

    QApplication::restoreOverrideCursor();

    int selection = editDialog.exec();

    printf("Received: Lat \"%s\", Lng \"%s\"\n", // ??
           latitudeEdit ->text().toStdString().c_str(), // ??
           longitudeEdit->text().toStdString().c_str());  // ??
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
        if (oldPointMetadata == nullptr) printf("l-p-p: editPoint(): Protobuf is bad!\n"); // ??
        if (oldPointMetadata == nullptr) return;

        emulator_location::PointMetadata pointMetadata(*oldPointMetadata);

        if (!newName.isEmpty()) {
            pointMetadata.set_logical_name(newName.toStdString().c_str());
        }
        pointMetadata.set_description(newDescription.toStdString().c_str());

        writePointProtobufFullPath(pointElement->protoFilePath, pointMetadata);
//        printf("l-p-p: editPoint(): Re-reading protobufs and repopulating everything\n"); // ??
        scanForPoints();
        populatePointListWidget();
        highlightPointListWidget();
        QApplication::restoreOverrideCursor();
    }
}

void LocationPage::deletePoint(int row) {
    if (row < 0 || row >= mPointList.size()) {
        printf("l-p-p: deletePoint(%d) Invalid row!\n", row); // ??
        return;
    }
//    printf("l-p-p: deletePoint(%d)\n", row); // ??
    auto widgetItem = (PointWidgetItem*)(mUi->pointList->item(row, 0));
    auto thisPoint = widgetItem->pointElement;

    QMessageBox msgBox(QMessageBox::Warning,
                       tr("Delete single point"),
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
            scanForPoints();
            mUi->pointList->setCurrentItem(nullptr);
            populatePointListWidget();
            highlightPointListWidget();
        }
        else printf("l-p-p: Could not get directory from path! \"%s\"\n",  protobufName.c_str()); // ??

        QApplication::restoreOverrideCursor();
    }
//    printf("l-p-p: deletePoint: Exiting\n"); // ??
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
    painter.drawText(HORIZ_PADDING, vertPosition, theItem->pointElement->description);

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

    QTableWidgetItem* item = new QTableWidgetItem();

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
        printf("location-page-point.cpp: writePointProtobuf: Failed to create path: %s\n", // ??
               protoFileDirectoryName.c_str()); // ??
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

void LocationPage::setUpWebEngine(QWebEnginePage* webEnginePage, const char* pageName) {
    QFile webChannelJsFile(":/html/js/qwebchannel.js");
    if(!webChannelJsFile.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
        return;
    }

    QByteArray webChannelJs = webChannelJsFile.readAll();
    webChannelJs.append(
            "\n"
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
                    "channel.objects.emulocationserver.showLocation.connect(function(lat, lng) {"
                        "if (showPendingLocation) {"
                            "showPendingLocation(lat, lng);"
                        "}"
                    "});"
                    "channel.objects.emulocationserver.locationChanged.connect(function(lat, lng) {"
                        "if (setDeviceLocation) {"
                            "setDeviceLocation(lat, lng);"
                        "}"
                    "});"
                    "channel.objects.emulocationserver.travelModeChanged.connect(function(mode) {"
                        "if (setTravelMode) {"
//                            "console.warn('l-p-p js:Calling setTravelMode()');" // ??
                            "setTravelMode(mode);"
                        "}"
//                        "else console.warn('l-p-p js:Cannot call setTravelMode()');" // ??
                    "});"
                    "channel.objects.emulocationserver.showRouteOnMap.connect(function(routeJson) {"
                        "if (setRouteOnMap) {"
                            "console.warn('l-p-p js:Calling setRouteOnMap()');" // ??
                            "setRouteOnMap(routeJson);"
                        "}"
                        "else console.warn('l-p-p js:Cannot call setRouteOnMap()');" // ??
                    "});"
                "});"
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

    webEnginePage->load(QUrl(pageName));
}
