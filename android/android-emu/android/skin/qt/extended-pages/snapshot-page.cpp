// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/snapshot-page.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/ui.h"
#include "android/snapshot/interface.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/proto/snapshot.pb.h"
#include "android/snapshot/Snapshot.h"

#include "android/skin/qt/stylesheet.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QGraphicsPixmapItem>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSemaphore>
#include <QVBoxLayout>

#include <fstream>

using namespace android::base;
using namespace android::snapshot;

SnapshotPage::SnapshotPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::SnapshotPage())
{
    mUi->setupUi(this);
    mUi->snapshotDisplay->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);

    populateSnapshotDisplay();

    if (mUi->snapshotDisplay->currentItem() == nullptr) {
        mUi->snapshotDisplay->setCurrentItem(mUi->snapshotDisplay->topLevelItem(0));
    }

    QObject::connect(this, SIGNAL(saveCompleted(int, QString)),
                     this, SLOT(slot_snapshotSaveCompleted(int, QString)));

    QObject::connect(this, SIGNAL(deleteCompleted()),
                     this, SLOT(slot_snapshotDeleteCompleted()));
}

void SnapshotPage::on_snapshotDisplay_itemClicked(QTreeWidgetItem* theItem, int theColumn) {
    if (theColumn == COLUMN_DOTDOT) {
        WidgetSnapshotItem* theSnapItem = static_cast<WidgetSnapshotItem*>(theItem);

        // Show the pop-up menu
        QMenu menu;
        QAction* actionClone  = nullptr;
//        QAction* actionEdit   = nullptr;

//        QAction* actionLoad   = menu.addAction(tr("Load"));
        if (theSnapItem->fileName() == "default_boot") {
            actionClone = menu.addAction(tr("Clone"));
//        } else {
//            actionEdit   = menu.addAction(tr("Edit"));
        }
        QAction* actionExport = menu.addAction(tr("Export"));
//        QAction* actionDelete = menu.addAction(tr("Delete"));

        QAction* theAction = menu.exec(QCursor::pos());

        QString simpleName = theSnapItem->data(COLUMN_NAME, Qt::DisplayRole).toString();

        if (theAction == nullptr) {
            // Some of the 'action*' items are null.
            // This prevents us from matching them.
            ;
        } else if (theAction == actionClone) {
            cloneSnapshot(theSnapItem);
//        } else if (theAction == actionDelete) {
//            deleteSnapshot(theSnapItem);
//        } else if (theAction == actionEdit) {
//            editSnapshot(theSnapItem);
        } else if (theAction == actionExport) {
            exportSnapshot(theSnapItem);
//        } else if (theAction == actionLoad) {
//            loadSnapshot(theSnapItem);
        }
    }
}

void SnapshotPage::deleteSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }

    QString logicalName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
    QString fileName = theItem->fileName();

    QMessageBox msgBox(QMessageBox::Question,
                       tr("Delete snapshot"),
                       tr("Do you want to delete snapshot \"") + logicalName + "\"",
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        android::base::ThreadLooper::runOnMainLooper([fileName, this] {
            androidSnapshot_delete(fileName.toStdString().c_str());
            emit(deleteCompleted());
        });
    }
}

void SnapshotPage::slot_snapshotDeleteCompleted() {
    populateSnapshotDisplay();
}

void SnapshotPage::exportSnapshot(WidgetSnapshotItem* theItem) {
    printf("snapshot-page.cpp: exportSnapshot() is not implemented\n");
}

void SnapshotPage::editSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
    nameEdit->setText(oldName);
    dialogLayout->addWidget(nameEdit);

    dialogLayout->addWidget(new QLabel(tr("Description")));
    QTextEdit* descriptionEdit = new QTextEdit(this);
    QString oldDescription = getDescription(theItem->fileName());
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save |
                                                       QDialogButtonBox::Cancel,
                                                       Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog editDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &editDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &editDialog, SLOT(accept()));

    editDialog.setWindowTitle(tr("Edit snapshot"));
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

        QString fileName = theItem->fileName();
        emulator_snapshot::Snapshot* protobuf = loadProtobuf(fileName);

        if (protobuf != nullptr) {
            if (!newName.isEmpty()) {
                protobuf->set_logical_name(newName.toStdString());
            }
            protobuf->set_description(newDescription.toStdString());
            writeProtobuf(fileName, protobuf);

            populateSnapshotDisplay();

            // Select the just-edited item
            QTreeWidgetItemIterator iter(mUi->snapshotDisplay);
            for ( ; *iter; iter++) {
                WidgetSnapshotItem* theSnapItem = static_cast<WidgetSnapshotItem*>(*iter);
                if (fileName == theSnapItem->fileName()) {
                    mUi->snapshotDisplay->setCurrentItem(theSnapItem);
                    break;
                }
            }
        }
        QApplication::restoreOverrideCursor();
    }
}

void SnapshotPage::cloneSnapshot(WidgetSnapshotItem* theItem) {

    QString oldName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
    QString defaultName("snap_"
                         + theItem->dateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    QString mainText = tr("Current name:\n") + oldName +
                       tr("\n\nNew name:");
    bool ok;
    QString newName = QInputDialog::getText(this,
                                            tr("Clone snapshot"),
                                            mainText,
                                            QLineEdit::Normal,
                                            defaultName,
                                            &ok);
    if (ok && !newName.isEmpty()) {
        printf("snapshot-page.cpp: This is where I would clone the snapshot\n"); // ??
    }
}

QString SnapshotPage::getDescription(QString fileName) {
    Snapshot theSnapshot(fileName.toStdString().c_str());
    const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
    if (protobuf == nullptr) return QString();

    return QString(protobuf->description().c_str());
}

const SnapshotPage::WidgetSnapshotItem* SnapshotPage::getSelectedSnapshot() {
    const QList<QTreeWidgetItem *> selectedItems = mUi->snapshotDisplay->selectedItems();

    if (selectedItems.size() == 0) {
        // Nothing selected
        return nullptr;
    }
    // Return the first selected element--multiple selections are not allowed
    return static_cast<WidgetSnapshotItem*>(selectedItems[0]);
}

void SnapshotPage::on_editSnapshot_clicked() {
    editSnapshot(getSelectedSnapshot());
}

void SnapshotPage::on_deleteSnapshot_clicked() {
    deleteSnapshot(getSelectedSnapshot());
}

void SnapshotPage::on_loadSnapshot_clicked() {
    const WidgetSnapshotItem* theItem = getSelectedSnapshot();
    if (!theItem) return;

    QString fileName = theItem->fileName();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    androidSnapshot_load(fileName.toStdString().c_str());
    populateSnapshotDisplay();
    QApplication::restoreOverrideCursor();
}

void SnapshotPage::on_snapshotDisplay_itemSelectionChanged() {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    const QList<QTreeWidgetItem *> selectedItems = mUi->snapshotDisplay->selectedItems();

    if (selectedItems.size() == 0) {
        if (mUi->snapshotDisplay->topLevelItemCount() > 0) {
            mUi->selectionInfo->setHtml(tr("<big>No snapshot selected"));
        } else {
            // Nothing can be selected
            mUi->selectionInfo->setHtml("");
        }
        QApplication::restoreOverrideCursor();
        return;
    }
    // Grab the first selected element--multiple selections are not allowed
    const WidgetSnapshotItem* theItem = static_cast<const WidgetSnapshotItem*>(selectedItems[0]);

    QString logicalName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();

    QString simpleName = theItem->fileName();

    const char *avdPath = avdInfo_getContentPath(android_avdInfo);
    QString snapshotPath(PathUtils::join(avdPath, "snapshots").c_str());
    // I see Snapshot::diskSize() always returning 999, so I calculate the size myself.
    qint64 snapSize = recursiveSize(snapshotPath, simpleName);

    QString selectionInfoString =
        + "<big><b>" + logicalName + "</b>";

    if (simpleName == androidSnapshot_loadedSnapshotFile()) {
        selectionInfoString += tr("<p><b>This image was used to start the current AVD instance.</b>");
    }
    QString dateTimeString = theItem->dateTime().isValid() ?
                                   theItem->dateTime().toString(Qt::SystemLocaleLongDate) :
                                   tr("(unknown)");

    selectionInfoString += tr("<p>Captured ") + dateTimeString
                           + "<p>" + formattedSize(snapSize) + tr(" on disk");
    if (logicalName != simpleName) {
        selectionInfoString += tr(" under ") + simpleName;
    }
    if (!theItem->parentName().isEmpty()) {
        selectionInfoString += "<p>Derived from <b>" + theItem->parentName() + "</b>";
    }
    QString description = getDescription(simpleName);
    if (!description.isEmpty()) {
        selectionInfoString += "<p><pre>" + description + "</pre>";
    }
    mUi->selectionInfo->setHtml(selectionInfoString);

    if (mUi->preview->isVisible()) {
        showPreviewImage(snapshotPath, simpleName);
    }

    QApplication::restoreOverrideCursor();
}

// Displays the preview image of the selected snapshot
void SnapshotPage::showPreviewImage(QString snapshotPath, QString snapshotName) {
    mPreviewScene.clear(); // Frees previewItem

    QPixmap pMap(PathUtils::join(snapshotPath.toStdString(),
                                 snapshotName.toStdString(),
                                 "screenshot.png").c_str());

    QGraphicsPixmapItem *previewItem = new QGraphicsPixmapItem(pMap);
    mPreviewScene.addItem(previewItem);

    mUi->preview->setScene(&mPreviewScene);
    mUi->preview->fitInView(previewItem, Qt::KeepAspectRatio);
}

void SnapshotPage::showEvent(QShowEvent* ee) {
    if (!mUi->preview->isVisible()) {
        return;
    }
    // (Re)assign the dot-dot-dot icon here in case the theme changed
    QIcon iconMore = getIconForCurrentTheme("more_vert");
    for (auto snapshotItem : mUi->snapshotDisplay->
            findItems("", Qt::MatchStartsWith|Qt::MatchRecursive, COLUMN_NAME))
    {
        snapshotItem->setIcon(COLUMN_DOTDOT, iconMore);
    }
    // The snapshot preview image does not render correctly if its
    // widget isn't visible. Now that the widget is visible, we
    // can show that image.
    const WidgetSnapshotItem* theItem = getSelectedSnapshot();
    if (!theItem) return;
    QString simpleName = theItem->fileName();

    const char *avdPath = avdInfo_getContentPath(android_avdInfo);
    QString snapshotPath(PathUtils::join(avdPath, "snapshots").c_str());

    showPreviewImage(snapshotPath, simpleName);
}

void SnapshotPage::on_takeSnapshotButton_clicked() {
    AndroidSnapshotStatus status;

    QString snapshotName("snap_"
                         + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    android::base::ThreadLooper::runOnMainLooper([&status, snapshotName, this] {
        status = androidSnapshot_save(snapshotName.toStdString().c_str());
        emit(saveCompleted((int)status, snapshotName));
    });
}

void SnapshotPage::slot_snapshotSaveCompleted(int statusInt, QString snapshotName) {
    AndroidSnapshotStatus status = (AndroidSnapshotStatus)statusInt;

    if (status != SNAPSHOT_STATUS_OK) {
        QApplication::restoreOverrideCursor();
        printf("snapshot-page.cpp: androidSnapshot_save() failed with %d\n", status);
        return;
    }

    // If there was a loaded snapshot, set that snapshot as the parent
    // of this one.
    const char* parentName = androidSnapshot_loadedSnapshotFile();
    if (parentName && parentName[0] != '\0' && strcmp(parentName, "default_boot")) {
        writeParentToProtobuf(snapshotName, QString(parentName));
    }

    // Refresh the list of available snapshots
    populateSnapshotDisplay();

    // Highlight the new snapshot
    int itemCount = mUi->snapshotDisplay->topLevelItemCount();
    for (int idx = 0; idx < itemCount; idx++) {
        QTreeWidgetItem *item = mUi->snapshotDisplay->topLevelItem(idx);
        if (snapshotName == item->data(COLUMN_NAME, Qt::DisplayRole).toString()) {
            mUi->snapshotDisplay->setCurrentItem(item);
            break;
        }
    }
    QTreeWidgetItemIterator iter(mUi->snapshotDisplay);
    for ( ; *iter; iter++) {
        WidgetSnapshotItem* theSnapItem = static_cast<WidgetSnapshotItem*>(*iter);
        if (snapshotName == theSnapItem->fileName()) {
            mUi->snapshotDisplay->setCurrentItem(theSnapItem);
            break;
        }
    }
}

void SnapshotPage::populateSnapshotDisplay() {

    mUi->snapshotDisplay->setSortingEnabled(false); // Don't sort during modification
    mUi->snapshotDisplay->clear();

    // Temporary structure for organizing the display of the snapshots
    class treeItem {
        public:
            QString             fileName;
            QString             logicalName;
            QDateTime           dateTime;
            QString             parent;
            int                 parentIdx;
            WidgetSnapshotItem* item;
    };

    std::string snapshotPath = getSnapshotBaseDir();

    // Find all the directories in the snapshot directory
    QDir snapshotDir(snapshotPath.c_str());
    snapshotDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList snapshotList(snapshotDir.entryList());

    int nItems = snapshotList.size();
    treeItem itemArray[nItems];

    // Look at all the directories and make a treeItem for each one
    int idx = 0;
    for (QString fileName : snapshotList) {

        Snapshot theSnapshot(fileName.toStdString().c_str());

        const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
        if (protobuf == nullptr) continue;
        QString logicalName;
        if (protobuf->has_logical_name()) {
            logicalName = protobuf->logical_name().c_str();
        } else {
            logicalName = fileName;
        }

        QString parentName = "";
        if (protobuf->has_parent()) {
            parentName = protobuf->parent().c_str();
        }

        QDateTime snapshotDate;
        if (protobuf->has_creation_time()) {
            snapshotDate = QDateTime::fromMSecsSinceEpoch(1000LL * protobuf->creation_time());
        }
        itemArray[idx].fileName = fileName;
        itemArray[idx].logicalName = logicalName;
        itemArray[idx].dateTime = snapshotDate;
        itemArray[idx].parent = parentName;
        itemArray[idx].parentIdx = -1; // Haven't found the parent's item (yet)
        itemArray[idx].item = nullptr; // No QTreeWidgetItem yet
        idx++;
    }
    nItems = idx;

    if (nItems <= 0) {
        // Hide the tree display and say that there are no snapshots
        mUi->snapshotDisplay->setVisible(false);
        mUi->noneAvailableLabel->setVisible(true);
        return;
    }
    mUi->snapshotDisplay->setVisible(true);
    mUi->noneAvailableLabel->setVisible(false);

    // Find the index corresponding to each parent
    for (int idx = 0; idx < nItems; idx++) {
        if (!itemArray[idx].parent.isEmpty()) {
            // Search for this item's parent
            const QString parentName = itemArray[idx].parent;
            for (int parentIdx = 0; parentIdx < nItems; parentIdx++) {
                if (parentName == itemArray[parentIdx].fileName) {
                    itemArray[idx].parentIdx = parentIdx;
                    break;
                }
            }
        }
    }

    // Loop through the array, creating any items that we can
    bool didSomething;
    do {
        didSomething = false;
        for (int idx = 0; idx < nItems; idx++) {
            if (itemArray[idx].item != nullptr) {
                // Already created an item for this array entry
                continue;
            }

            WidgetSnapshotItem* thisItem;
            if (itemArray[idx].parentIdx < 0) {
                // This snapshot has no parent or its parent was not found.
                // Create a top-level item.
                thisItem = new WidgetSnapshotItem(itemArray[idx].fileName,
                                                  itemArray[idx].logicalName,
                                                  itemArray[idx].dateTime);
                mUi->snapshotDisplay->addTopLevelItem(thisItem);
            } else if (itemArray[itemArray[idx].parentIdx].item != nullptr) {
                // The parent was found and its item was created.
                // Create a child item.
                thisItem = new WidgetSnapshotItem(itemArray[itemArray[idx].parentIdx].item,
                                                  itemArray[idx].fileName,
                                                  itemArray[idx].logicalName,
                                                  itemArray[idx].dateTime);
            } else {
                // This snapshot's parent does not exist as an 'item' yet,
                // so we cannot create the item for this snapshot yet.
                continue;
            }
            // We created an item for this snapshot
            itemArray[idx].item = thisItem;

            if (itemArray[idx].fileName ==
                    androidSnapshot_loadedSnapshotFile())
            {
                // This snapshot was used to start the AVD
                QFont bigBold = thisItem->font(COLUMN_NAME);
                bigBold.setPointSize(bigBold.pointSize() + 2);
                bigBold.setBold(true);
                thisItem->setFont(COLUMN_NAME, bigBold);
                mUi->snapshotDisplay->setCurrentItem(thisItem);
            } else {
                QFont biggish = thisItem->font(COLUMN_NAME);
                biggish.setPointSize(biggish.pointSize() + 1);
                thisItem->setFont(COLUMN_NAME, biggish);
            }
            didSomething = true;
        }
    } while (didSomething);

    mUi->snapshotDisplay->header()->setStretchLastSection(false);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_DOTDOT, QHeaderView::Fixed);
    mUi->snapshotDisplay->header()->resizeSection(COLUMN_DOTDOT, 20);
    mUi->snapshotDisplay->setSortingEnabled(true);
}


void SnapshotPage::writeParentToProtobuf(QString fileName, QString parentName) {

    emulator_snapshot::Snapshot* protobuf = loadProtobuf(fileName);

    if (protobuf != nullptr) {
        protobuf->set_parent(parentName.toStdString());
        writeProtobuf(fileName, protobuf);
    }
}

emulator_snapshot::Snapshot* SnapshotPage::loadProtobuf(QString fileName) {
    Snapshot theSnapshot(fileName.toStdString().c_str());

    const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
    if (protobuf == nullptr) {
        printf("*** Cannot update protobuf: it is null!\n"); // ??
        return nullptr;
    }

    // Make a writable copy
    emulator_snapshot::Snapshot* newProtobuf = new emulator_snapshot::Snapshot(*protobuf);
    return newProtobuf;
}

void SnapshotPage::writeProtobuf(QString fileName, emulator_snapshot::Snapshot* protobuf) {
    std::string protoFileName = PathUtils::join(getSnapshotBaseDir().c_str(),
                                                fileName.toStdString().c_str(),
                                                "snapshot.pb");
    std::filebuf fileBuf;
    fileBuf.open(protoFileName.c_str(), std::ios::out);
    std::ostream outStream(&fileBuf);

    protobuf->SerializeToOstream(&outStream);
}

qint64 SnapshotPage::recursiveSize(QString base, QString fileName) {
    QFileInfo thisFileInfo(base, fileName);
    QString thisPath = thisFileInfo.filePath();
    if (thisFileInfo.isSymLink()) {
        return 0;
    }
    if (thisFileInfo.isFile()) {
        QFile thisFile(thisPath);
        return thisFile.size();
    }
    if (!thisFileInfo.isDir()) {
        return 0;
    }
    // Directory: Count up all the included files
    QDir thisDir(thisPath);
    thisDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList includedNames(thisDir.entryList());
    qint64 totalSize = 0;
    for (QString nextName : includedNames) {
        totalSize += recursiveSize(thisPath, nextName);
    }
    return totalSize;
}

QString SnapshotPage::formattedSize(qint64 theSize) {
    double sizeValue = theSize / (1024.0 * 1024.0);
    QString sizeUnits = " MB";
    if (sizeValue >= 1024.0) {
        sizeValue /= 1024.0;
        sizeUnits = " GB";
    }
    int nFractionalDigits = (sizeValue < 9.94) ? 1 : 0;
    return QString::number(sizeValue, 'f', nFractionalDigits) + sizeUnits;
}
