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

#include <qbytearray.h>                              // for operator==
#include <qdialog.h>                                 // for QDialog::Rejected
#include <qdialogbuttonbox.h>                        // for operator|, QDial...
#include <qdir.h>                                    // for operator|, QDir:...
#include <qeasingcurve.h>                            // for QEasingCurve::Ou...
#include <qheaderview.h>                             // for QHeaderView::Fixed
#include <qiodevice.h>                               // for operator|, QIODe...
#include <qmessagebox.h>                             // for operator|, QMess...
#include <qnamespace.h>                              // for WaitCursor, Asce...
#include <qsettings.h>                               // for QSettings::IniFo...
#include <qstring.h>                                 // for operator+, QStri...
#include <stdio.h>                                   // for fprintf, printf
#include <string.h>                                  // for strcmp
#include <QApplication>                              // for QApplication
#include <QBrush>                                    // for QBrush
#include <QByteArray>                                // for QByteArray
#include <QCheckBox>                                 // for QCheckBox
#include <QComboBox>                                 // for QComboBox
#include <QDateTime>                                 // for QDateTime
#include <QDialog>                                   // for QDialog
#include <QDialogButtonBox>                          // for QDialogButtonBox
#include <QDir>                                      // for QDir
#include <QEasingCurve>                              // for QEasingCurve
#include <QFile>                                     // for QFile
#include <QFont>                                     // for QFont
#include <QGraphicsPixmapItem>                       // for QGraphicsPixmapItem
#include <QGraphicsTextItem>                         // for QGraphicsTextItem
#include <QGraphicsView>                             // for QGraphicsView
#include <QHash>                                     // for QHash
#include <QHeaderView>                               // for QHeaderView
#include <QIcon>                                     // for QIcon
#include <QLabel>                                    // for QLabel
#include <QLineEdit>                                 // for QLineEdit
#include <QList>                                     // for QList
#include <QMessageBox>                               // for QMessageBox
#include <QObject>                                   // for QObject
#include <QPixmap>                                   // for QPixmap
#include <QPlainTextEdit>                            // for QPlainTextEdit
#include <QPropertyAnimation>                        // for QPropertyAnimation
#include <QPushButton>                               // for QPushButton
#include <QRectF>                                    // for QRectF
#include <QSettings>                                 // for QSettings
#include <QSizeF>                                    // for QSizeF
#include <QTabWidget>                                // for QTabWidget
#include <QTextDocument>                             // for QTextDocument
#include <QTextEdit>                                 // for QTextEdit
#include <QTreeWidget>                               // for QTreeWidget
#include <QTreeWidgetItem>                           // for QTreeWidgetItem
#include <QTreeWidgetItemIterator>                   // for QTreeWidgetItemI...
#include <QVBoxLayout>                               // for QVBoxLayout
#include <QVariant>                                  // for QVariant
#include <fstream>                                   // for ofstream
#include <functional>                                // for __base
#include <string>                                    // for string

#include "android/avd/info.h"                        // for AVDINFO_NO_SNAPS...
#include "android/avd/util.h"                        // for path_getAvdConte...
#include "android/base/Log.h"                        // for base
#include "android/base/ProcessControl.h"             // for isRestartDisabled
#include "android/base/async/ThreadLooper.h"         // for ThreadLooper
#include "android/base/files/PathUtils.h"            // for PathUtils
#include "android/base/system/System.h"              // for System, System::...
#include "android/cmdline-option.h"                  // for AndroidOptions
#include "android/emulation/control/window_agent.h"  // for EmulatorWindow
#include "android/emulator-window.h"                 // for emulator_window_get
#include "android/featurecontrol/FeatureControl.h"   // for isEnabled
#include "android/featurecontrol/Features.h"         // for QuickbootFileBacked
#include "android/globals.h"                         // for android_avdParams
#include "android/metrics/MetricsReporter.h"         // for MetricsReporter
#include "android/metrics/MetricsWriter.h"           // for android_studio
#include "studio_stats.pb.h"   // for EmulatorSnapshot...
#include "android/settings-agent.h"                  // for SettingsTheme
#include "android/skin/qt/error-dialog.h"            // for showErrorDialog
#include "android/skin/qt/extended-pages/common.h"   // for setButtonEnabled
#include "android/skin/qt/raised-material-button.h"  // for RaisedMaterialBu...
#include "android/skin/qt/stylesheet.h"              // for stylesheetForTheme
#include "android/snapshot/common.h"
#include "android/snapshot/PathUtils.h"              // for getSnapshotBaseDir
#include "android/snapshot/Quickboot.h"              // for Quickboot, Quick...
#include "android/snapshot/Snapshot.h"               // for Snapshot
#include "android/snapshot/interface.h"              // for androidSnapshot_...
#include "snapshot.pb.h"      // for Snapshot

class QCheckBox;
class QCloseEvent;
class QDateTime;
class QGraphicsItem;
class QGraphicsPixmapItem;
class QGraphicsTextItem;
class QLineEdit;
class QPlainTextEdit;
class QPropertyAnimation;
class QPushButton;
class QShowEvent;
class QTextDocument;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QWidget;

using android::metrics::MetricsReporter;
using android::snapshot::Quickboot;
using Ui::Settings::SaveSnapshotOnExit;
using Ui::Settings::SaveSnapshotOnExitUiOrder;
using Ui::Settings::DeleteInvalidSnapshots;
using Ui::Settings::DeleteInvalidSnapshotsUiOrder;

using namespace android::base;
using namespace android::snapshot;

namespace pb = android_studio;
namespace fc = android::featurecontrol;
using fc::Feature;

static const char CURRENT_SNAPSHOT_ICON_NAME[] = "current_snapshot";
static const char CURRENT_SNAPSHOT_SELECTED_ICON_NAME[] = "current_snapshot_selected";
static const char INVALID_SNAPSHOT_ICON_NAME[] = "invalid_snapshot";
static const char INVALID_SNAPSHOT_SELECTED_ICON_NAME[] = "invalid_snapshot_selected";

static constexpr char kDefaultBootItemName[] = "Quickboot";
static constexpr char kDefaultBootFileBackedTitleName[] = "Quickboot (auto-saved)";

static SaveSnapshotOnExit getSaveOnExitChoice();
static void setSaveOnExitChoice(SaveSnapshotOnExit choice);

// Globally accessable
bool userSettingIsDontSaveSnapshot() {
    return getSaveOnExitChoice() == SaveSnapshotOnExit::Never;
}
// A class for items in the QTreeWidget.
class SnapshotPage::WidgetSnapshotItem : public QTreeWidgetItem {
    public:

        // ctor for a top-level item
        WidgetSnapshotItem(const QString&   fileName,
                           const QString&   logicalName,
                           const QDateTime& dateTime,
                                 bool       isValid,
                                 bool       isTheParent) :
            QTreeWidgetItem((QTreeWidget*)0),
            mFileName(fileName),
            mDateTime(dateTime),
            mIsValid(isValid),
            mIsTheParent(isTheParent)
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
        //     mParentName = parentItem->logicalName();
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
        QString logicalName() const {
            return data(COLUMN_NAME, Qt::DisplayRole).toString();
        }

        bool isValid() const {
            return mIsValid;
        }
        bool isTheParent() const {
            return mIsTheParent;
        }

    private:
        QString   mFileName;
        QString   mParentName;
        QDateTime mDateTime;
        bool      mIsValid = true;
        bool      mIsTheParent = false;
};

static SnapshotPage* sInstance = nullptr;

SnapshotPage::SnapshotPage(QWidget* parent, bool standAlone) :
    QWidget(parent),
    mIsStandAlone(standAlone),
    mUi(new Ui::SnapshotPage())
{
    mUi->setupUi(this);

    mUi->inProgressLabel->hide();

    if (android_cmdLineOptions->read_only) {
        // Stand-alone.
        // Overlay a mask and text saying that snapshots are disabled.
        mUi->noSnapshot_mask->raise();
        mUi->noSnapshot_message->raise();
        // Hide widgets that look bad when showing through the overlay
        mUi->noneAvailableLabel->hide();
        mUi->reduceInfoButton->hide();
        return;
    }

    // Hide the overlay that obscures the page. Hide the text
    // that says snapshots are disabled.
    mUi->noSnapshot_mask->hide();
    mUi->noSnapshot_message->hide();

    mUi->defaultSnapshotDisplay->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    mUi->snapshotDisplay->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);

    mUi->enlargeInfoButton->setVisible(true);
    mUi->reduceInfoButton->setVisible(false);

    mInfoPanelSmallGeo = mUi->selectionInfo->geometry();
    QRect previewGeo = mUi->preview->geometry();
    // The large Info panel goes from the top of the Preview panel
    // to the bottom of the small Info panel
    mInfoPanelLargeGeo = QRect(mInfoPanelSmallGeo.x(),
                               previewGeo.y(),
                               mInfoPanelSmallGeo.width(),
                               mInfoPanelSmallGeo.y() + mInfoPanelSmallGeo.height() - previewGeo.y());


    SaveSnapshotOnExit saveOnExitChoice = getSaveOnExitChoice();
    changeUiFromSaveOnExitSetting(saveOnExitChoice);

    for (int i = 0; i < mUi->saveQuickBootOnExit->count(); i++) {
        mUi->saveQuickBootOnExit->setItemData(i, QVariant(i));
    }

    // In file-backed Quickboot, the 'save now' button is always disabled.
    if (fc::isEnabled(fc::QuickbootFileBacked)) {
        mUi->saveOnExitTitle->setText(QString(tr("Auto-save current state to Quickboot")));

        // Migrate "Ask" users to "Always"
        if (saveOnExitChoice == SaveSnapshotOnExit::Ask) {
            setSaveOnExitChoice(SaveSnapshotOnExit::Always);
            changeUiFromSaveOnExitSetting(getSaveOnExitChoice());
        }

        for (int i = 0; i < mUi->saveQuickBootOnExit->count();) {
            SaveSnapshotOnExitUiOrder uiOrder =
                static_cast<SaveSnapshotOnExitUiOrder>(mUi->saveQuickBootOnExit->itemData(i).toInt());
            switch (uiOrder) {
                case SaveSnapshotOnExitUiOrder::Ask:
                    mUi->saveQuickBootOnExit->removeItem(i);
                    break;
                default:
                    ++i;
                    break;
            }
        }

        // TODO: loadQuickBootNowButton requires unmap and remap of ram file
        // saveQuickBootNowButton requires a lot more work
        mUi->saveQuickBootNowButton->hide();
        mUi->loadQuickBootNowButton->hide();

        mUi->autoSaveNoteLabel->show();
        mUi->autoSaveNoteLabelIcon->show();

        // Disallow changing the setting if restart was disabled.
        if (android::base::isRestartDisabled()) {
            mUi->saveQuickBootOnExit->setEnabled(false);
        }

        if (saveOnExitChoice == SaveSnapshotOnExit::Always) {
            mUi->noneAvailableLabel->setText(QString(tr("Auto-saving Quickboot state...")));
        }
    } else {
        // Save QuickBoot snapshot on exit
        QString avdNameWithUnderscores(android_hw->avd_name);

        mUi->saveOnExitTitle->setText(QString(tr("Save quick-boot state on exit for AVD: "))
                + avdNameWithUnderscores.replace('_', ' '));

        // Enable SAVE NOW if we won't overwrite the state on exit
        mUi->saveQuickBootNowButton->setEnabled(saveOnExitChoice != SaveSnapshotOnExit::Always);

        // Auto-save not enabled
        mUi->autoSaveNoteLabel->hide();
        mUi->autoSaveNoteLabelIcon->hide();
    }

    QSettings settings;
    DeleteInvalidSnapshots deleteInvalids = static_cast<DeleteInvalidSnapshots>
                                            (settings.value(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                                                            static_cast<int>(DeleteInvalidSnapshots::Ask))
                                                      .toInt());
    DeleteInvalidSnapshotsUiOrder deleteInvalidsUiIdx;
    switch (deleteInvalids) {
        default:
        case DeleteInvalidSnapshots::Ask:  deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::Ask;  break;
        case DeleteInvalidSnapshots::Auto: deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::Auto; break;
        case DeleteInvalidSnapshots::No:   deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::No;   break;
    }
    mUi->deleteInvalidSnapshots->setCurrentIndex(static_cast<int>(deleteInvalidsUiIdx));

    if (mIsStandAlone) {
        QRect widgetGeometry = frameGeometry();
        setFixedHeight(widgetGeometry.height() + 36); // Allow for the title bar
        setFixedWidth(widgetGeometry.width());
        setStyleSheet(Ui::stylesheetForTheme(getSelectedTheme()));
        setWindowTitle(tr("Manage snapshots"));
        mUi->loadSnapshot->setVisible(false);
        mUi->deleteSnapshot->setVisible(false); // Cannot delete without QEMU active
        mUi->takeSnapshotButton->setText(tr("CHOOSE SNAPSHOT"));
        mUi->tabWidget->removeTab(1); // Do not show the Settings tab

        getOutputFileName();
    }

    QObject::connect(this, SIGNAL(deleteCompleted()),
                     this, SLOT(slot_snapshotDeleteCompleted()));
    QObject::connect(this, SIGNAL(loadCompleted(int, QString)),
                     this, SLOT(slot_snapshotLoadCompleted(int, QString)));
    QObject::connect(this, SIGNAL(saveCompleted(int, QString)),
                     this, SLOT(slot_snapshotSaveCompleted(int, QString)));
    // Queue the "ask" signal so the pop-up doesn't appear until the
    // base window has been populated and raised.
    QObject::connect(this, SIGNAL(askAboutInvalidSnapshots(QStringList)),
                     this, SLOT(slot_askAboutInvalidSnapshots(QStringList)),
                     Qt::QueuedConnection);

    qRegisterMetaType<Ui::Settings::SaveSnapshotOnExit>();

    // Likewise, queue the confirm auto-save setting signal so the base window
    // is populated and raised before the pop-up shows.
    QObject::connect(
        this,
        SIGNAL(
            confirmAutosaveChoiceAndRestart(
                Ui::Settings::SaveSnapshotOnExit,
                Ui::Settings::SaveSnapshotOnExit)),
        this,
        SLOT(
            slot_confirmAutosaveChoiceAndRestart(
                Ui::Settings::SaveSnapshotOnExit,
                Ui::Settings::SaveSnapshotOnExit)),
        Qt::QueuedConnection);

    sInstance = this;
}

// static
SnapshotPage* SnapshotPage::get() {
    return sInstance;
}

void SnapshotPage::deleteSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }

    QString logicalName = theItem->logicalName();
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
        disableActions();
        setOperationInProgress(true);
        android::base::ThreadLooper::runOnMainLooper([fileName, this] {
            androidSnapshot_delete(fileName.toStdString().c_str());
            emit(deleteCompleted());
        });
    }
}

void SnapshotPage::slot_snapshotDeleteCompleted() {
    populateSnapshotDisplay();
    enableActions();
    setOperationInProgress(false);
    QApplication::restoreOverrideCursor();
}

void SnapshotPage::editSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    disableActions();
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = theItem->logicalName();
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);

    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
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

    enableActions();
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
        disableActions();

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
        enableActions();
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

void SnapshotPage::changeUiFromSaveOnExitSetting(SaveSnapshotOnExit choice) {
    switch (choice) {
        case SaveSnapshotOnExit::Always:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Ask:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Ask));
            // If we can't ask, we'll treat ASK the same as ALWAYS.
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Never:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Never));
            android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        default:
            fprintf(stderr,
                    "%s: WARNING: unknown 'Save snapshot on exit' preference value 0x%x. "
                    "Setting to Always.\n",
                    __func__, (unsigned int)choice);
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
    }
}

void SnapshotPage::on_enlargeInfoButton_clicked() {
    // Make the info window grow
    mUi->preview->lower(); // Let the preview window get covered
    QPropertyAnimation *infoAnimation = new QPropertyAnimation(mUi->selectionInfo, "geometry");

    infoAnimation->setDuration(500);  // mSec
    infoAnimation->setStartValue(mInfoPanelSmallGeo);
    infoAnimation->setEndValue  (mInfoPanelLargeGeo);
    infoAnimation->setEasingCurve(QEasingCurve::OutCubic);
    infoAnimation->start();

    mInfoWindowIsBig = true;
    on_snapshotDisplay_itemSelectionChanged();
}

void SnapshotPage::on_reduceInfoButton_clicked() {
    // Make the info window shrink
    QPropertyAnimation *infoAnimation = new QPropertyAnimation(mUi->selectionInfo, "geometry");

    infoAnimation->setDuration(500);  // mSec
    infoAnimation->setStartValue(mInfoPanelLargeGeo);
    infoAnimation->setEndValue  (mInfoPanelSmallGeo);
    infoAnimation->setEasingCurve(QEasingCurve::OutCubic);
    infoAnimation->start();

    mInfoWindowIsBig = false;
    on_snapshotDisplay_itemSelectionChanged();
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
    disableActions();
    setOperationInProgress(true);
    android::base::ThreadLooper::runOnMainLooper([fileName, this] {
        AndroidSnapshotStatus status = androidSnapshot_load(fileName.toStdString().c_str());
        emit(loadCompleted((int)status, fileName));
    });
}

void SnapshotPage::on_saveQuickBootNowButton_clicked() {
    setEnabled(false);
    setOperationInProgress(true);
    // Invoke the snapshot save function.
    // But don't run it on the UI thread.
    android::base::ThreadLooper::runOnMainLooper(
            [this]() { AndroidSnapshotStatus status = androidSnapshot_save(Quickboot::kDefaultBootSnapshot);
                       emit(saveCompleted((int)status, Quickboot::kDefaultBootSnapshot)); });
}

void SnapshotPage::on_loadQuickBootNowButton_clicked() {
    setEnabled(false);
    setOperationInProgress(true);
    // Invoke the snapshot load function.
    // But don't run it on the UI thread.
    android::base::ThreadLooper::runOnMainLooper(
            [this]() { AndroidSnapshotStatus status = androidSnapshot_load(Quickboot::kDefaultBootSnapshot);
                       emit(loadCompleted((int)status, Quickboot::kDefaultBootSnapshot)); });
}

void SnapshotPage::slot_snapshotLoadCompleted(int statusInt, const QString& snapshotFileName) {
    AndroidSnapshotStatus status = (AndroidSnapshotStatus)statusInt;

    setEnabled(true);

    if (status != SNAPSHOT_STATUS_OK) {
        enableActions();
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Snapshot did not load"), tr("Load snapshot"));
        return;
    }
    // Refresh the list of available snapshots
    populateSnapshotDisplay();
    highlightItemWithFilename(snapshotFileName);
    enableActions();
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

static SaveSnapshotOnExit getSaveOnExitChoice() {
    // This setting belongs to the AVD, not to the entire Emulator.
    SaveSnapshotOnExit userChoice(SaveSnapshotOnExit::Always);
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        userChoice = static_cast<SaveSnapshotOnExit>(
            avdSpecificSettings.value(
                Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                static_cast<int>(SaveSnapshotOnExit::Always)).toInt());
    }
    return userChoice;
}

static void setSaveOnExitChoice(SaveSnapshotOnExit choice) {
    // Save for only this AVD
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        avdSpecificSettings.setValue(Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                                     static_cast<int>(choice));
    }
}

void SnapshotPage::on_saveQuickBootOnExit_currentIndexChanged(int uiIndex) {
    SaveSnapshotOnExit preferenceValue;
    switch(static_cast<SaveSnapshotOnExitUiOrder>(uiIndex)) {
        case SaveSnapshotOnExitUiOrder::Never:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_no(1 + counts->quickboot_selection_no());
            });
            preferenceValue = SaveSnapshotOnExit::Never;
            android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExitUiOrder::Ask:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_ask(1 + counts->quickboot_selection_ask());
            });
            preferenceValue = SaveSnapshotOnExit::Ask;
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        default:
        case SaveSnapshotOnExitUiOrder::Always:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_yes(1 + counts->quickboot_selection_yes());
            });
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            preferenceValue = SaveSnapshotOnExit::Always;
            break;
    }

    if (fc::isEnabled(fc::QuickbootFileBacked)) {
        mUi->saveQuickBootNowButton->setEnabled(false);
        // TODO: loadQuickBootNowButton requires unmap
        // and remap of ram file
        mUi->loadQuickBootNowButton->setEnabled(false);

        SaveSnapshotOnExit previousChoice = getSaveOnExitChoice();
        if (previousChoice != preferenceValue) {
            emit(confirmAutosaveChoiceAndRestart(previousChoice, preferenceValue));
        }
    } else {
        // Enable SAVE STATE NOW if we won't overwrite the state on exit
        mUi->saveQuickBootNowButton->setEnabled(preferenceValue != SaveSnapshotOnExit::Always);
        setSaveOnExitChoice(preferenceValue);
    }
}

void SnapshotPage::on_deleteInvalidSnapshots_currentIndexChanged(int uiIndex) {
    DeleteInvalidSnapshots preferenceValue;
    switch(static_cast<DeleteInvalidSnapshotsUiOrder>(uiIndex)) {
        case DeleteInvalidSnapshotsUiOrder::No:
            preferenceValue = DeleteInvalidSnapshots::No;
            break;
        default:
        case DeleteInvalidSnapshotsUiOrder::Ask:
            preferenceValue = DeleteInvalidSnapshots::Ask;
            break;
        case DeleteInvalidSnapshotsUiOrder::Auto:
            preferenceValue = DeleteInvalidSnapshots::Auto;
            break;
    }
    QSettings settings;
    settings.setValue(Ui::Settings::DELETE_INVALID_SNAPSHOTS, static_cast<int>(preferenceValue));
    // Re-enable our check, so we act on this new setting
    // when the user goes back to look at the list
    mDidInitialInvalidCheck = false;
}

void SnapshotPage::on_tabWidget_currentChanged(int index) {
    // The previous tab may have changed how this tab
    // should look. Refresh the snapshot list to be safe.
    populateSnapshotDisplay();
}

void SnapshotPage::updateAfterSelectionChanged() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    disableActions();

    adjustIcons(mUi->defaultSnapshotDisplay);
    adjustIcons(mUi->snapshotDisplay);

    QString selectionInfoString;
    QString descriptionString;
    QString simpleName;

    const WidgetSnapshotItem* theItem = getSelectedSnapshot();

    SelectionStatus selectedItemStatus;

    if (!theItem) {
        selectedItemStatus = SelectionStatus::NoSelection;
        if (mUi->snapshotDisplay->topLevelItemCount() > 0) {
            selectionInfoString = tr("<big>No snapshot selected");
        } else {
            // Nothing can be selected
            selectionInfoString = "";
        }
        mAllowLoad = false;
        mAllowEdit = false;
        mAllowDelete = false;
        mAllowTake = !mIsStandAlone;
    } else {
        QString logicalName = theItem->logicalName();
        simpleName = theItem->fileName();

        System::FileSize snapSize = android::snapshot::folderSize(simpleName.toStdString());

        QString description = getDescription(simpleName);
        if (!description.isEmpty()) {
            descriptionString = QString("<p><big>%1</big>")
                                    .arg(description.replace('<', "&lt;").replace('\n', "<br>"));
        }

        if (theItem->isValid()) {
            selectedItemStatus = SelectionStatus::Valid;
            selectionInfoString =
                      tr("<big><b>%1</b></big><br>"
                         "%2, captured %3<br>"
                         "File: %4"
                         "%5")
                              .arg(logicalName,
                                   formattedSize(snapSize),
                                   theItem->dateTime().isValid() ?
                                       theItem->dateTime().toString(Qt::SystemLocaleShortDate) :
                                       tr("(unknown)"),
                                   simpleName,
                                   descriptionString);

            bool isFileBacked = theItem->logicalName() == kDefaultBootFileBackedTitleName;

            mAllowLoad = !isFileBacked;
            mAllowDelete = !isFileBacked;
            // Cannot edit the default snapshot
            mAllowEdit = !isFileBacked && (simpleName != Quickboot::kDefaultBootSnapshot);
            mAllowTake = mAllowEdit || !mIsStandAlone;
        } else {
            // Invalid snapshot directory
            selectedItemStatus = SelectionStatus::Invalid;
            selectionInfoString =
                      tr("<big><b>%1</b></big><br>"
                         "%2<br>"
                         "File: %3"
                         "<div style=\"color:red\">Invalid snapshot</div> %4")
                              .arg(logicalName, formattedSize(snapSize), simpleName, descriptionString);
            mAllowLoad = false;
            mAllowEdit = false;
            mAllowDelete = true;
            mAllowTake = !mIsStandAlone;
        }
    }

    if (mInfoWindowIsBig) {
        mUi->selectionInfo->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        // Small info window: Turn off the scroll bars and trim the text to fit.
        mUi->selectionInfo->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        selectionInfoString = elideSnapshotInfo(selectionInfoString);
    }
    mUi->selectionInfo->setHtml(selectionInfoString);
    mUi->reduceInfoButton->setVisible(mInfoWindowIsBig);
    mUi->enlargeInfoButton->setVisible(!mInfoWindowIsBig);

    if (!mInfoWindowIsBig && mUi->preview->isVisible()) {
        showPreviewImage(simpleName, selectedItemStatus);
    }
    enableActions();
    QApplication::restoreOverrideCursor();
}

// This method is used when the small region is used for
// the snapshot info. In this case, we do not allow the
// region to scroll. If the info text does not fit, we
// trim off enough that we can append "  ..." and still
// have the result fit.
QString SnapshotPage::elideSnapshotInfo(QString fullString) {
    constexpr char dotdotdot[] = "<b>&nbsp;&nbsp;...</b>";
    QTextDocument* textDocument = mUi->selectionInfo->document();

    if (mSmallInfoRegionSize == 0.0) {
        mSmallInfoRegionSize = (qreal)mUi->selectionInfo->height();
    }
    mUi->selectionInfo->setHtml(fullString);
    if (textDocument->size().rheight() <= mSmallInfoRegionSize) {
        // The full string fits. No need to elide.
        return fullString;
    }
    // The full string is too big.
    // Bisect to find how much fits in the reduced size.
    int leftLength = 0;
    int rightLength = fullString.size();
    while(rightLength - leftLength > 1) {
        int midLength = (leftLength + rightLength) / 2;
        QString midLengthString = fullString.left(midLength) + dotdotdot;
        mUi->selectionInfo->setHtml(midLengthString);
        if (textDocument->size().rheight() > mSmallInfoRegionSize) {
            rightLength = midLength;
        } else {
            leftLength = midLength;
        }
    }
    // We want to truncate fullString at leftLength. But if that leaves
    // a partial HTML tag, we need to remove the whole tag.
    int lastLessPosition = fullString.lastIndexOf('<', leftLength);
    int lastGreaterPosition = fullString.lastIndexOf('>', leftLength);
    if (lastLessPosition >= 0 && lastGreaterPosition >= 0 &&
        lastLessPosition > lastGreaterPosition)
    {
        leftLength = lastLessPosition;
    }
    // Trim it and add "  ..."
    return (fullString.left(leftLength) + dotdotdot);
}

// Adjust the icons for selected vs non-selected rows
void SnapshotPage::adjustIcons(QTreeWidget* theDisplayList) {
    QTreeWidgetItemIterator iter(theDisplayList);
    for ( ; *iter; iter++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iter);
        QIcon decoration;
        if (!item->isValid()) {
            decoration = item->isSelected() ? getIconForCurrentTheme(INVALID_SNAPSHOT_SELECTED_ICON_NAME)
                                            : getIconForCurrentTheme(INVALID_SNAPSHOT_ICON_NAME);
        } else if (item->isTheParent()) {
            decoration = item->isSelected() ? getIconForCurrentTheme(CURRENT_SNAPSHOT_SELECTED_ICON_NAME)
                                            : getIconForCurrentTheme(CURRENT_SNAPSHOT_ICON_NAME);
        }
        item->setIcon(COLUMN_ICON, decoration);
    }
}

// Displays the preview image of the selected snapshot
void SnapshotPage::showPreviewImage(const QString& snapshotName, SelectionStatus itemStatus) {
    static const QString invalidSnapText =
            tr("<div style=\"text-align:center\">"
               "<p style=\"color:red\"><big>Invalid snapshot</big></p>"
               "<p style=\"color:%1\"><big>Delete this snapshot to free disk space</big></p>"
               "</div>");
    static const QString noImageText =
            tr("<div style=\"text-align:center\">"
               "<p style=\"color:%1\"><big>This snapshot does not have a preview image</big></p>"
               "</div>");

    mPreviewScene.clear(); // Frees previewItem

    QGraphicsPixmapItem *previewItem;
    bool haveImage = false;

    if (!snapshotName.isEmpty()) {
        QPixmap pMap(PathUtils::join(getSnapshotBaseDir(),
                                     snapshotName.toStdString(),
                                     "screenshot.png").c_str());

        previewItem = new QGraphicsPixmapItem(pMap);
        // Is there really a preview image?
        QRectF imageRect = previewItem->boundingRect();
        haveImage = (imageRect.width() > 1.0 && imageRect.height() > 1.0);
    }

    if (haveImage) {
        // Display the image
        mPreviewScene.addItem(previewItem);
        mPreviewScene.setSceneRect(0, 0, 0, 0); // Reset to default

        mUi->preview->setScene(&mPreviewScene);
        mUi->preview->fitInView(previewItem, Qt::KeepAspectRatio);
    } else {
        // Display some text saying why there is no image
        mUi->preview->items().clear();

        QGraphicsTextItem *textItem = new QGraphicsTextItem;
        const QString& textColor = Ui::stylesheetValues(getSelectedTheme())[Ui::THEME_TEXT_COLOR];
        switch (itemStatus) {
            case SelectionStatus::NoSelection:
                textItem->setHtml("");
                break;
            case SelectionStatus::Valid:
                textItem->setHtml(noImageText.arg(textColor));
                break;
            case SelectionStatus::Invalid:
                textItem->setHtml(invalidSnapText.arg(textColor));
                break;
        }
        textItem->setTextWidth(mUi->preview->width());
        mPreviewScene.addItem(textItem);
        int offset = (mUi->preview->height() * 3) / 8; // Put the text near the center, vertically
        mPreviewScene.setSceneRect(0, -offset, mUi->preview->width(), mUi->preview->height());
        mUi->preview->setScene(&mPreviewScene);
        mUi->preview->fitInView(0, 0, mUi->preview->width(), mUi->preview->height(), Qt::KeepAspectRatio);
    }
}

void SnapshotPage::showEvent(QShowEvent* ee) {
    populateSnapshotDisplay();
}

// In normal mode, this button is used to capture a snapshot.
// In stand-alone mode, it is used to choose the selected
// snapshot.
void SnapshotPage::on_takeSnapshotButton_clicked() {
    setOperationInProgress(true);


    if (mIsStandAlone) {
        mMadeSelection = true;
        close();
    } else {

        setEnabled(false);
        QString snapshotName("snap_"
                             + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

        QApplication::setOverrideCursor(Qt::WaitCursor);
        disableActions();
        android::base::ThreadLooper::runOnMainLooper([snapshotName, this] {
            AndroidSnapshotStatus status =
            androidSnapshot_save(snapshotName.toStdString().c_str());
            emit(saveCompleted((int)status, snapshotName));
        });
    }
}

// This function is invoked only in stand-alone mode. It runs
// when the user clicks either takeSnapshot or X
void SnapshotPage::closeEvent(QCloseEvent* closeEvent) {
    if (mOutputFileName.isEmpty()) {
        return;
    }
    // Write the file name of the selected snapshot to our
    // output file
    const WidgetSnapshotItem* selectedItem = nullptr;
    if (mMadeSelection) {
        // The user clicked 'takeSnapshot'
        selectedItem = getSelectedSnapshot();
    }
    QString selectedFile = (selectedItem ? selectedItem->fileName() : "");
    QFile outputFile(mOutputFileName);
    if (outputFile.open(QFile::WriteOnly | QFile::Text)) {
        QString selectionString = "selectedSnapshotFile=" + selectedFile + "\n";
        outputFile.write(selectionString.toUtf8());
        outputFile.close();
    }
}

void SnapshotPage::slot_snapshotSaveCompleted(int statusInt, const QString& snapshotName) {
    AndroidSnapshotStatus status = (AndroidSnapshotStatus)statusInt;

    setEnabled(true);

    if (status != SNAPSHOT_STATUS_OK) {
        enableActions();
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Could not save snapshot"), tr("Take snapshot"));
        return;
    }

    // If there was a loaded snapshot, set that snapshot as the parent
    // of this one.
    const char* parentName = androidSnapshot_loadedSnapshotFile();
    if (parentName && parentName[0] != '\0' &&
        strcmp(parentName, Quickboot::kDefaultBootSnapshot)) {
        writeParentToProtobuf(snapshotName, QString(parentName));
    }

    // Refresh the list of available snapshots
    populateSnapshotDisplay();
    highlightItemWithFilename(snapshotName);
    enableActions();
    QApplication::restoreOverrideCursor();
}

void sClearTreeWidget(QTreeWidget* tree) {
#ifdef __APPLE__
    // bug: 112196179
    // QAccessibility is buggy on macOS.  We need to remove tree
    // items one by one, or QAccessbility will go out of sync and cause a
    // segfault.
    // More background info:
    // https://blog.inventic.eu/2015/05/crash-in-qtreewidget-qtreeview-index-mapping-on-mac-osx-10-10-part-iii/
    while (tree->topLevelItemCount()) {
        delete tree->takeTopLevelItem(0);
    }
#endif
    tree->clear();
}

void SnapshotPage::clearSnapshotDisplay() {
    sClearTreeWidget(mUi->defaultSnapshotDisplay);
    sClearTreeWidget(mUi->snapshotDisplay);
}

void SnapshotPage::setShowSnapshotDisplay(bool show) {
    mUi->defaultSnapshotDisplay->setVisible(show);
    mUi->snapshotDisplay->setVisible(show);
}

void SnapshotPage::setOperationInProgress(bool inProgress) {
    if (inProgress) {
        setShowSnapshotDisplay(false);
        mUi->inProgressLabel->show();
    } else {
        setShowSnapshotDisplay(true);
        mUi->inProgressLabel->hide();
    }
}

void SnapshotPage::populateSnapshotDisplay() {
    if (android_cmdLineOptions->read_only) {
        // Leave the list empty because snapshots
        // are disabled in read-only mode
        return;
    }
    // (In the future, we may also want a hierarchical display.)
    populateSnapshotDisplay_flat();
}

// Populate the list of snapshot WITHOUT the hierarchy of parentage
void SnapshotPage::populateSnapshotDisplay_flat() {

    mUi->defaultSnapshotDisplay->setSortingEnabled(false); // Don't sort during modification
    mUi->snapshotDisplay->setSortingEnabled(false);
    clearSnapshotDisplay();

    std::string snapshotPath = getSnapshotBaseDir();

    QSettings settings;
    DeleteInvalidSnapshots deleteInvalidsChoice;
    if (mIsStandAlone || mDidInitialInvalidCheck) {
        // Don't auto-delete or ask in stand-alone mode.
        // Don't auto-delete or ask more than once.
        deleteInvalidsChoice = DeleteInvalidSnapshots::No;
    } else {
        deleteInvalidsChoice = static_cast<DeleteInvalidSnapshots>
                                    (settings.value(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                                                    static_cast<int>(DeleteInvalidSnapshots::Ask))
                                             .toInt());
    }

    // Find all the directories in the snapshot directory
    QDir snapshotDir(snapshotPath.c_str());
    snapshotDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList snapshotList(snapshotDir.entryList());
    QStringList invalidSnapshotNames;

    // Look at all the directories and make a WidgetSnapshotItem for each one
    int nItems = 0;
    bool anItemIsSelected = false;
    for (const QString& fileName : snapshotList) {
        Snapshot theSnapshot(fileName.toStdString().c_str());

        const emulator_snapshot::Snapshot* protobuf = theSnapshot.getGeneralInfo();
        bool snapshotIsValid =
            protobuf &&
            (mIsStandAlone || // Don't checkValid in standalone mode
             theSnapshot.checkValid(
                false /* don't write out the error code to protobuf; we just want
                         to check validity here */));

        // bug: 113037359
        // Don't auto-invalidate quickboot snapshot
        // when switching to file-backed Quickboot from older version.
        if (fc::isEnabled(fc::QuickbootFileBacked) &&
            fileName == Quickboot::kDefaultBootSnapshot) {
            snapshotIsValid = true;
        }

        if (!snapshotIsValid && deleteInvalidsChoice != DeleteInvalidSnapshots::No) {
            invalidSnapshotNames.append(fileName);
        }
        // Decimate the invalid snapshots
        if (!snapshotIsValid && !mIsStandAlone) {
            android::base::ThreadLooper::runOnMainLooper([fileName, this] {
                androidSnapshot_invalidate(fileName.toStdString().c_str());
            });
        }

        QString logicalName(fileName);
        QDateTime snapshotDate;
        if (protobuf) {
            if (protobuf->has_logical_name()) {
                logicalName = protobuf->logical_name().c_str();
            }
            if (protobuf->has_creation_time()) {
                snapshotDate = QDateTime::fromMSecsSinceEpoch(1000LL * protobuf->creation_time());
            }
        }

        // Create a top-level item.
        QTreeWidget* displayWidget;
        if (fileName == Quickboot::kDefaultBootSnapshot) {
            // Use the "default" section of the display.
            if (fc::isEnabled(fc::QuickbootFileBacked)) {
                logicalName = kDefaultBootFileBackedTitleName;
            } else {
                logicalName = kDefaultBootItemName;
            }
            displayWidget = mUi->defaultSnapshotDisplay;
        } else {
            // Use the regular section of the display.
            displayWidget = mUi->snapshotDisplay;
        }

        bool snapshotIsTheParent = (fileName == androidSnapshot_loadedSnapshotFile());

        WidgetSnapshotItem* thisItem =
                new WidgetSnapshotItem(fileName, logicalName, snapshotDate,
                                       snapshotIsValid, snapshotIsTheParent);

        displayWidget->addTopLevelItem(thisItem);

        if (!snapshotIsValid) {
            QFont italic = thisItem->font(COLUMN_NAME);
            italic.setPointSize(italic.pointSize() + 1);
            italic.setItalic(true);
            thisItem->setFont(COLUMN_NAME, italic);
            thisItem->setForeground(COLUMN_NAME, QBrush(Qt::gray));
        } else if (snapshotIsTheParent) {
            // This snapshot was used to start the AVD
            QFont bigBold = thisItem->font(COLUMN_NAME);
            bigBold.setPointSize(bigBold.pointSize() + 2);
            bigBold.setBold(true);
            thisItem->setFont(COLUMN_NAME, bigBold);
            displayWidget->setCurrentItem(thisItem);
            anItemIsSelected = true;
        } else {
            QFont biggish = thisItem->font(COLUMN_NAME);
            biggish.setPointSize(biggish.pointSize() + 1);
            thisItem->setFont(COLUMN_NAME, biggish);
        }

        nItems++;
    }

    if (nItems <= 0) {
        // Hide the lists and say that there are no snapshots
        setShowSnapshotDisplay(false);
        mUi->noneAvailableLabel->setVisible(true);
        return;
    }

    if (!anItemIsSelected) {
        // There are items, but none is selected.
        // Select the first one.
        mUi->defaultSnapshotDisplay->setSortingEnabled(true);
        QTreeWidgetItem* itemZero = mUi->defaultSnapshotDisplay->topLevelItem(0);
        if (itemZero != nullptr && !mIsStandAlone) {
            mUi->defaultSnapshotDisplay->setCurrentItem(itemZero);
        } else {
            // Try the 'normal' list
            mUi->snapshotDisplay->setSortingEnabled(true);
            itemZero = mUi->snapshotDisplay->topLevelItem(0);
            if (itemZero != nullptr) {
                mUi->snapshotDisplay->setCurrentItem(itemZero);
            }
        }
    }

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

    adjustIcons(mUi->defaultSnapshotDisplay);
    adjustIcons(mUi->snapshotDisplay);

    if (!mIsStandAlone && !mDidInitialInvalidCheck && !invalidSnapshotNames.isEmpty()) {
        if (deleteInvalidsChoice == DeleteInvalidSnapshots::Auto) {
            // Delete the invalid snapshots
            for (int idx = 0; idx < invalidSnapshotNames.size(); idx++) {
                QString fileName = invalidSnapshotNames.at(idx);
                android::base::ThreadLooper::runOnMainLooper([fileName, this] {
                    androidSnapshot_delete(fileName.toStdString().c_str());
                    emit(deleteCompleted());
                });
            }
        } else if (deleteInvalidsChoice == DeleteInvalidSnapshots::Ask) {
            // Emit a signal to ask the user. (After the UI actually displays
            // all the information we just loaded.)
            emit askAboutInvalidSnapshots(invalidSnapshotNames);
        }
    }
    mDidInitialInvalidCheck = true;
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


void SnapshotPage::slot_askAboutInvalidSnapshots(QStringList names) {
    if (names.size() <= 0) {
        return;
    }
    // Let's see how much disk these occupy
    System::FileSize totalOccupation = 0;

    for (int idx = 0; idx < names.size(); idx++) {
        System::FileSize snapSize = android::snapshot::folderSize(names.at(idx).toStdString());
        totalOccupation += snapSize;
    }
    QString questionString;
    if (names.size() == 1) {
        questionString = tr("You have one invalid snapshot occupying %1 "
                            "disk space. Do you want to permanently delete it?")
                           .arg(formattedSize(totalOccupation));
    } else {
        questionString = tr("You have %1 invalid snapshots occupying %2 "
                            "disk space. Do you want to permanently delete them?")
                           .arg(QString::number(names.size()),
                                formattedSize(totalOccupation));
    }

    QMessageBox msgBox(QMessageBox::Question,
                       tr("Delete invalid snapshots?"),
                       questionString,
                       (QMessageBox::Yes | QMessageBox::Cancel),
                       this);
    QCheckBox* goAuto = new QCheckBox(tr("Delete automatically from now on"));
    msgBox.setCheckBox(goAuto);

    int selection = msgBox.exec();

    if (selection == QMessageBox::Yes) {
        if (goAuto->isChecked()) {
            QSettings settings;
            settings.setValue(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                              static_cast<int>(DeleteInvalidSnapshots::Auto));
            mUi->deleteInvalidSnapshots->setCurrentIndex(
                    static_cast<int>(DeleteInvalidSnapshotsUiOrder::Auto));
        }
        // Delete the invalid snapshots
        for (int idx = 0; idx < names.size(); idx++) {
            QString fileName = names.at(idx);
            android::base::ThreadLooper::runOnMainLooper([fileName, this] {
                androidSnapshot_delete(fileName.toStdString().c_str());
                emit(deleteCompleted());
            });
        }
    }
}

void SnapshotPage::slot_confirmAutosaveChoiceAndRestart(
    SaveSnapshotOnExit previousSetting,
    SaveSnapshotOnExit nextSetting) {

    QMessageBox msgBox(QMessageBox::Question,
                       tr("Restart emulator?"),
                       tr("The emulator needs to restart\n"
                          "for changes to be effected.\n"
                          "Do you wish to proceed?"),
                       (QMessageBox::Yes | QMessageBox::No),
                       this);

    int selection = msgBox.exec();

    if (selection == QMessageBox::No) {
        switch (previousSetting) {
            case SaveSnapshotOnExit::Always:
            case SaveSnapshotOnExit::Never:
                break;
            case SaveSnapshotOnExit::Ask:
                fprintf(stderr, "%s: WARNING: previous setting for save on exit was Ask!\n", __func__);
            default:
                fprintf(stderr, "%s: WARNING: unknown auto-save setting 0x%x\n",
                        __func__, (unsigned int)previousSetting);
                break;
        }

        changeUiFromSaveOnExitSetting(previousSetting);
        return;
    }

    // Selection was 'yes', apply the setting and restart emulator
    setSaveOnExitChoice(nextSetting);
    // Assumption: Not needed: changeUiFromSaveOnExitSetting(nextSetting),
    // because this was from the combo box index changing.
    android::base::restartEmulator();
}

void SnapshotPage::disableActions() {
    // Disable all the action pushbuttons
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->deleteSnapshot,     theme, false);
    setButtonEnabled(mUi->editSnapshot,       theme, false);
    setButtonEnabled(mUi->loadSnapshot,       theme, false);
    setButtonEnabled(mUi->takeSnapshotButton, theme, false);
}

void SnapshotPage::enableActions() {
    // Enable the appropriate action pushbuttons
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->deleteSnapshot,     theme, mAllowDelete);
    setButtonEnabled(mUi->editSnapshot,       theme, mAllowEdit);
    setButtonEnabled(mUi->loadSnapshot,       theme, mAllowLoad);
    setButtonEnabled(mUi->takeSnapshotButton, theme, mAllowTake);
}

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
                                                android::snapshot::kSnapshotProtobufName);
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

void SnapshotPage::getOutputFileName() {
    // In stand-alone mode, after the user selects a snapshot, we will write
    // the name of that snapshot into our output file. Android Studio will
    // read that file to learn what was selected.
    mOutputFileName = "";

    // Get the name of the file containing parameters from Android Studio
    EmulatorWindow* const ew = emulator_window_get();
    if (!ew || !ew->opts->studio_params) {
        // No file to read
        return;
    }

    // Android Studio created a parameters file for us
    QFile paramFile(ew->opts->studio_params);
    if (!paramFile.open(QFile::ReadOnly | QFile::Text)) {
        // Open failure
        return;
    }
    // Scan the parameters file to get the name of our output file
    while (!paramFile.atEnd()) {
        QByteArray line = paramFile.readLine().trimmed();
        int idx = line.indexOf("=");
        if (idx > 0) {
            QByteArray key = line.left(idx);
            if (key == "snapshotTempFile") {
                QByteArray value = line.right(line.length() - 1 - idx);
                mOutputFileName = value;
                break;
            }
        }
    }
}
