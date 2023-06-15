/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "android/skin/qt/extended-pages/snapshot-page-grpc.h"

#include <algorithm>
#include <string>
#include <utility>

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEasingCurve>
#include <QFile>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRectF>
#include <QSettings>
#include <QSizeF>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QVariant>
#include <QtGui>

#include "aemu/base/ProcessControl.h"
#include "aemu/base/TypeTraits.h"
#include "aemu/base/logging/CLog.h"
#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/base/system/System.h"
#include "android/cmdline-definitions.h"
#include "android/console.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/MetricsWriter.h"
#include "android/metrics/UiEventTracker.h"
#include "android/settings-agent.h"
#include "android/skin/qt/common_settings.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/stylesheet.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Quickboot.h"
#include "google/protobuf/wrappers.pb.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "snapshot.pb.h"
#include "snapshot_service.grpc.pb.h"
#include "snapshot_service.pb.h"
#include "studio_stats.pb.h"

using android::emulation::control::EmulatorGrpcClient;
using android::emulation::control::SnapshotDetails;
using android::emulation::control::SnapshotFilter;
using android::emulation::control::SnapshotList;
using android::emulation::control::SnapshotPackage;
using android::emulation::control::SnapshotScreenshotFile;
using android::emulation::control::SnapshotService;
using android::emulation::control::SnapshotUpdateDescription;
using android::metrics::MetricsReporter;
using android::snapshot::Quickboot;
using Ui::Settings::DeleteInvalidSnapshots;
using Ui::Settings::DeleteInvalidSnapshotsUiOrder;
using Ui::Settings::SaveSnapshotOnExit;
using Ui::Settings::SaveSnapshotOnExitUiOrder;

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif
using namespace android::base;
using namespace android::snapshot;

namespace pb = android_studio;
namespace fc = android::featurecontrol;
using fc::Feature;
static const char CURRENT_SNAPSHOT_ICON_NAME[] = "current_snapshot";
static const char CURRENT_SNAPSHOT_SELECTED_ICON_NAME[] =
        "current_snapshot_selected";
static const char INVALID_SNAPSHOT_ICON_NAME[] = "invalid_snapshot";
static const char INVALID_SNAPSHOT_SELECTED_ICON_NAME[] =
        "invalid_snapshot_selected";
static constexpr char kDefaultBootItemName[] = "Quickboot";
static constexpr char kDefaultBootFileBackedTitleName[] =
        "Quickboot (auto-saved)";
static SaveSnapshotOnExit getSaveOnExitChoice();
static void setSaveOnExitChoice(SaveSnapshotOnExit choice);

extern void SnapshotClearTreeWidget(QTreeWidget* tree);

// Globally accessable from SnapshotPage
// bool userSettingIsDontSaveSnapshot() {
//     return getSaveOnExitChoice() == SaveSnapshotOnExit::Never;
// }

// Well, you could generalize this to
// fmap :: (a -> b) -> f a -> f b
static PackageData fmap(absl::StatusOr<SnapshotPackage*> status_or_ptr) {
    if (!status_or_ptr.ok()) {
        return status_or_ptr.status();
    }
    return absl::StatusOr<std::string>(status_or_ptr.value()->snapshot_id());
}

// A class for items in the QTreeWidget.
class SnapshotPageGrpc::WidgetSnapshotItem : public QTreeWidgetItem {
public:
    // ctor for a top-level item
    WidgetSnapshotItem(const SnapshotDetails snapshot)
        : QTreeWidgetItem((QTreeWidget*)0) {
        mSnapshot.CopyFrom(snapshot);
        std::string name;
        if (snapshot.snapshot_id() == Quickboot::kDefaultBootSnapshot) {
            // Use the "default" section of the display.
            if (fc::isEnabled(fc::QuickbootFileBacked)) {
                name = kDefaultBootFileBackedTitleName;
            } else {
                name = kDefaultBootItemName;
            }
        } else {
            if (snapshot.details().logical_name().empty()) {
                name = snapshot.snapshot_id();
            } else {
                name = snapshot.details().logical_name();
            }
        }
        setText(COLUMN_NAME, QString::fromStdString(name));
    }
    // Customize the sort order: Sort by date
    // (Put invalid items after valid items. Invalid items
    // are sorted by name.)
    bool operator<(const QTreeWidgetItem& other) const {
        WidgetSnapshotItem& otherItem = (WidgetSnapshotItem&)other;
        if (isValid()) {
            if (!otherItem.isValid())
                return true;
            return mSnapshot.details().creation_time() <
                   mSnapshot.details().creation_time();
        } else {
            if (otherItem.isValid())
                return false;
            return mSnapshot.snapshot_id() < otherItem.mSnapshot.snapshot_id();
        }
    }
    QString id() const {
        return QString::fromStdString(mSnapshot.snapshot_id());
    }
    QDateTime dateTime() const {
        return QDateTime::fromMSecsSinceEpoch(
                mSnapshot.details().creation_time());
    }
    QString logicalName() const {
        return data(COLUMN_NAME, Qt::DisplayRole).toString();
    }
    QString description() const {
        return QString::fromStdString(mSnapshot.details().description());
    }
    bool isValid() const {
        return mSnapshot.status() == SnapshotDetails::Compatible ||
               isTheParent();
    }
    bool isTheParent() const {
        return mSnapshot.status() == SnapshotDetails::Loaded;
    }
    emulator_snapshot::Snapshot* details() const {
        return const_cast<emulator_snapshot::Snapshot*>(&mSnapshot.details());
    }
    const SnapshotDetails* snapshot() const { return &mSnapshot; }

private:
    SnapshotDetails mSnapshot;
};

static SnapshotPageGrpc* sInstance = nullptr;

SnapshotPageGrpc::SnapshotPageGrpc(QWidget* parent, bool standAlone)
    : QWidget(parent),
      mIsStandAlone(standAlone),
      mUi(new Ui::SnapshotPageGrpc()),
      mSnapshotTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_SNAPSHOT_TAB)),
      mSnapshotService(EmulatorGrpcClient::me()) {
    mUi->setupUi(this);
    mUi->inProgressLabel->hide();

    if (getConsoleAgents()->settings->android_cmdLineOptions()->read_only) {
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
    mInfoPanelLargeGeo = QRect(
            mInfoPanelSmallGeo.x(), previewGeo.y(), mInfoPanelSmallGeo.width(),
            mInfoPanelSmallGeo.y() + mInfoPanelSmallGeo.height() -
                    previewGeo.y());
    SaveSnapshotOnExit saveOnExitChoice = getSaveOnExitChoice();
    changeUiFromSaveOnExitSetting(saveOnExitChoice);
    for (int i = 0; i < mUi->saveQuickBootOnExit->count(); i++) {
        mUi->saveQuickBootOnExit->setItemData(i, QVariant(i));
    }
    // In file-backed Quickboot, the 'save now' button is always disabled.
    if (fc::isEnabled(fc::QuickbootFileBacked)) {
        mUi->saveOnExitTitle->setText(
                QString(tr("Auto-save current state to Quickboot")));
        // Migrate "Ask" users to "Always"
        if (saveOnExitChoice == SaveSnapshotOnExit::Ask) {
            setSaveOnExitChoice(SaveSnapshotOnExit::Always);
            changeUiFromSaveOnExitSetting(getSaveOnExitChoice());
        }
        for (int i = 0; i < mUi->saveQuickBootOnExit->count();) {
            SaveSnapshotOnExitUiOrder uiOrder =
                    static_cast<SaveSnapshotOnExitUiOrder>(
                            mUi->saveQuickBootOnExit->itemData(i).toInt());
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
            mUi->noneAvailableLabel->setText(
                    QString(tr("Auto-saving Quickboot state...")));
        }
    } else {
        // Save QuickBoot snapshot on exit
        QString avdNameWithUnderscores(
                getConsoleAgents()->settings->hw()->avd_name);
        mUi->saveOnExitTitle->setText(
                QString(tr("Save quick-boot state on exit for AVD: ")) +
                avdNameWithUnderscores.replace('_', ' '));
        // Enable SAVE NOW if we won't overwrite the state on exit
        mUi->saveQuickBootNowButton->setEnabled(saveOnExitChoice !=
                                                SaveSnapshotOnExit::Always);
        // Auto-save not enabled
        mUi->autoSaveNoteLabel->hide();
        mUi->autoSaveNoteLabelIcon->hide();
    }
    QSettings settings;
    DeleteInvalidSnapshots deleteInvalids = static_cast<DeleteInvalidSnapshots>(
            settings.value(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                           static_cast<int>(DeleteInvalidSnapshots::Ask))
                    .toInt());
    DeleteInvalidSnapshotsUiOrder deleteInvalidsUiIdx;
    switch (deleteInvalids) {
        default:
        case DeleteInvalidSnapshots::Ask:
            deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::Ask;
            break;
        case DeleteInvalidSnapshots::Auto:
            deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::Auto;
            break;
        case DeleteInvalidSnapshots::No:
            deleteInvalidsUiIdx = DeleteInvalidSnapshotsUiOrder::No;
            break;
    }
    mUi->deleteInvalidSnapshots->setCurrentIndex(
            static_cast<int>(deleteInvalidsUiIdx));
    if (mIsStandAlone) {
        QRect widgetGeometry = frameGeometry();
        setFixedHeight(widgetGeometry.height() +
                       36);  // Allow for the title bar
        setFixedWidth(widgetGeometry.width());
        setStyleSheet(Ui::stylesheetForTheme(getSelectedTheme()));
        setWindowTitle(tr("Manage snapshots"));
        mUi->loadSnapshot->setVisible(false);
        mUi->deleteSnapshot->setVisible(
                false);  // Cannot delete without QEMU active
        mUi->takeSnapshotButton->setText(tr("CHOOSE SNAPSHOT"));
        mUi->tabWidget->removeTab(1);  // Do not show the Settings tab
    }
    qRegisterMetaType<Ui::Settings::SaveSnapshotOnExit>();
    sInstance = this;

    enableConnections();
}

void SnapshotPageGrpc::enableConnections() {
    // TODO: We need to use QVariants here, otherwise they might now work on linux
    connect(this, SIGNAL(deleteCompleted()), this,
            SLOT(slot_snapshotDeleteCompleted()));
    connect(this, SIGNAL(loadCompleted(PackageData)), this,
            SLOT(slot_snapshotLoadCompleted(PackageData)));
    connect(this, SIGNAL(saveCompleted(PackageData)), this,
            SLOT(slot_snapshotSaveCompleted(PackageData)));
    connect(this, SIGNAL(screenshotLoaded(std::string, SelectionStatus)), this,
            SLOT(on_screenshotLoaded(std::string, SelectionStatus)));
    connect(this, SIGNAL(snapshotsList(std::vector<SnapshotDetails>)), this,
            SLOT(on_snapshotsList(std::vector<SnapshotDetails>)));
    // Queue the "ask" signal so the pop-up doesn't appear until the
    // base window has been populated and raised.
    connect(this,
            SIGNAL(askAboutInvalidSnapshots(std::vector<SnapshotDetails>)),
            this,
            SLOT(slot_askAboutInvalidSnapshots(std::vector<SnapshotDetails>)),
            Qt::QueuedConnection);
    // Likewise, queue the confirm auto-save setting signal so the base window
    // is populated and raised before the pop-up shows.
    connect(this,
            SIGNAL(confirmAutosaveChoiceAndRestart(
                    Ui::Settings::SaveSnapshotOnExit,
                    Ui::Settings::SaveSnapshotOnExit)),
            this,
            SLOT(slot_confirmAutosaveChoiceAndRestart(
                    Ui::Settings::SaveSnapshotOnExit,
                    Ui::Settings::SaveSnapshotOnExit)),
            Qt::QueuedConnection);
}
// static
SnapshotPageGrpc* SnapshotPageGrpc::get() {
    return sInstance;
}

void SnapshotPageGrpc::deleteSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }
    QString logicalName = theItem->logicalName();
    QString id = theItem->id();
    QMessageBox msgBox(
            QMessageBox::Question, tr("Delete snapshot"),
            tr("Do you want to permanently delete<br>snapshot \"%1\"?")
                    .arg(logicalName),
            QMessageBox::Cancel, this);
    QPushButton* deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));
    int selection = msgBox.exec();
    if (selection == QMessageBox::Ok) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        disableActions();
        setOperationInProgress(true);
        mSnapshotService.DeleteSnapshotAsync(
                theItem->snapshot()->snapshot_id(),
                [this](auto _unused) { emit(deleteCompleted()); });
    }
}

void SnapshotPageGrpc::slot_snapshotDeleteCompleted() {
    populateSnapshotDisplay();
    enableActions();
    setOperationInProgress(false);
    QApplication::restoreOverrideCursor();
}

void SnapshotPageGrpc::editSnapshot(const WidgetSnapshotItem* theItem) {
    if (!theItem) {
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    disableActions();
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(new QLabel(tr("Name")));
    QLineEdit* nameEdit = new QLineEdit(this);
    QString oldName = theItem->logicalName();
    nameEdit->setText(oldName);
    nameEdit->selectAll();
    dialogLayout->addWidget(nameEdit);
    dialogLayout->addWidget(new QLabel(tr("Description")));
    QPlainTextEdit* descriptionEdit = new QPlainTextEdit(this);
    QString oldDescription = theItem->description();
    descriptionEdit->setPlainText(oldDescription);
    dialogLayout->addWidget(descriptionEdit);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, Qt::Horizontal);
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
        newDescription != oldDescription) {
        // Something changed. Read the existing Protobuf,
        // update it, and write it back out.
        QApplication::setOverrideCursor(Qt::WaitCursor);
        disableActions();
        QString id = theItem->id();
        SnapshotUpdateDescription update;
        update.set_snapshot_id(theItem->id().toStdString());
        if (!newName.isEmpty()) {
            theItem->details()->set_logical_name(newName.toStdString());
            update.mutable_logical_name()->set_value(
                    theItem->details()->logical_name());
        }
        theItem->details()->set_description(newDescription.toStdString());
        update.mutable_description()->set_value(
                theItem->details()->description());
        // writeProtobuf(id, protobuf);
        mSnapshotService.UpdateSnapshotAsync(update, [this, id](auto status) {
            populateSnapshotDisplay();
            // Select the just-edited item
            highlightItemWithFilename(id);
        });
    }
    enableActions();
    QApplication::restoreOverrideCursor();
}

const SnapshotPageGrpc::WidgetSnapshotItem*
SnapshotPageGrpc::getSelectedSnapshot() {
    QList<QTreeWidgetItem*> selectedItems =
            mUi->snapshotDisplay->selectedItems();
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

void SnapshotPageGrpc::changeUiFromSaveOnExitSetting(
        SaveSnapshotOnExit choice) {
    switch (choice) {
        case SaveSnapshotOnExit::Always:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
            getConsoleAgents()->settings->avdParams()->flags &=
                    !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Ask:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Ask));
            // If we can't ask, we'll treat ASK the same as ALWAYS.
            getConsoleAgents()->settings->avdParams()->flags &=
                    !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Never:
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Never));
            getConsoleAgents()->settings->avdParams()->flags |=
                    AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        default:
            dwarning(
                    "%s: unknown 'Save snapshot on exit' preference value "
                    "0x%x. "
                    "Setting to Always.",
                    __func__, (unsigned int)choice);
            mUi->saveQuickBootOnExit->setCurrentIndex(
                    static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
            getConsoleAgents()->settings->avdParams()->flags &=
                    !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
    }
}

void SnapshotPageGrpc::on_enlargeInfoButton_clicked() {
    // Make the info window grow
    mSnapshotTracker->increment("ENLARGE");
    mUi->preview->lower();  // Let the preview window get covered
    QPropertyAnimation* infoAnimation =
            new QPropertyAnimation(mUi->selectionInfo, "geometry");
    infoAnimation->setDuration(500);  // mSec
    infoAnimation->setStartValue(mInfoPanelSmallGeo);
    infoAnimation->setEndValue(mInfoPanelLargeGeo);
    infoAnimation->setEasingCurve(QEasingCurve::OutCubic);
    infoAnimation->start();
    mInfoWindowIsBig = true;
    on_snapshotDisplay_itemSelectionChanged();
}

void SnapshotPageGrpc::on_reduceInfoButton_clicked() {
    // Make the info window shrink
    QPropertyAnimation* infoAnimation =
            new QPropertyAnimation(mUi->selectionInfo, "geometry");
    infoAnimation->setDuration(500);  // mSec
    infoAnimation->setStartValue(mInfoPanelLargeGeo);
    infoAnimation->setEndValue(mInfoPanelSmallGeo);
    infoAnimation->setEasingCurve(QEasingCurve::OutCubic);
    infoAnimation->start();
    mInfoWindowIsBig = false;
    on_snapshotDisplay_itemSelectionChanged();
}

void SnapshotPageGrpc::on_editSnapshot_clicked() {
    mSnapshotTracker->increment("EDIT");
    editSnapshot(getSelectedSnapshot());
}

void SnapshotPageGrpc::on_deleteSnapshot_clicked() {
    mSnapshotTracker->increment("DELETE");
    deleteSnapshot(getSelectedSnapshot());
}

void SnapshotPageGrpc::on_loadSnapshot_clicked() {
    mSnapshotTracker->increment("LOAD");
    const WidgetSnapshotItem* theItem = getSelectedSnapshot();
    if (!theItem)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    disableActions();
    setOperationInProgress(true);
    mSnapshotService.LoadSnapshotAsync(theItem->snapshot()->snapshot_id(),
                                       [this](auto status) {
                                           DD("Emit loadCompleted");
                                           emit(loadCompleted(fmap(status)));
                                       });
}

void SnapshotPageGrpc::on_saveQuickBootNowButton_clicked() {
    mSnapshotTracker->increment("SAVE_BOOT");
    setEnabled(false);
    setOperationInProgress(true);
    // Invoke the snapshot save function.
    // But don't run it on the UI thread.
    mSnapshotService.SaveSnapshotAsync(
            Quickboot::kDefaultBootSnapshot,
            [this](auto status) { emit(saveCompleted(fmap(status))); });
}

void SnapshotPageGrpc::on_loadQuickBootNowButton_clicked() {
    mSnapshotTracker->increment("LOAD_BOOT");
    setEnabled(false);
    setOperationInProgress(true);
    // Invoke the snapshot load function.
    // But don't run it on the UI thread.
    mSnapshotService.LoadSnapshotAsync(
            Quickboot::kDefaultBootSnapshot,
            [this](auto status) { emit(loadCompleted(fmap(status))); });
}

void SnapshotPageGrpc::slot_snapshotLoadCompleted(PackageData status) {
    DD("slot_snapshotLoadCompleted");
    setOperationInProgress(false);
    setEnabled(true);
    if (!status.ok()) {
        enableActions();
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Snapshot did not load"), tr("Load snapshot"));
        return;
    }
    // Refresh the list of available snapshots
    populateSnapshotDisplay();
    highlightItemWithFilename(QString::fromStdString(*status));
    enableActions();
    QApplication::restoreOverrideCursor();
}

void SnapshotPageGrpc::on_defaultSnapshotDisplay_itemSelectionChanged() {
    mSnapshotTracker->increment("SELECT_DEFAULT");
    QList<QTreeWidgetItem*> selectedItems =
            mUi->defaultSnapshotDisplay->selectedItems();
    if (selectedItems.size() > 0) {
        // We have the selection, de-select the other list
        mUi->snapshotDisplay->clearSelection();
    }
    updateAfterSelectionChanged();
}

void SnapshotPageGrpc::on_snapshotDisplay_itemSelectionChanged() {
    mSnapshotTracker->increment("SELECT");
    QList<QTreeWidgetItem*> selectedItems =
            mUi->snapshotDisplay->selectedItems();
    if (selectedItems.size() > 0) {
        // We have the selection, de-select the other list
        mUi->defaultSnapshotDisplay->clearSelection();
    }
    updateAfterSelectionChanged();
}

static SaveSnapshotOnExit getSaveOnExitChoice() {
    // This setting belongs to the AVD, not to the entire Emulator.
    SaveSnapshotOnExit userChoice(SaveSnapshotOnExit::Always);
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        userChoice = static_cast<SaveSnapshotOnExit>(
                avdSpecificSettings
                        .value(Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                               static_cast<int>(SaveSnapshotOnExit::Always))
                        .toInt());
    }
    return userChoice;
}

static void setSaveOnExitChoice(SaveSnapshotOnExit choice) {
    // Save for only this AVD
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                                     static_cast<int>(choice));
    }
}

void SnapshotPageGrpc::on_saveQuickBootOnExit_currentIndexChanged(int uiIndex) {
    mSnapshotTracker->increment("SELECT_BOOT");
    SaveSnapshotOnExit preferenceValue;
    switch (static_cast<SaveSnapshotOnExitUiOrder>(uiIndex)) {
        case SaveSnapshotOnExitUiOrder::Never:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()
                                      ->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_no(
                        1 + counts->quickboot_selection_no());
            });
            preferenceValue = SaveSnapshotOnExit::Never;
            getConsoleAgents()->settings->avdParams()->flags |=
                    AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExitUiOrder::Ask:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()
                                      ->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_ask(
                        1 + counts->quickboot_selection_ask());
            });
            preferenceValue = SaveSnapshotOnExit::Ask;
            getConsoleAgents()->settings->avdParams()->flags &=
                    !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        default:
        case SaveSnapshotOnExitUiOrder::Always:
            MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
                auto counts = event->mutable_emulator_details()
                                      ->mutable_snapshot_ui_counts();
                counts->set_quickboot_selection_yes(
                        1 + counts->quickboot_selection_yes());
            });
            getConsoleAgents()->settings->avdParams()->flags &=
                    !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
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
            emit(confirmAutosaveChoiceAndRestart(previousChoice,
                                                 preferenceValue));
        }
    } else {
        // Enable SAVE STATE NOW if we won't overwrite the state on exit
        mUi->saveQuickBootNowButton->setEnabled(preferenceValue !=
                                                SaveSnapshotOnExit::Always);
        setSaveOnExitChoice(preferenceValue);
    }
}

void SnapshotPageGrpc::on_deleteInvalidSnapshots_currentIndexChanged(
        int uiIndex) {
    mSnapshotTracker->increment("DELETE_INVALID");
    DeleteInvalidSnapshots preferenceValue;
    switch (static_cast<DeleteInvalidSnapshotsUiOrder>(uiIndex)) {
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
    settings.setValue(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                      static_cast<int>(preferenceValue));
    // Re-enable our check, so we act on this new setting
    // when the user goes back to look at the list
    mDidInitialInvalidCheck = false;
}

void SnapshotPageGrpc::on_tabWidget_currentChanged(int index) {
    // The previous tab may have changed how this tab
    // should look. Refresh the snapshot list to be safe.
    populateSnapshotDisplay();
}

void SnapshotPageGrpc::updateAfterSelectionChanged() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    disableActions();
    adjustIcons(mUi->defaultSnapshotDisplay);
    adjustIcons(mUi->snapshotDisplay);
    QString selectionInfoString;
    QString descriptionString;
    QString simpleName;
    const WidgetSnapshotItem* theItem = getSelectedSnapshot();
    DD("Selected %p", theItem);
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
        simpleName = theItem->id();
        System::FileSize snapSize =
                android::snapshot::folderSize(simpleName.toStdString());
        QString description = theItem->description();
        if (!description.isEmpty()) {
            descriptionString = QString("<p><big>%1</big>")
                                        .arg(description.replace('<', "&lt;")
                                                     .replace('\n', "<br>"));
        }
        if (theItem->isValid()) {
            selectedItemStatus = SelectionStatus::Valid;
            QLocale locale;
            selectionInfoString =
                    tr("<big><b>%1</b></big><br>"
                       "%2, captured %3<br>"
                       "File: %4"
                       "%5")
                            .arg(logicalName, formattedSize(snapSize),
                                 theItem->dateTime().isValid()
                                         ? locale.toString(theItem->dateTime(),
                                                           QLocale::FormatType::
                                                                   ShortFormat)
                                         : tr("(unknown)"),
                                 simpleName, descriptionString);
            bool isFileBacked =
                    theItem->logicalName() == kDefaultBootFileBackedTitleName;
            mAllowLoad = !isFileBacked;
            mAllowDelete = !isFileBacked;
            // Cannot edit the default snapshot
            mAllowEdit = !isFileBacked &&
                         (simpleName != Quickboot::kDefaultBootSnapshot);
            mAllowTake = mAllowEdit || !mIsStandAlone;
        } else {
            // Invalid snapshot directory
            selectedItemStatus = SelectionStatus::Invalid;
            selectionInfoString =
                    tr("<big><b>%1</b></big><br>"
                       "%2<br>"
                       "File: %3"
                       "<div style=\"color:red\">Invalid snapshot</div> %4")
                            .arg(logicalName, formattedSize(snapSize),
                                 simpleName, descriptionString);
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
    if (!mInfoWindowIsBig && mUi->preview->isVisible() && theItem) {
        DD("Loading screenshot for %s",
           theItem->snapshot()->snapshot_id().c_str());
        mSnapshotService.GetScreenshotAsync(
                theItem->snapshot()->snapshot_id(),
                [this, selectedItemStatus](auto status) {
                    if (status.ok()) {
                        SnapshotScreenshotFile* file = *status;
                        std::string pixels = std::move(file->data());
                        emit(screenshotLoaded(pixels, selectedItemStatus));
                    }
                });
    }
    enableActions();
    QApplication::restoreOverrideCursor();
}

// This method is used when the small region is used for
// the snapshot info. In this case, we do not allow the
// region to scroll. If the info text does not fit, we
// trim off enough that we can append "  ..." and still
// have the result fit.
QString SnapshotPageGrpc::elideSnapshotInfo(QString fullString) {
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
    while (rightLength - leftLength > 1) {
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
        lastLessPosition > lastGreaterPosition) {
        leftLength = lastLessPosition;
    }
    // Trim it and add "  ..."
    return (fullString.left(leftLength) + dotdotdot);
}

// Adjust the icons for selected vs non-selected rows
void SnapshotPageGrpc::adjustIcons(QTreeWidget* theDisplayList) {
    QTreeWidgetItemIterator iter(theDisplayList);
    for (; *iter; iter++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iter);
        QIcon decoration;
        if (!item->isValid()) {
            decoration = item->isSelected()
                                 ? getIconForCurrentTheme(
                                           INVALID_SNAPSHOT_SELECTED_ICON_NAME)
                                 : getIconForCurrentTheme(
                                           INVALID_SNAPSHOT_ICON_NAME);
        } else if (item->isTheParent()) {
            decoration = item->isSelected()
                                 ? getIconForCurrentTheme(
                                           CURRENT_SNAPSHOT_SELECTED_ICON_NAME)
                                 : getIconForCurrentTheme(
                                           CURRENT_SNAPSHOT_ICON_NAME);
        }
        item->setIcon(COLUMN_ICON, decoration);
    }
}

// Displays the preview image of the selected snapshot
void SnapshotPageGrpc::on_screenshotLoaded(std::string image,
                                           SelectionStatus itemStatus) {
    static const QString invalidSnapText =
            tr("<div style=\"text-align:center\">"
               "<p style=\"color:red\"><big>Invalid snapshot</big></p>"
               "<p style=\"color:%1\"><big>Delete this snapshot to free disk "
               "space</big></p>"
               "</div>");
    static const QString noImageText =
            tr("<div style=\"text-align:center\">"
               "<p style=\"color:%1\"><big>This snapshot does not have a "
               "preview image</big></p>"
               "</div>");
    mPreviewScene.clear();  // Frees previewItem
    QGraphicsPixmapItem* previewItem;
    bool haveImage = false;
    if (!image.empty()) {
        QPixmap pMap;
        QByteArray data = QByteArray::fromStdString(image);
        pMap.loadFromData(data, "PNG");
        previewItem = new QGraphicsPixmapItem(pMap);
        // Is there really a preview image?
        QRectF imageRect = previewItem->boundingRect();
        haveImage = (imageRect.width() > 1.0 && imageRect.height() > 1.0);
    }
    if (haveImage) {
        // Display the image
        mPreviewScene.addItem(previewItem);
        mPreviewScene.setSceneRect(0, 0, 0, 0);  // Reset to default
        mUi->preview->setScene(&mPreviewScene);
        mUi->preview->fitInView(previewItem, Qt::KeepAspectRatio);
    } else {
        // Display some text saying why there is no image
        mUi->preview->items().clear();
        QGraphicsTextItem* textItem = new QGraphicsTextItem;
        const QString& textColor =
                Ui::stylesheetValues(getSelectedTheme())[Ui::THEME_TEXT_COLOR];
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
        int offset = (mUi->preview->height() * 3) /
                     8;  // Put the text near the center, vertically
        mPreviewScene.setSceneRect(0, -offset, mUi->preview->width(),
                                   mUi->preview->height());
        mUi->preview->setScene(&mPreviewScene);
        mUi->preview->fitInView(0, 0, mUi->preview->width(),
                                mUi->preview->height(), Qt::KeepAspectRatio);
    }
}

void SnapshotPageGrpc::showEvent(QShowEvent* ee) {
    populateSnapshotDisplay();
}

// In normal mode, this button is used to capture a snapshot.
// In stand-alone mode, it is used to choose the selected
// snapshot.
void SnapshotPageGrpc::on_takeSnapshotButton_clicked() {
    mSnapshotTracker->increment("TAKE_SNAPSHOT");
    setOperationInProgress(true);
    if (mIsStandAlone) {
        mMadeSelection = true;
        close();
    } else {
        setEnabled(false);
        QString snapshotName("snap_" + QDateTime::currentDateTime().toString(
                                               "yyyy-MM-dd_HH-mm-ss"));
        QApplication::setOverrideCursor(Qt::WaitCursor);
        disableActions();
        mSnapshotService.SaveSnapshotAsync(
                snapshotName.toStdString(), [this](auto status) {
                    DD("Snapshot saved, emitting saveCompleted");
                    emit(saveCompleted(fmap(status)));
                });
    }
}

// This function is invoked only in stand-alone mode. It runs
// when the user clicks either takeSnapshot or X
void SnapshotPageGrpc::closeEvent(QCloseEvent* closeEvent) {
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
    QString selectedFile = (selectedItem ? selectedItem->id() : "");
    QFile outputFile(mOutputFileName);
    if (outputFile.open(QFile::WriteOnly | QFile::Text)) {
        QString selectionString = "selectedSnapshotFile=" + selectedFile + "\n";
        outputFile.write(selectionString.toUtf8());
        outputFile.close();
    }
}

void SnapshotPageGrpc::slot_snapshotSaveCompleted(PackageData status) {
    DD("slot_snapshotSaveCompleted");
    setOperationInProgress(false);
    setEnabled(true);
    if (!status.ok()) {
        enableActions();
        QApplication::restoreOverrideCursor();
        showErrorDialog(tr("Could not save snapshot"), tr("Take snapshot"));
        return;
    }
    // Refresh the list of available snapshots
    populateSnapshotDisplay();
    highlightItemWithFilename(QString::fromStdString(*status));
    enableActions();
    QApplication::restoreOverrideCursor();
}

void SnapshotClearTreeWidget(QTreeWidget* tree) {
#ifdef __APPLE__
// bug: 112196179
// QAccessibility is buggy on macOS.  We need to remove tree
// items one by one, or QAccessbility will go out of sync and cause a
// segfault.
// More background info:
//
https
    :  // blog.inventic.eu/2015/05/crash-in-qtreewidget-qtreeview-index-mapping-on-mac-osx-10-10-part-iii/
    while (tree->topLevelItemCount()) {
        delete tree->takeTopLevelItem(0);
    }
#endif
    tree->clear();
}

void SnapshotPageGrpc::clearSnapshotDisplay() {
    SnapshotClearTreeWidget(mUi->defaultSnapshotDisplay);
    SnapshotClearTreeWidget(mUi->snapshotDisplay);
}

void SnapshotPageGrpc::setShowSnapshotDisplay(bool show) {
    mUi->defaultSnapshotDisplay->setVisible(show);
    mUi->snapshotDisplay->setVisible(show);
}

void SnapshotPageGrpc::setOperationInProgress(bool inProgress) {
    if (inProgress) {
        setShowSnapshotDisplay(false);
        mUi->inProgressLabel->show();
    } else {
        setShowSnapshotDisplay(true);
        mUi->inProgressLabel->hide();
    }
}

void SnapshotPageGrpc::populateSnapshotDisplay() {
    if (getConsoleAgents()->settings->android_cmdLineOptions()->read_only) {
        // Leave the list empty because snapshots
        // are disabled in read-only mode
        return;
    }
    // (In the future, we may also want a hierarchical display.)
    mSnapshotService.ListAllSnapshotsAsync([this](auto snapshots) {
        DD("All snapshots were listed");
        if (snapshots.ok()) {
            SnapshotList* list = *snapshots;
            std::vector<SnapshotDetails> details;
            std::copy(list->snapshots().begin(), list->snapshots().end(),
                      std::back_inserter(details));
            DD("Emit snapshotList");
            emit(snapshotsList(std::move(details)));
        } else {
            derror("Unable to retrieve snapshots due to: %s",
                   snapshots.status().message());
        }
    });
}

// Populate the list of snapshot WITHOUT the hierarchy of parentage
void SnapshotPageGrpc::on_snapshotsList(
        std::vector<SnapshotDetails> snapshots) {
    DD("Updating snapshot list with (%d) items", snapshots.size());
    mUi->defaultSnapshotDisplay->setSortingEnabled(
            false);  // Don't sort during modification
    mUi->snapshotDisplay->setSortingEnabled(false);
    clearSnapshotDisplay();
    QSettings settings;
    DeleteInvalidSnapshots deleteInvalidsChoice;
    if (mIsStandAlone || mDidInitialInvalidCheck) {
        // Don't auto-delete or ask in stand-alone mode.
        // Don't auto-delete or ask more than once.
        deleteInvalidsChoice = DeleteInvalidSnapshots::No;
    } else {
        deleteInvalidsChoice = static_cast<DeleteInvalidSnapshots>(
                settings.value(Ui::Settings::DELETE_INVALID_SNAPSHOTS,
                               static_cast<int>(DeleteInvalidSnapshots::Ask))
                        .toInt());
    }
    std::vector<SnapshotDetails> invalidSnapshots;
    // Look at all the directories and make a WidgetSnapshotItem for each one
    int nItems = 0;
    bool anItemIsSelected = false;
    for (const SnapshotDetails& aSnapshot : snapshots) {
        // Snapshot theSnapshot(id.toStdString().c_str());
        bool snapshotIsValid =
                aSnapshot.status() != SnapshotDetails::Incompatible;
        // bug: 113037359
        // Don't auto-invalidate quickboot snapshot
        // when switching to file-backed Quickboot from older version.
        if (fc::isEnabled(fc::QuickbootFileBacked) &&
            aSnapshot.snapshot_id() == Quickboot::kDefaultBootSnapshot) {
            snapshotIsValid = true;
        }
        if (!snapshotIsValid &&
            deleteInvalidsChoice != DeleteInvalidSnapshots::No) {
            invalidSnapshots.push_back(aSnapshot);
        }
        // Decimate the invalid snapshots
        if (!snapshotIsValid && !mIsStandAlone) {
            mSnapshotService.DeleteSnapshotAsync(aSnapshot.snapshot_id(),
                                                 [](auto status) {});
        }
        // Create a top-level item.
        QTreeWidget* displayWidget;
        if (aSnapshot.snapshot_id() == Quickboot::kDefaultBootSnapshot) {
            displayWidget = mUi->defaultSnapshotDisplay;
        } else {
            // Use the regular section of the display.
            displayWidget = mUi->snapshotDisplay;
        }
        WidgetSnapshotItem* thisItem = new WidgetSnapshotItem(aSnapshot);
        displayWidget->addTopLevelItem(thisItem);
        bool snapshotIsTheParent =
                aSnapshot.status() == SnapshotDetails::Loaded;
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
        DD("Hiding list, we have %d items", nItems);
        // Hide the lists and say that there are no snapshots
        setShowSnapshotDisplay(false);
        mUi->noneAvailableLabel->setVisible(true);
        return;
    }
    if (!anItemIsSelected) {
        DD("There are items, but none is selected, selecting first one");
        // There are items, but none is selected.
        // Select the first one.
        mUi->defaultSnapshotDisplay->setSortingEnabled(true);
        QTreeWidgetItem* itemZero =
                mUi->defaultSnapshotDisplay->topLevelItem(0);
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
    mUi->defaultSnapshotDisplay->header()->setSectionResizeMode(
            COLUMN_ICON, QHeaderView::Fixed);
    mUi->defaultSnapshotDisplay->header()->resizeSection(COLUMN_ICON, 36);
    mUi->defaultSnapshotDisplay->header()->setSectionResizeMode(
            COLUMN_NAME, QHeaderView::Stretch);
    mUi->defaultSnapshotDisplay->setSortingEnabled(true);
    mUi->snapshotDisplay->header()->setStretchLastSection(false);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_ICON,
                                                         QHeaderView::Fixed);
    mUi->snapshotDisplay->header()->resizeSection(COLUMN_ICON, 36);
    mUi->snapshotDisplay->header()->setSectionResizeMode(COLUMN_NAME,
                                                         QHeaderView::Stretch);
    mUi->snapshotDisplay->setSortingEnabled(true);
    adjustIcons(mUi->defaultSnapshotDisplay);
    adjustIcons(mUi->snapshotDisplay);
    if (!mIsStandAlone && !mDidInitialInvalidCheck &&
        !invalidSnapshots.empty()) {
        if (deleteInvalidsChoice == DeleteInvalidSnapshots::Auto) {
            // Delete the invalid snapshots
            for (const auto toRemove : invalidSnapshots) {
                mSnapshotService.DeleteSnapshotAsync(
                        toRemove.snapshot_id(),
                        [this](auto _unused) { populateSnapshotDisplay(); });
            }
        } else if (deleteInvalidsChoice == DeleteInvalidSnapshots::Ask) {
            // Emit a signal to ask the user. (After the UI actually displays
            // all the information we just loaded.)
            emit askAboutInvalidSnapshots(invalidSnapshots);
        }
    }
    mDidInitialInvalidCheck = true;
}

void SnapshotPageGrpc::slot_askAboutInvalidSnapshots(
        std::vector<SnapshotDetails> snapshots) {
    if (snapshots.size() <= 0) {
        return;
    }
    // Let's see how much disk these occupy
    System::FileSize totalOccupation = 0;
    for (const auto& snapshot : snapshots) {
        totalOccupation += snapshot.size();
    }
    QString questionString;
    if (snapshots.size() == 1) {
        questionString = tr("You have one invalid snapshot occupying %1 "
                            "disk space. Do you want to permanently delete it?")
                                 .arg(formattedSize(totalOccupation));
    } else {
        questionString =
                tr("You have %1 invalid snapshots occupying %2 "
                   "disk space. Do you want to permanently delete them?")
                        .arg(QString::number(snapshots.size()),
                             formattedSize(totalOccupation));
    }
    QMessageBox msgBox(QMessageBox::Question, tr("Delete invalid snapshots?"),
                       questionString, (QMessageBox::Yes | QMessageBox::Cancel),
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
        for (const auto& snapshot : snapshots) {
            mSnapshotService.DeleteSnapshotAsync(snapshot.snapshot_id(),
                                                 [](auto _unused) {});
        }
    }
}

void SnapshotPageGrpc::slot_confirmAutosaveChoiceAndRestart(
        SaveSnapshotOnExit previousSetting,
        SaveSnapshotOnExit nextSetting) {
    QMessageBox msgBox(QMessageBox::Question, tr("Restart emulator?"),
                       tr("The emulator needs to restart\n"
                          "for changes to be effected.\n"
                          "Do you wish to proceed?"),
                       (QMessageBox::Yes | QMessageBox::No), this);
    int selection = msgBox.exec();
    if (selection == QMessageBox::No) {
        switch (previousSetting) {
            case SaveSnapshotOnExit::Always:
            case SaveSnapshotOnExit::Never:
                break;
            case SaveSnapshotOnExit::Ask:
                dwarning("%s: previous setting for save on exit was Ask!",
                         __func__);
            default:
                dwarning("%s: unknown auto-save setting 0x%x", __func__,
                         (unsigned int)previousSetting);
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
void SnapshotPageGrpc::disableActions() {
    // Disable all the action pushbuttons
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->deleteSnapshot, theme, false);
    setButtonEnabled(mUi->editSnapshot, theme, false);
    setButtonEnabled(mUi->loadSnapshot, theme, false);
    setButtonEnabled(mUi->takeSnapshotButton, theme, false);
}
void SnapshotPageGrpc::enableActions() {
    // Enable the appropriate action pushbuttons
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->deleteSnapshot, theme, mAllowDelete);
    setButtonEnabled(mUi->editSnapshot, theme, mAllowEdit);
    setButtonEnabled(mUi->loadSnapshot, theme, mAllowLoad);
    setButtonEnabled(mUi->takeSnapshotButton, theme, mAllowTake);
}

void SnapshotPageGrpc::highlightItemWithFilename(const QString& id) {
    // Try the main list
    QTreeWidgetItemIterator iterMain(mUi->snapshotDisplay);
    for (; *iterMain; iterMain++) {
        WidgetSnapshotItem* item = static_cast<WidgetSnapshotItem*>(*iterMain);
        if (id == item->id()) {
            mUi->snapshotDisplay->setCurrentItem(item);
            return;
        }
    }
    // Try the default snapshot list
    QTreeWidgetItemIterator iterDefault(mUi->defaultSnapshotDisplay);
    for (; *iterDefault; iterDefault++) {
        WidgetSnapshotItem* item =
                static_cast<WidgetSnapshotItem*>(*iterDefault);
        if (id == item->id()) {
            mUi->defaultSnapshotDisplay->setCurrentItem(item);
            return;
        }
    }
}

QString SnapshotPageGrpc::formattedSize(qint64 theSize) {
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