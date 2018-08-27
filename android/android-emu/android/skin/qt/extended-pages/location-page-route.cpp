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
#include "android/location/Route.h"
#include "android/skin/qt/stylesheet.h"
#include "android/utils/path.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleValidator>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>

#include <fstream>

static const char PROTO_FILE_NAME[] = "route_metadata.pb";

void LocationPage::makeRouteProtobuf() { // ?? Debug only

    printf("l-p-r: In makeRouteProtobuf()\n"); // ??

    QDateTime now = QDateTime::currentDateTime();
    QString routeName("route_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::RouteMetadata myMetadata;
    myMetadata.set_logical_name(routeName.toStdString().c_str());
    myMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    myMetadata.set_mode_of_transportation(emulator_location::RouteMetadata_Mode_CAR); // = 2
    myMetadata.set_loop(false);
    myMetadata.set_speed_factor(emulator_location::RouteMetadata_PlaybackSpeed_SPEED_1x); // = 0
    myMetadata.set_description("One misty moisty morning, when cloudy was the weather...");
    myMetadata.set_duration(90);

    writeRouteProtobufByName(routeName, myMetadata);

    scanForRoutes();
    populateRouteListWidget();
    highlightRouteListWidget();
}

void LocationPage::on_saveRoute_clicked() {
    printf("location-page-route: saveRoute clicked\n"); // ??
    makeRouteProtobuf();
}

void LocationPage::on_playRouteButton_clicked() {
    printf("location-page-route: playRoute clicked\n"); // ??
}

// Populate mRouteList with the routes that are found on disk
void LocationPage::scanForRoutes() {
//    printf("l-p-p: >>>>> Starting scanForRoutes()\n"); // ??
    mRouteList.clear();

    // Get the directory
    std::string locationsDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations", "routes");

    if (!path_is_dir(locationsDirectoryName.c_str())) {
        // No "locations/routes" directory
        printf("l-p-p: In scanForRoutes(): No \"locations/routes\" directory\n"); // ??
        return;
    }

    // Find all the subdirectories in the locations directory
    QDir locationsDir(locationsDirectoryName.c_str());
    locationsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList routeList(locationsDir.entryList());

    // Look at all the directories and create an entry for each valid route
    for (const QString& routeName : routeList) {
        // Read the route protobuf
        std::string protoFilePath = android::base::PathUtils::
                join(locationsDirectoryName.c_str(),
                     routeName.toStdString().c_str(),
                     PROTO_FILE_NAME);

//        printf("l-p-r: scanning for Routes: checking \"%s\"\n", protoFilePath.c_str()); // ??
        android::location::Route route(protoFilePath.c_str());

        const emulator_location::RouteMetadata* routeMetadata = route.getProtoInfo();

//        if (routeMetadata == nullptr) printf("l-p-r:       No valid Route protobuf\n"); // ??
        if (routeMetadata == nullptr) continue;

        RouteListElement listElement;
        listElement.protoFilePath = protoFilePath.c_str();
        listElement.logicalName   = routeMetadata->logical_name().c_str();
        listElement.description   = routeMetadata->description().c_str();
        listElement.modeIndex     = routeMetadata->mode_of_transportation();
        listElement.speedFactor   = routeMetadata->speed_factor();
        listElement.loop          = routeMetadata->loop();
        listElement.duration      = routeMetadata->duration();

        mRouteList.append(listElement);
    }
}

// Populate the UI list of routes from mRouteList
void LocationPage::populateRouteListWidget() {
//    printf("l-p-r: In populateRouteListWidget\n"); // ??
    // Disable sorting while we're updating the table
    mUi->routeList->setSortingEnabled(false);

    // Set the dotdotdot column to take the available space. This
    // prevents the widget from scrolling horizontally due to an
    // unreasonably wide final column.
    // ?? The first time through, the widget width is not reported
    //    correctly. Thus this does not work correctly until the
    //    second time through.
//    printf("l.p.p Widths: col 0 %d, col 1 %d, widget %d\n", // ??
//           mUi->routeList->columnWidth(0), // ??
//           mUi->routeList->columnWidth(1), // ??
//           mUi->routeList->width()); // ??
    mUi->routeList->setColumnWidth(1,
            mUi->routeList->width() - mUi->routeList->columnWidth(0));

    int nItems = mRouteList.size();
    mUi->routeList->setRowCount(nItems);

    for (int idx = 0; idx < nItems; idx++) {
//        printf("l-p-r: List %2d: %s\n", idx, mRouteList[idx].logicalName.toStdString().c_str()); // ??
        mUi->routeList->setItem(idx, 0, new RouteWidgetItem(&mRouteList[idx]));
        mUi->routeList->setItem(idx, 1, new QTableWidgetItem());
    }

    // All done updating. Enable sorting now.
    mUi->routeList->sortByColumn(0, Qt::AscendingOrder);
    mUi->routeList->setSortingEnabled(true);

    // If the list is empty, show an overlay saying that.
    mUi->noSavedRoutes_mask->setVisible(nItems <= 0);

}

// Update the UI list of routes to highlight the
// row that is selected
void LocationPage::highlightRouteListWidget() {
//    printf("l-p-r: In higlightRoutesListWidget\n"); // ??
    // Update the QTableWidgetItems that are associated
    // with this widget
    for (int row = 0; row < mUi->routeList->rowCount(); row++) {
        RouteWidgetItem* routeItem = (RouteWidgetItem*)mUi->routeList->takeItem(row, 0);
        bool isSelected = (mSelectedRouteName == routeItem->routeElement->protoFilePath);

        mRouteItemBuilder->highlightRouteWidgetItem(routeItem, isSelected);
        mUi->routeList->setItem(row, 0, routeItem);

        QTableWidgetItem* dotDotItem = mUi->routeList->takeItem(row, 1);
        mRouteItemBuilder->highlightDotDotWidgetItem(dotDotItem, isSelected);
        mUi->routeList->setItem(row, 1, dotDotItem);

        if (isSelected) {
            mUi->routeList->setCurrentItem(routeItem);
        }
    }
    mUi->routeList->viewport()->repaint();
}

void LocationPage::on_routeList_cellClicked(int row, int column) {
    mUi->routeList->setCurrentCell(row, 0, QItemSelectionModel::Rows);
    if (column == 1) {
        QMenu* popMenu = new QMenu(this);
        QAction* editAction   = popMenu->addAction(tr("Edit"));
        QAction* deleteAction = popMenu->addAction(tr("Delete"));
        QAction* otherAction  = popMenu->addAction(tr("Some other action"));

        QAction* theAction = popMenu->exec(QCursor::pos());
        if (theAction == editAction) {
            editRoute(row);
        } else if (theAction == deleteAction) {
            deleteRoute(row);
        } else if (theAction == otherAction) {
            printf("Got SOME OTHER ACTION\n");
        }
    }
}

void LocationPage::on_routeList_itemSelectionChanged() {
    int selectedRow = mUi->routeList->currentRow();

    if (selectedRow < 0) {
        mUi->routeInfo->clear();
        return;
    }

    mUi->routeList->setCurrentCell(selectedRow, 0, QItemSelectionModel::Rows);

    auto widgetItem = (RouteWidgetItem*)(mUi->routeList->item(selectedRow, 0));
    auto routeElement = widgetItem->routeElement;

    if (mSelectedRouteName == routeElement->protoFilePath) {
        // No change in the selection
        if (selectedRow >= 0) {
            mUi->routeList->setCurrentCell(selectedRow, 0); // (Helps if the user drags the selection)
            mUi->routeList->scrollToItem(mUi->routeList->currentItem());
        }
        return;
    }

    mSelectedRouteName = routeElement->protoFilePath;

    // Redraw the table to show the new selection
    highlightRouteListWidget();
    showRouteDetails(routeElement);
}

void LocationPage::editRoute(int row) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    auto widgetItem = (RouteWidgetItem*)(mUi->routeList->item(row, 0));
    auto routeElement = widgetItem->routeElement;

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = routeElement->logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = routeElement->description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    // Mode of transportation
    dialogLayout->addWidget(new QLabel(tr("Mode")));
    QComboBox* transportMode = new QComboBox();
    transportMode->addItem(tr("None"));
    transportMode->addItem(getIconForCurrentTheme("bike"), tr("Bike"));
    transportMode->addItem(getIconForCurrentTheme("car"),  tr("Drive"));
    transportMode->addItem(getIconForCurrentTheme("run"),  tr("Run"));
    transportMode->addItem(getIconForCurrentTheme("walk"), tr("Walk"));
    transportMode->setCurrentIndex(routeElement->modeIndex);
    dialogLayout->addWidget(transportMode);

    // Playback speed
    dialogLayout->addWidget(new QLabel(tr("Playback speed")));
    QComboBox* playbackSpeed = new QComboBox();
    playbackSpeed->addItem(tr("1x"));
    playbackSpeed->addItem(tr("2x"));
    playbackSpeed->addItem(tr("3x"));
    playbackSpeed->addItem(tr("4x"));
    playbackSpeed->addItem(tr("5x"));
    playbackSpeed->setCurrentIndex(routeElement->speedFactor);
    dialogLayout->addWidget(playbackSpeed);

    // "Loop" checkbox
    QCheckBox* loopCheckBox = new QCheckBox(tr("Repeat on playback"));
    loopCheckBox->setChecked(routeElement->loop);
    dialogLayout->addWidget(loopCheckBox);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save |
                                                       QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(tr("Edit route"));
    editDialog.setLayout(dialogLayout);

    QApplication::restoreOverrideCursor();

    int selection = editDialog.exec();
    if (selection == QDialog::Rejected) {
        return;
    }

    QString newName = nameEdit->text();
    QString newDescription = descriptionEdit->toPlainText();

    if ((newName.isEmpty() || newName == oldName)                  &&
        newDescription                == oldDescription            &&
        transportMode->currentIndex() == routeElement->modeIndex   &&
        playbackSpeed->currentIndex() == routeElement->speedFactor &&
        loopCheckBox->isChecked()     == routeElement->loop          )
    {
        return;
    }
    // Something changed. Read the existing Protobuf,
    // update it, and write it back out.
    QApplication::setOverrideCursor(Qt::WaitCursor);

    android::location::Route route(routeElement->protoFilePath.toStdString().c_str());

    const emulator_location::RouteMetadata* oldRouteMetadata = route.getProtoInfo();
    if (oldRouteMetadata == nullptr) printf("l-p-r: editRoute(): Protobuf is bad!\n"); // ??
    if (oldRouteMetadata == nullptr) return;

    emulator_location::RouteMetadata routeMetadata(*oldRouteMetadata);

    if (!newName.isEmpty()) {
        routeMetadata.set_logical_name(newName.toStdString().c_str());
    }
    routeMetadata.set_description(newDescription.toStdString().c_str());
    routeMetadata.set_mode_of_transportation(
            (emulator_location::RouteMetadata_Mode)transportMode->currentIndex());
    routeMetadata.set_speed_factor(
            (emulator_location::RouteMetadata_PlaybackSpeed)playbackSpeed->currentIndex());
    routeMetadata.set_loop(loopCheckBox->isChecked());

    writeRouteProtobufFullPath(routeElement->protoFilePath, routeMetadata);
    scanForRoutes();
    populateRouteListWidget();
    highlightRouteListWidget();
    showRouteDetails(routeElement);
    QApplication::restoreOverrideCursor();
}

void LocationPage::deleteRoute(int row) {
    if (row < 0 || row >= mRouteList.size()) {
        printf("l-p-r: deleteRoute(%d) Invalid row!\n", row); // ??
        return;
    }
//    printf("l-p-r: deleteRoute(%d)\n", row); // ??
    auto widgetItem = (RouteWidgetItem*)(mUi->routeList->item(row, 0));
    auto thisRoute = widgetItem->routeElement;

    QMessageBox msgBox(QMessageBox::Warning,
                       tr("Delete route"),
                       tr("Do you want to permanently delete<br>route \"%1\"?")
                               .arg(thisRoute->logicalName),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        std::string protobufName = thisRoute->protoFilePath.toStdString();
        android::base::StringView dirName;
        bool haveDirName = android::base::PathUtils::split(protobufName,
                                                           &dirName,
                                                           nullptr /* base name */);
        if (haveDirName) {
            path_delete_dir(dirName.str().c_str());
            mSelectedRouteName.clear();
            scanForRoutes();
            mUi->routeList->setCurrentItem(nullptr);
            populateRouteListWidget();
            highlightRouteListWidget();
        }
        else printf("l-p-r: Could not get directory from path! \"%s\"\n",  protobufName.c_str()); // ??

        QApplication::restoreOverrideCursor();
    }
//    printf("l-p-r: deleteRoute: Exiting\n"); // ??
}

void
LocationPage::RouteItemBuilder::highlightRouteWidgetItem(LocationPage::RouteWidgetItem* theItem,
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

    int gapToCenterVertically = (mFieldHeight - (bigFontHeight + TEXT_SEPARATION + baseFontHeight)) / 2;
    QRectF textRectangle(HORIZ_PADDING, gapToCenterVertically,
                        mFieldWidth - 3*HORIZ_PADDING - ICON_SIZE, mFieldHeight);
    painter.drawText(textRectangle, Qt::AlignLeft, theItem->routeElement->logicalName);

    painter.setFont(baseFont);
    textRectangle.setTop(textRectangle.top() + bigFontHeight + TEXT_SEPARATION);
    painter.drawText(textRectangle, Qt::AlignLeft, theItem->routeElement->description);

    QString modeIconName;
    switch (theItem->routeElement->modeIndex) {
        default:
        case 0:  modeIconName = "";     break;
        case 1:  modeIconName = "bike"; break;
        case 2:  modeIconName = "car";  break;
        case 3:  modeIconName = "run";  break;
        case 4:  modeIconName = "walk"; break;
    }
    QIcon modeIcon = getIconForCurrentTheme(modeIconName);
    QPixmap transportIcon = modeIcon.pixmap(ICON_SIZE, ICON_SIZE);
    int vertPosition = (mFieldHeight - ICON_SIZE) / 2;
    int horizPosition = mFieldWidth - ICON_SIZE;
    horizPosition -= HORIZ_PADDING;
    painter.drawPixmap(QRect(horizPosition, vertPosition, ICON_SIZE, ICON_SIZE), transportIcon);

    theItem->setIcon(basePixmap);
}

void LocationPage::RouteItemBuilder::highlightDotDotWidgetItem(QTableWidgetItem* dotDotItem, bool isSelected) {
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

// Display the details of the selected route
void LocationPage::showRouteDetails(const RouteListElement* theElement) {
    // Show the route info in the details window.
    QString modeString;
    switch (theElement->modeIndex) {
        default:
        case 0:  modeString = "";      break;
        case 1:  modeString = "Bike";  break;
        case 2:  modeString = "Drive"; break;
        case 3:  modeString = "Run";   break;
        case 4:  modeString = "Walk";  break;
    }
    std::string protobufName = theElement->protoFilePath.toStdString();
    android::base::StringView dirPath;
    android::base::StringView dirTail;
    bool splitSucceeded = android::base::PathUtils::split(protobufName,
                                                          &dirPath,
                                                          nullptr /* base name */);
    if (!splitSucceeded) {
        dirTail = "";
    } else {
        // Remove the trailing path separator
        dirPath = dirPath.substrAbs(0, dirPath.size() - 1);

        splitSucceeded = android::base::PathUtils::split(dirPath.str(),
                                                         nullptr, /* start of path */
                                                         &dirTail);
        if (!splitSucceeded) {
            dirTail = "";
        }
    }
    int durationSeconds = theElement->duration;
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
    QString infoString = tr("<b>%1</b><br>"
                            "&nbsp;&nbsp;Mode: <b>%2</b><br>"
                            "&nbsp;&nbsp;Playback speed: <b>%3x</b><br>"
                            "&nbsp;&nbsp;Repeat on playback: <b>%4</b><br>"
                            "&nbsp;&nbsp;Duration: <b>%5</b><br>"
                            "&nbsp;&nbsp;Directory: <b>%6</b><br>"
                            "<b>%7</b>")
                          .arg(theElement->logicalName)
                          .arg(modeString)
                          .arg(theElement->speedFactor + 1)
                          .arg(theElement->loop ? "yes" : "no")
                          .arg(durationString)
                          .arg(dirTail.str().c_str())
                          .arg(theElement->description);
    mUi->routeInfo->setHtml(infoString);
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
        printf("location-page-route.cpp: writeRouteProtobuf: Failed to create path: %s\n", // ??
               protoFileDirectoryName.c_str()); // ??
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
    printf("location-page-route.cpp: writeRouteProtobufFulLPath to \"%s\"\n", // ??
           protoFullPath.toStdString().c_str()); // ??
    std::ofstream outStream(protoFullPath.toStdString().c_str(), std::ofstream::binary);
    protobuf.SerializeToOstream(&outStream);
}
