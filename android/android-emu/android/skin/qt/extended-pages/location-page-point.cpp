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
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>

#include <fstream>

static const char PROTO_FILE_NAME[] = "point_metadata.pb";

void LocationPage::makePointProtobuf() { // ?? Debug only
//    printf("l-p-p: In makePointProtobuf()\n"); // ??

    QDateTime now = QDateTime::currentDateTime();
    QString pointName("point_" + now.toString("yyyy-MM-dd_HH-mm-ss"));

    emulator_location::PointMetadata ptMetadata;
    QString logicalName = "Point " + pointName.right(5);
    ptMetadata.set_logical_name(logicalName.toStdString());
    ptMetadata.set_creation_time(now.toMSecsSinceEpoch() / 1000LL);
    ptMetadata.set_description("One misty moisty morning, when cloudy was the weather...");
    ptMetadata.set_latitude(22.2222);
    ptMetadata.set_longitude(-123.4567);
    ptMetadata.set_altitude(11.1111);

    writePointProtobufByName(pointName, ptMetadata);
}

// Populate mPointList with the points that are found on disk
void LocationPage::scanForPoints() {

//    printf("l-p-p: >>>>> Starting scanForPoints()\n"); // ??
    mPointList.clear();

    // Get the directory
    std::string pointDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations");

    if (!path_is_dir(pointDirectoryName.c_str())) {
        // No "locations/" directory
        printf("l-p-p: In scanForPoints(): No \"locations/\" directory\n"); // ??
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
//        printf("    Pbuf name \"%s\"\n", protoFilePath.c_str()); // ??

        android::location::Point point(protoFilePath.c_str());

        const emulator_location::PointMetadata* pointMetadata = point.getProtoInfo();

        if (pointMetadata == nullptr) continue;

        pointListElement listElement;
        listElement.protoFilePath = protoFilePath.c_str();
        listElement.logicalName = pointMetadata->logical_name().c_str();
        listElement.description = pointMetadata->has_description() ?
                                          pointMetadata->description().c_str() : "";

        mPointList.append(listElement);
    }
//    printf("l-p-p: <<<<< Exit scanForPoints()\n"); // ??
}

// Populate the UI list of point from mPointList
void LocationPage::populatePointListTable() {
    // Set the dotdotdot column to take the available space. This prevents
    // horizontal scrolling for an unreasonably wide final column.
    mUi->pointList->setColumnWidth(1,
            mUi->pointList->width() - mUi->pointList->columnWidth(0));

    int nItems = mPointList.size();

    int selectedRow = std::min(mUi->pointList->currentRow(), nItems - 1);
    if (selectedRow >= 0) {
        mUi->pointList->setCurrentCell(selectedRow, 0); // (Helps if the user drags the selection)
        mUi->pointList->scrollToItem(mUi->pointList->currentItem());
    }
    mUi->pointList->setRowCount(nItems);

    int itemIdx = 0;
    for (auto listItem : mPointList) {
        mUi->pointList->setItem(itemIdx, 0,
                                    mPointItemBuilder->pointItem(
                                            listItem.logicalName,
                                            listItem.description,
                                            QIcon(),
                                            itemIdx == selectedRow));

        mUi->pointList->setItem(itemIdx, 1,
                                    mPointItemBuilder->dotDotItem(itemIdx == selectedRow));
        itemIdx++;
    }
}

void LocationPage::on_pointList_cellPressed(int row, int column) {
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
    if (mUi->pointList->currentRow() < 0) return;

    populatePointListTable();
}

void LocationPage::editPoint(int pointIdx) {
    printf("In editPoint for item %d\n", pointIdx);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    // Name
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = mPointList[pointIdx].logicalName;
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    // Description
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = mPointList[pointIdx].description;
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save |
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
        printf("l-p-p: Edit: canceled\n"); // ??
        return;
    }

    QString newName = nameEdit->text();
    QString newDescription = descriptionEdit->toPlainText();

    if ((!newName.isEmpty() && newName != oldName) ||
        newDescription != oldDescription             )
    {
//        printf("l-p-p: Edit: Something changed...\n"); // ??
        // Something changed. Read the existing Protobuf,
        // update it, and write it back out.
        QApplication::setOverrideCursor(Qt::WaitCursor);

        android::location::Point point(mPointList[pointIdx].protoFilePath.toStdString().c_str());

        const emulator_location::PointMetadata* oldPointMetadata = point.getProtoInfo();
        if (oldPointMetadata == nullptr) printf("l-p-p: editPoint(): Protobuf is bad!\n"); // ??
        if (oldPointMetadata == nullptr) return;

        emulator_location::PointMetadata pointMetadata(*oldPointMetadata);

        if (!newName.isEmpty()) {
            pointMetadata.set_logical_name(newName.toStdString().c_str());
        }
        pointMetadata.set_description(newDescription.toStdString().c_str());

        writePointProtobufFullPath(mPointList[pointIdx].protoFilePath, pointMetadata);
        scanForPoints();
        populatePointListTable();
        QApplication::restoreOverrideCursor();
    }
//    else printf("l-p-p: Edit: Nothing new\n"); // ??
}

void LocationPage::deletePoint(int pointIdx) {
    if (pointIdx < 0 || pointIdx >= mPointList.size()) {
        printf("l-p-p: deletePoint(%d) Invalid index!\n", pointIdx); // ??
        return;
    }
    printf("l-p-p: deletePoint(%d)\n", pointIdx); // ??

    auto thisPoint = mPointList.at(pointIdx);

    QMessageBox msgBox(QMessageBox::Question,
                       tr("Delete single point"),
                       tr("Do you want to permanently delete<br>point \"%1\"?")
                               .arg(thisPoint.logicalName),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        std::string protobufName = thisPoint.protoFilePath.toStdString();
        android::base::StringView dirName;
        bool haveDirName = android::base::PathUtils::split(protobufName,
                                                           &dirName, nullptr /* base name */);
        if (haveDirName) {
            printf("Deleting \"%s\" ...\n", dirName.str().c_str()); // ??
            path_delete_dir(dirName.str().c_str());

            scanForPoints();
            printf("l-p-p: deletePoint() calling populatePointListTable()\n"); // ??
            populatePointListTable();
        }
        else printf("l-p-p: Could not get directory from path! \"%s\"\n",  protobufName.c_str()); // ??

        QApplication::restoreOverrideCursor();
    }
}
QTableWidgetItem* LocationPage::PointItemBuilder::pointItem(QString name,
                                                            QString description,
                                                            const QIcon& icon,
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
    painter.drawText(HORIZ_PADDING, vertPosition, name);

    painter.setFont(baseFont);
    vertPosition += bigFontHeight + TEXT_SEPARATION;
    painter.drawText(HORIZ_PADDING, vertPosition, description);

    QPixmap transportIcon = icon.pixmap(ICON_SIZE, ICON_SIZE);

    vertPosition = (mFieldHeight - ICON_SIZE) / 2;

    int horizPosition = mFieldWidth - ICON_SIZE;

    horizPosition -= HORIZ_PADDING;

    painter.drawPixmap(QRect(horizPosition, vertPosition, ICON_SIZE, ICON_SIZE), transportIcon);

    QTableWidgetItem* item = new QTableWidgetItem();
    item->setIcon(basePixmap);

    return item;
}

QTableWidgetItem* LocationPage::PointItemBuilder::dotDotItem(bool isSelected) {
    QColor backgroundColor = isSelected ? getColorForCurrentTheme(Ui::TABLE_SELECTED_VAR)
                                        : getColorForCurrentTheme(Ui::TAB_BKG_COLOR_VAR);

    QPixmap basePixmap(ICON_SIZE, ICON_SIZE);
    basePixmap.fill(backgroundColor);
    QPainter painter(&basePixmap);

    QPixmap dotDotIcon = getIconForCurrentTheme("more_vert").pixmap(ICON_SIZE, ICON_SIZE);

    painter.drawPixmap(QRect(0, 0, ICON_SIZE, ICON_SIZE), dotDotIcon);

    QTableWidgetItem* item = new QTableWidgetItem();
    item->setIcon(basePixmap);

    return item;
}

void LocationPage::writePointProtobufByName(
        const QString& pointFormalName,
        const emulator_location::PointMetadata& protobuf)
{
    // Get the directory to hold the protobuf
    std::string protoFileDirectoryName = android::base::PathUtils::
            join(::android::ConfigDirs::getUserDirectory().c_str(),
                 "locations",
                 pointFormalName.toStdString().c_str());

    // Ensure that the directory exists
    if (path_mkdir_if_needed(protoFileDirectoryName.c_str(), 0755)) {
        // Fail
        printf("location-page-point.cpp: writePointProtobuf: Failed to create path: %s\n", // ??
               protoFileDirectoryName.c_str()); // ??
    } else {
        // The path exists now.
        // Construct the full file path
        std::string protoFilePath = android::base::PathUtils::
                join(protoFileDirectoryName.c_str(), PROTO_FILE_NAME);

        writePointProtobufFullPath(protoFilePath.c_str(), protobuf);
    }
}

void LocationPage::writePointProtobufFullPath(
        const QString& protoFullPath,
        const emulator_location::PointMetadata& protobuf)
{
    std::ofstream outStream(protoFullPath.toStdString().c_str(), std::ofstream::binary);
    protobuf.SerializeToOstream(&outStream);
}
