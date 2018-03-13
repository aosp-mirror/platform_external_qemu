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

#include "android/skin/qt/extended-pages/snapshot-page.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/ui.h"
#include "android/snapshot/interface.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/proto/snapshot.pb.h"
#include "android/snapshot/Quickboot.h"
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


static const char CURRENT_SNAPSHOT_ICON_NAME[] = "current_snapshot";
static const char CURRENT_SNAPSHOT_SELECTED_ICON_NAME[] = "current_snapshot_selected";

// A class for items in the QTreeWidget.
class SnapshotPage::WidgetSnapshotItem : public QTreeWidgetItem {
    public:
        // ctor for a top-level item
        WidgetSnapshotItem(const QString&   fileName,
                           const QString&   logicalName,
                           const QDateTime& dateTime,
                                 bool       isValid) :
            QTreeWidgetItem((QTreeWidget*)0),
            mFileName(fileName),
            mDateTime(dateTime),
            mIsValid(isValid)
        {
            setText(COLUMN_NAME, logicalName);
        }

        // TODO: jameskaye@ Enable this code if we decide to provide a hierarchical
        //                  display. Remove this code if we decide not to.
        // // ctor for a subordinate item
        // // (used only for hierarchical display)
        // WidgetSnapshotItem(QTreeWidgetItem* parentItem,
        //                    QString          fileName,
        //                    QString          logicalName,
        //                    QDateTime        dateTime) :
        //     QTreeWidgetItem(parentItem),
        //     mFileName(fileName),
        //     mDateTime(dateTime)
        // {
        //     mParentName = parentItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
        //     setText(COLUMN_NAME, logicalName);
        // }

        // Customize the sort order: Sort by date
        // (Put invalid items after valid items. Invalid items
        // are sorted by name.)
        bool operator<(const QTreeWidgetItem &other)const {
            WidgetSnapshotItem& otherItem = (WidgetSnapshotItem&)other;

            if (mIsValid) {
                if (!otherItem.mIsValid) return true;
                return mDateTime < otherItem.mDateTime;
            } else {
                if (otherItem.mIsValid) return false;
                return mFileName < otherItem.mFileName;
            }
        }

        QString fileName() const {
            return mFileName;
        }
        QString parentName() const {
            return mParentName;
        }
        QDateTime dateTime() const {
            return mDateTime;
        }
        bool isValid() const {
            return mIsValid;
        }

    private:
        QString   mFileName;
        QString   mParentName;
        QDateTime mDateTime;
        bool      mIsValid;
};

SnapshotPage::SnapshotPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::SnapshotPage())
{
    mUi->setupUi(this);
    mUi->defaultSnapshotDisplay->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    mUi->snapshotDisplay->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);

    QObject::connect(this, SIGNAL(deleteCompleted()),
                     this, SLOT(slot_snapshotDeleteCompleted()));
    QObject::connect(this, SIGNAL(loadCompleted(int, QString)),
                     this, SLOT(slot_snapshotLoadCompleted(int, QString)));
    QObject::connect(this, SIGNAL(saveCompleted(int, QString)),
                     this, SLOT(slot_snapshotSaveCompleted(int, QString)));
}

void SnapshotPage::deleteSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }

    QString logicalName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
    QString fileName = theItem->fileName();

    QMessageBox msgBox(QMessageBox::Question,
                       tr("Delete snapshot"),
                       tr("Do you want to permanently delete<br>snapshot \"%1\"?")
                               .arg(logicalName),
                       QMessageBox::Cancel,
                       this);
    QPushButton *deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();

    if (selection == QMessageBox::Ok) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        android::base::ThreadLooper::runOnMainLooper([fileName, this] {
            androidSnapshot_delete(fileName.toStdString().c_str());
            emit(deleteCompleted());
        });
    }
}

void SnapshotPage::slot_snapshotDeleteCompleted() {
    populateSnapshotDisplay();
    QApplication::restoreOverrideCursor();
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
    nameEdit->selectAll();
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
        std::unique_ptr<emulator_snapshot::Snapshot> protobuf = loadProtobuf(fileName);

        if (protobuf != nullptr) {
            if (!newName.isEmpty()) {
                protobuf->set_logical_name(newName.toStdString());
            }
            protobuf->set_description(newDescription.toStdString());
            writeProtobuf(fileName, protobuf);

            populateSnapshotDisplay();

            // Select the just-edited item
            highlightItemWithFilename(fileName);
        }
        QApplication::restoreOverrideCursor();
    }
}


QString SnapshotPage::getDescription(const QString& fileName) {
    Snapshot theSnapshot(fileName.toStdString().c_str());
    const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
    if (protobuf == nullptr) return QString();

    return QString(protobuf->description().c_str());
}

const SnapshotPage::WidgetSnapshotItem* SnapshotPage::getSelectedSnapshot() {
    QList<QTreeWidgetItem *> selectedItems = mUi->snapshotDisplay->selectedItems();

    if (selectedItems.size() == 0) {
        // Nothing selected in the main list. Check the default snapshot's list.
        selectedItems = mUi->defaultSnapshotDisplay->selectedItems();
    }
    if (selectedItems.size() == 0) {
        return nullptr;
    }
    // Return the first selected element--multiple selections are not allowed
    return static_cast<WidgetSnapshotItem*>(selectedItems[0]);
}

void SnapshotPage::on_enlargeInfoButton_clicked() {
    mUseBigInfoWindow = true;
    on_snapshotDisplay_itemSelectionChanged();
}

void SnapshotPage::on_reduceInfoButton_clicked() {
    mUseBigInfoWindow = false;
    on_snapshotDisplay_itemSelectionChanged();
}

void SnapshotPage::on_editSnapshot_clicked() {
    editSnapshot(getSelectedSnapshot());
}

void SnapshotPage::on_deleteSnapshot_clicked() {
    deleteSnapshot(getSelectedSnapshot());
}

void SnapshotPage::on_loadSnapshot_clicked() {
    AndroidSnapshotStatus status;

    const WidgetSnapshotItem* theItem = getSelectedSnapshot();
    if (!theItem) return;

    QString fileName = theItem->fileName();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    android::base::ThreadLooper::runOnMainLooper([&status, fileName, this] {
        status = androidSnapshot_load(fileName.toStdString().c_str());
        emit(loadCompleted((int)status, fileName));
    });
}

void SnapshotPage::slot_snapshotLoadCompleted(int statusInt, const QString& snapshotFileName) {
    AndroidSnapshotStatus status = (AndroidSnapshotStatus)statusInt;

    if (status != SNAPSHOT_STATUS_OK) {
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Snapshot did not load"), tr("Load snapshot"));
        return;
    }
    // Refresh the list of available snapshots
    populateSnapshotDisplay();
    highlightItemWithFilename(snapshotFileName);
    QApplication::restoreOverrideCursor();
}

void SnapshotPage::on_defaultSnapshotDisplay_itemSelectionChanged() {
    QList<QTreeWidgetItem *> selectedItems = mUi->defaultSnapshotDisplay->selectedItems();
    if (selectedItems.size() > 0) {
        // We have the selection, de-select the other list
        mUi->snapshotDisplay->clearSelection();
    }
    updateAfterSelectionChanged();
}
void SnapshotPage::on_snapshotDisplay_itemSelectionChanged() {
    QList<QTreeWidgetItem *> selectedItems = mUi->snapshotDisplay->selectedItems();
    if (selectedItems.size() > 0) {
        // We have the selection, de-select the other list
        mUi->defaultSnapshotDisplay->clearSelection();
    }
    updateAfterSelectionChanged();
}

void SnapshotPage::updateAfterSelectionChanged() {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    adjustIcons(mUi->defaultSnapshotDisplay);
    adjustIcons(mUi->snapshotDisplay);

    QString selectionInfoString;
    QString simpleName;

    const WidgetSnapshotItem* theItem = getSelectedSnapshot();

    bool validItem = false;

    if (!theItem) {
        if (mUi->snapshotDisplay->topLevelItemCount() > 0) {
            selectionInfoString = tr("<big>No snapshot selected");
        } else {
            // Nothing can be selected
            selectionInfoString = "";
        }
    } else {
        validItem = theItem->isValid();
        QString logicalName = theItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
        simpleName = theItem->fileName();

        System::FileSize snapSize = android::snapshot::folderSize(simpleName.toStdString());

        if (validItem) {
            selectionInfoString =
                      tr("<big><b>%1</b></big><br>"
                         "%2, captured %3<br>"
                         "File: %4")
                              .arg(logicalName,
                                   formattedSize(snapSize),
                                   theItem->dateTime().isValid() ?
                                       theItem->dateTime().toString(Qt::SystemLocaleShortDate) :
                                       tr("(unknown)"),
                                   simpleName);

            QString description = getDescription(simpleName);
            if (!description.isEmpty()) {
                selectionInfoString += QString("<p><big>%1</big>")
                                       .arg(description.replace('<', "&lt;").replace('\n', "<br>"));
            }
            mUi->loadSnapshot->setEnabled(true);
            // Cannot edit the default snapshot
            mUi->editSnapshot->setEnabled(simpleName != Quickboot::kDefaultBootSnapshot.c_str());
        } else {
            // Invalid snapshot directory
            selectionInfoString =
                      tr("<big><b>%1</b></big><br>"
                         "Invalid snapshot: %2<br>")
                              .arg(logicalName, formattedSize(snapSize));
            mUi->loadSnapshot->setEnabled(false);
            mUi->editSnapshot->setEnabled(false);
        }
    }

    if (mUseBigInfoWindow) {
        // Use the big window for displaying the info
        mUi->bigSelectionInfo->setHtml(selectionInfoString);
        mUi->bigSelectionInfo->setVisible(true);
        mUi->reduceInfoButton->setVisible(true);

        mUi->smallSelectionInfo->setVisible(false);
        mUi->preview->setVisible(false);
        mUi->enlargeInfoButton->setVisible(false);
    } else {
        // Use the small window for displaying the info
        mUi->smallSelectionInfo->setHtml(selectionInfoString);
        mUi->smallSelectionInfo->setVisible(true);
        mUi->preview->setVisible(true);
        mUi->enlargeInfoButton->setVisible(true);

        mUi->bigSelectionInfo->setVisible(false);
        mUi->reduceInfoButton->setVisible(false);

        if (mUi->preview->isVisible()) {
            showPreviewImage(simpleName, validItem);
        }
    }

    QApplication::restoreOverrideCursor();
}

// Adjust the icons for selected vs non-selected rows
void SnapshotPage::adjustIcons(QTreeWidget* theDisplayList) {
    QTreeWidgetItemIterator iter(theDisplayList);
    for ( ; *iter; iter++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iter);
        if (item->icon(COLUMN_ICON).isNull()) continue;
        if (item->isSelected()) {
            item->setIcon(COLUMN_ICON, getIconForCurrentTheme(CURRENT_SNAPSHOT_SELECTED_ICON_NAME));
        } else {
            item->setIcon(COLUMN_ICON, getIconForCurrentTheme(CURRENT_SNAPSHOT_ICON_NAME));

        }
    }
}

// Displays the preview image of the selected snapshot
void SnapshotPage::showPreviewImage(const QString& snapshotName, bool isValid) {
    mPreviewScene.clear(); // Frees previewItem

    if (snapshotName.isEmpty() || !isValid) {
        mUi->preview->items().clear();
        return;
    }

    QPixmap pMap(PathUtils::join(getSnapshotBaseDir(),
                                 snapshotName.toStdString(),
                                 "screenshot.png").c_str());

    QGraphicsPixmapItem *previewItem = new QGraphicsPixmapItem(pMap);
    mPreviewScene.addItem(previewItem);

    mUi->preview->setScene(&mPreviewScene);
    mUi->preview->fitInView(previewItem, Qt::KeepAspectRatio);
}

void SnapshotPage::showEvent(QShowEvent* ee) {
    populateSnapshotDisplay();
}

void SnapshotPage::on_takeSnapshotButton_clicked() {
    AndroidSnapshotStatus status;

    QString snapshotName("snap_"
                         + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    android::base::ThreadLooper::runOnMainLooper([&status, snapshotName, this] {
        status = androidSnapshot_save(snapshotName.toStdString().c_str());
        emit(saveCompleted((int)status, snapshotName));
    });
}

void SnapshotPage::slot_snapshotSaveCompleted(int statusInt, const QString& snapshotName) {
    AndroidSnapshotStatus status = (AndroidSnapshotStatus)statusInt;

    if (status != SNAPSHOT_STATUS_OK) {
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Could not save snapshot"), tr("Take snapshot"));
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
    highlightItemWithFilename(snapshotName);
    QApplication::restoreOverrideCursor();
}

void SnapshotPage::populateSnapshotDisplay() {
    // (In the future, we may also want a hierarchical display.)
    populateSnapshotDisplay_flat();
}

// Populate the list of snapshot WITHOUT the hierarchy of parentage
void SnapshotPage::populateSnapshotDisplay_flat() {

    mUi->defaultSnapshotDisplay->setSortingEnabled(false); // Don't sort during modification
    mUi->snapshotDisplay->setSortingEnabled(false);
    mUi->defaultSnapshotDisplay->clear();
    mUi->snapshotDisplay->clear();

    std::string snapshotPath = getSnapshotBaseDir();

    // Find all the directories in the snapshot directory
    QDir snapshotDir(snapshotPath.c_str());
    snapshotDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList snapshotList(snapshotDir.entryList());

    // Look at all the directories and make a WidgetSnapshotItem for each one
    int nItems = 0;
    for (const QString& fileName : snapshotList) {

        Snapshot theSnapshot(fileName.toStdString().c_str());

        const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
        bool snapshotIsValid = (protobuf != nullptr);

        QString logicalName;
        QDateTime snapshotDate;
        if (!snapshotIsValid) {
            logicalName = fileName;
        } else {
            if (protobuf->has_logical_name()) {
                logicalName = protobuf->logical_name().c_str();
            } else {
                logicalName = fileName;
            }

            if (protobuf->has_creation_time()) {
                snapshotDate = QDateTime::fromMSecsSinceEpoch(1000LL * protobuf->creation_time());
            }
        }

        // Create a top-level item.
        WidgetSnapshotItem* thisItem =
                new WidgetSnapshotItem(fileName, logicalName, snapshotDate, snapshotIsValid);

        QTreeWidget* displayWidget = (fileName == Quickboot::kDefaultBootSnapshot.c_str()) ?
                                     mUi->defaultSnapshotDisplay : mUi->snapshotDisplay;

        displayWidget->addTopLevelItem(thisItem);

        if (!snapshotIsValid) {
            QFont italic = thisItem->font(COLUMN_NAME);
            italic.setPointSize(italic.pointSize() + 1);
            italic.setItalic(true);
            thisItem->setFont(COLUMN_NAME, italic);
        } else if (fileName == androidSnapshot_loadedSnapshotFile()) {
            // This snapshot was used to start the AVD
            QFont bigBold = thisItem->font(COLUMN_NAME);
            bigBold.setPointSize(bigBold.pointSize() + 2);
            bigBold.setBold(true);
            thisItem->setFont(COLUMN_NAME, bigBold);
            thisItem->setIcon(COLUMN_ICON, getIconForCurrentTheme("current_snapshot"));
            displayWidget->setCurrentItem(thisItem);
        } else {
            QFont biggish = thisItem->font(COLUMN_NAME);
            biggish.setPointSize(biggish.pointSize() + 1);
            thisItem->setFont(COLUMN_NAME, biggish);
        }

        nItems++;
    }

    if (nItems <= 0) {
        // Hide the lists and say that there are no snapshots
        mUi->defaultSnapshotDisplay->setVisible(false);
        mUi->snapshotDisplay->setVisible(false);
        mUi->noneAvailableLabel->setVisible(true);
        return;
    }
    mUi->defaultSnapshotDisplay->setVisible(true);
    mUi->snapshotDisplay->setVisible(true);
    mUi->noneAvailableLabel->setVisible(false);

    mUi->defaultSnapshotDisplay->header()->setStretchLastSection(false);
    mUi->defaultSnapshotDisplay->header()->setSectionResizeMode(COLUMN_ICON, QHeaderView::Fixed);
    mUi->defaultSnapshotDisplay->header()->resizeSection(COLUMN_ICON, 36);
    mUi->defaultSnapshotDisplay->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    mUi->defaultSnapshotDisplay->setSortingEnabled(true);

    mUi->snapshotDisplay->header()->setStretchLastSection(false);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_ICON, QHeaderView::Fixed);
    mUi->snapshotDisplay->header()->resizeSection(COLUMN_ICON, 36);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    mUi->snapshotDisplay->setSortingEnabled(true);
}

// TODO: jameskaye@ Enable this code if we decide to provide a hierarchical display.
//                  Remove this code if we decide not to.
// // Populate the list of snapshot with the hierarchy of parentage
// // (used only for the hierarchical display)
// void SnapshotPage::populateSnapshotDisplay_hierarchical() {
//
//     mUi->snapshotDisplay->setSortingEnabled(false); // Don't sort during modification
//     mUi->snapshotDisplay->clear();
//
//     // Temporary structure for organizing the display of the snapshots
//     class treeItem {
//         public:
//             QString             fileName;
//             QString             logicalName;
//             QDateTime           dateTime;
//             QString             parent;
//             int                 parentIdx;
//             WidgetSnapshotItem* item;
//     };
//
//     std::string snapshotPath = getSnapshotBaseDir();
//
//     // Find all the directories in the snapshot directory
//     QDir snapshotDir(snapshotPath.c_str());
//     snapshotDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
//     QStringList snapshotList(snapshotDir.entryList());
//
//     int nItems = snapshotList.size();
//     treeItem itemArray[nItems];
//
//     // Look at all the directories and make a treeItem for each one
//     int idx = 0;
//     for (QString fileName : snapshotList) {
//
//         Snapshot theSnapshot(fileName.toStdString().c_str());
//
//         const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
//         if (protobuf == nullptr) continue;
//         QString logicalName;
//         if (protobuf->has_logical_name()) {
//             logicalName = protobuf->logical_name().c_str();
//         } else {
//             logicalName = fileName;
//         }
//
//         QString parentName = "";
//         if (protobuf->has_parent()) {
//             parentName = protobuf->parent().c_str();
//         }
//
//         QDateTime snapshotDate;
//         if (protobuf->has_creation_time()) {
//             snapshotDate = QDateTime::fromMSecsSinceEpoch(1000LL * protobuf->creation_time());
//         }
//         itemArray[idx].fileName = fileName;
//         itemArray[idx].logicalName = logicalName;
//         itemArray[idx].dateTime = snapshotDate;
//         itemArray[idx].parent = parentName;
//         itemArray[idx].parentIdx = -1; // Haven't found the parent's item (yet)
//         itemArray[idx].item = nullptr; // No QTreeWidgetItem yet
//         idx++;
//     }
//     nItems = idx;
//
//     if (nItems <= 0) {
//         // Hide the tree display and say that there are no snapshots
//         mUi->snapshotDisplay->setVisible(false);
//         mUi->noneAvailableLabel->setVisible(true);
//         return;
//     }
//     mUi->snapshotDisplay->setVisible(true);
//     mUi->noneAvailableLabel->setVisible(false);
//
//     // Find the index corresponding to each parent
//     for (int idx = 0; idx < nItems; idx++) {
//         if (!itemArray[idx].parent.isEmpty()) {
//             // Search for this item's parent
//             const QString parentName = itemArray[idx].parent;
//             for (int parentIdx = 0; parentIdx < nItems; parentIdx++) {
//                 if (parentName == itemArray[parentIdx].fileName) {
//                     itemArray[idx].parentIdx = parentIdx;
//                     break;
//                 }
//             }
//         }
//     }
//
//     // Loop through the array, creating any items that we can
//     bool didSomething;
//     do {
//         didSomething = false;
//         for (int idx = 0; idx < nItems; idx++) {
//             if (itemArray[idx].item != nullptr) {
//                 // Already created an item for this array entry
//                 continue;
//             }
//
//             WidgetSnapshotItem* thisItem;
//             if (itemArray[idx].parentIdx < 0) {
//                 // This snapshot has no parent or its parent was not found.
//                 // Create a top-level item.
//                 thisItem = new WidgetSnapshotItem(itemArray[idx].fileName,
//                                                   itemArray[idx].logicalName,
//                                                   itemArray[idx].dateTime);
//                 mUi->snapshotDisplay->addTopLevelItem(thisItem);
//             } else if (itemArray[itemArray[idx].parentIdx].item != nullptr) {
//                 // The parent was found and its item was created.
//                 // Create a child item.
//                 thisItem = new WidgetSnapshotItem(itemArray[itemArray[idx].parentIdx].item,
//                                                   itemArray[idx].fileName,
//                                                   itemArray[idx].logicalName,
//                                                   itemArray[idx].dateTime);
//             } else {
//                 // This snapshot's parent does not exist as an 'item' yet,
//                 // so we cannot create the item for this snapshot yet.
//                 continue;
//             }
//             // We created an item for this snapshot
//             itemArray[idx].item = thisItem;
//
//             if (itemArray[idx].fileName ==
//                     androidSnapshot_loadedSnapshotFile())
//             {
//                 // This snapshot was used to start the AVD
//                 QFont bigBold = thisItem->font(COLUMN_NAME);
//                 bigBold.setPointSize(bigBold.pointSize() + 2);
//                 bigBold.setBold(true);
//                 thisItem->setFont(COLUMN_NAME, bigBold);
//                 mUi->snapshotDisplay->setCurrentItem(thisItem);
//             } else {
//                 QFont biggish = thisItem->font(COLUMN_NAME);
//                 biggish.setPointSize(biggish.pointSize() + 1);
//                 thisItem->setFont(COLUMN_NAME, biggish);
//             }
//             didSomething = true;
//         }
//     } while (didSomething);
//
//     mUi->snapshotDisplay->header()->setStretchLastSection(false);
//     mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
//     mUi->snapshotDisplay->setSortingEnabled(true);
// }

 void SnapshotPage::highlightItemWithFilename(const QString& fileName) {
    // Try the main list
    QTreeWidgetItemIterator iterMain(mUi->snapshotDisplay);
    for ( ; *iterMain; iterMain++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iterMain);
        if (fileName == item->fileName()) {
            mUi->snapshotDisplay->setCurrentItem(item);
            return;
        }
    }
    // Try the default snapshot list
    QTreeWidgetItemIterator iterDefault(mUi->defaultSnapshotDisplay);
    for ( ; *iterDefault; iterDefault++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iterDefault);
        if (fileName == item->fileName()) {
            mUi->defaultSnapshotDisplay->setCurrentItem(item);
            return;
        }
    }
}

void SnapshotPage::writeParentToProtobuf(const QString& fileName,
                                         const QString& parentName) {

    std::unique_ptr<emulator_snapshot::Snapshot> protobuf = loadProtobuf(fileName);

    if (protobuf != nullptr) {
        protobuf->set_parent(parentName.toStdString());
        writeProtobuf(fileName, protobuf);
    }
}

std::unique_ptr<emulator_snapshot::Snapshot> SnapshotPage::loadProtobuf(const QString& fileName) {
    Snapshot theSnapshot(fileName.toStdString().c_str());

    const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
    if (protobuf == nullptr) {
        printf("Cannot update protobuf: it is null!\n");
        return nullptr;
    }

    // Make a writable copy so the caller can update the contents
    std::unique_ptr<emulator_snapshot::Snapshot> newProtobuf = nullptr;
    newProtobuf.reset(new emulator_snapshot::Snapshot(*protobuf));
    return newProtobuf;
}

void SnapshotPage::writeProtobuf(const QString& fileName,
                                 const std::unique_ptr<emulator_snapshot::Snapshot>& protobuf) {
    std::string protoFileName = PathUtils::join(getSnapshotBaseDir().c_str(),
                                                fileName.toStdString().c_str(),
                                                "snapshot.pb");
    std::ofstream outStream(protoFileName.c_str(), std::ofstream::binary);

    protobuf->SerializeToOstream(&outStream);
}

QString SnapshotPage::formattedSize(qint64 theSize) {
    double sizeValue = theSize / (1024.0 * 1024.0);
    QString sizeUnits = " MB";
    if (sizeValue >= 1024.0) {
        sizeValue /= 1024.0;
        sizeUnits = " GB";
    }
    // Show at least two significant digits: if the value
    // will display as less than 10, include a fractional
    // digit. (Note that 9.95 would round and display as
    // '10.0', so make it '10' instead.)
    int nFractionalDigits = (sizeValue < 9.94) ? 1 : 0;
    return QString::number(sizeValue, 'f', nFractionalDigits) + sizeUnits;
}
