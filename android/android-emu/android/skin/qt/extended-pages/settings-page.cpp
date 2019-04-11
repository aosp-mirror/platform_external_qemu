// Copyright (C) 2015-2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/settings-page.h"

#include "android/base/files/PathUtils.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/FramelessDetector.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"

#ifndef SNAPSHOT_CONTROLS // TODO:jameskaye Remove when Snapshot controls are fully enabled
#include "android/base/async/ThreadLooper.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"

#include <QApplication>
#endif

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#ifndef SNAPSHOT_CONTROLS // TODO:jameskaye Remove when Snapshot controls are fully enabled
using Ui::Settings::SaveSnapshotOnExit;
using Ui::Settings::SaveSnapshotOnExitUiOrder;
using android::metrics::MetricsReporter;

namespace pb = android_studio;

static SaveSnapshotOnExit getSaveOnExitChoice();
#endif

// Helper function to set the contents of a QLineEdit.
static void setElidedText(QLineEdit* line_edit, const QString& text) {
    QFontMetrics font_metrics(line_edit->font());
    line_edit->setText(
            font_metrics.elidedText(text, Qt::ElideRight, line_edit->width() * 0.9));
}

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent), mAdb(nullptr), mUi(new Ui::SettingsPage()) {
    mUi->setupUi(this);
    mUi->set_saveLocBox->installEventFilter(this);
    mUi->set_adbPathBox->installEventFilter(this);

    QString savePath =
        QDir::toNativeSeparators(getScreenshotSaveDirectory());

    if (savePath.isEmpty()) {
        mUi->set_saveLocBox->setText(tr("None"));
    } else {
        setElidedText(mUi->set_saveLocBox, savePath);
    }

    // ADB path
    QSettings settings;

    bool autoFindAdb =
            settings.value(Ui::Settings::AUTO_FIND_ADB, true).toBool();
    mUi->set_autoFindAdb->setChecked(autoFindAdb);
    on_set_autoFindAdb_toggled(autoFindAdb);

    QString adbPath = settings.value(Ui::Settings::ADB_PATH, "").toString();
    adbPath = QDir::toNativeSeparators(adbPath);
    setElidedText(mUi->set_adbPathBox, adbPath);

    // Dark/Light theme
    SettingsTheme theme = getSelectedTheme();
    mUi->set_themeBox->setCurrentIndex(static_cast<int>(theme));

    connect(mUi->set_forwardShortcutsToDevice, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(onForwardShortcutsToDeviceChanged(int)));

    // "Send keyboard shortcuts": a pull-down that acts like a checkbox
    bool shortcutBool = settings.value(
               Ui::Settings::FORWARD_SHORTCUTS_TO_DEVICE, false).toBool();

    mUi->set_forwardShortcutsToDevice->setCurrentIndex( shortcutBool ? 1 : 0 );

    // Show a frame around the device?
    mUi->set_frameAlways->setChecked(
        FramelessDetector::isFramelessOk() ?
            settings.value(Ui::Settings::FRAME_ALWAYS, false).toBool() : true);

#ifdef __linux__
    // "Always on top" is not supported for Linux (see emulator-qt-window.cpp)
    // Make the control invisible
    mUi->set_onTopTitle->hide();
    mUi->set_onTop->hide();
#else // Windows or OSX
    bool onTopOnly = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    mUi->set_onTop->setCheckState(onTopOnly ? Qt::Checked : Qt::Unchecked);
#endif

    initProxy();

    // Crash reporting
    Ui::Settings::CRASHREPORT_PREFERENCE_VALUE report_pref =
        static_cast<Ui::Settings::CRASHREPORT_PREFERENCE_VALUE>(
            settings.value(Ui::Settings::CRASHREPORT_PREFERENCE, 0).toInt());

    switch (report_pref) {
        case Ui::Settings::CRASHREPORT_PREFERENCE_ASK:
            mUi->set_crashReportPrefComboBox->setCurrentIndex(
                    Ui::Settings::CRASHREPORT_COMBOBOX_ASK);
            break;
        case Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS:
            mUi->set_crashReportPrefComboBox->setCurrentIndex(
                    Ui::Settings::CRASHREPORT_COMBOBOX_ALWAYS);
            break;
        case Ui::Settings::CRASHREPORT_PREFERENCE_NEVER:
            mUi->set_crashReportPrefComboBox->setCurrentIndex(
                    Ui::Settings::CRASHREPORT_COMBOBOX_NEVER);
            break;
        default:
            fprintf(stderr,
                    "%s: warning: unknown crash report preference value 0x%x. "
                    "Setting to Ask.\n",
                    __func__, (unsigned int)report_pref);
            mUi->set_crashReportPrefComboBox->setCurrentIndex(
                    Ui::Settings::CRASHREPORT_COMBOBOX_ASK);
            break;
    }

#ifndef SNAPSHOT_CONTROLS // TODO:jameskaye Remove this when Snapshot controls are fully enabled
    // This is interim code.
    if (android::featurecontrol::isEnabled(android::featurecontrol::GenericSnapshotsUI)) {
        // The Emulator features file has Generic Snapshots UI enabled, so we
        // show a separate Snapshots sub-window, and the snapshots controls on
        // this page are not used.
        //
        // Hide the few Snapshot controls here.
        mUi->set_saveOnExitTitle->hide();
        mUi->set_saveSnapshotOnExit->hide();
        mUi->set_loadSnapNowButton->hide();
        mUi->set_saveSnapNowButton->hide();
    } else {
        // The Emulator features file does NOT have Generic Snapshots UI enabled.
        // We do not show a separate Snapshots sub-window, so we need the few
        // snapshots controls on this page.

        // Save snapshot on exit
        QString avdNameWithUnderscores(android_hw->avd_name);

        mUi->set_saveOnExitTitle->setText(QString(tr("Save quick-boot state on exit for AVD: "))
                                       + avdNameWithUnderscores.replace('_', ' '));

        SaveSnapshotOnExit saveOnExitChoice = getSaveOnExitChoice();
        switch (saveOnExitChoice) {
            case SaveSnapshotOnExit::Always:
                mUi->set_saveSnapshotOnExit->setCurrentIndex(
                        static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
                android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
                break;
            case SaveSnapshotOnExit::Ask:
                mUi->set_saveSnapshotOnExit->setCurrentIndex(
                        static_cast<int>(SaveSnapshotOnExitUiOrder::Ask));
                // If we can't ask, we'll treat ASK the same as ALWAYS.
                android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
                break;
            case SaveSnapshotOnExit::Never:
                mUi->set_saveSnapshotOnExit->setCurrentIndex(
                        static_cast<int>(SaveSnapshotOnExitUiOrder::Never));
                android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
                break;
            default:
                fprintf(stderr,
                        "%s: warning: unknown 'Save snapshot on exit' preference value 0x%x. "
                        "Setting to Always.\n",
                        __func__, (unsigned int)saveOnExitChoice);
                mUi->set_saveSnapshotOnExit->setCurrentIndex(
                        static_cast<int>(SaveSnapshotOnExitUiOrder::Always));
                android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
                break;
        }
        // Enable SAVE NOW if we won't overwrite the state on exit
        mUi->set_saveSnapNowButton->setEnabled(saveOnExitChoice != SaveSnapshotOnExit::Always);
    }

#endif
    // OpenGL ES renderer
    for (int i = 0; i < mUi->set_glesBackendPrefComboBox->count(); i++) {
        mUi->set_glesBackendPrefComboBox->setItemData(i, QVariant(i));
    }

#ifndef _WIN32
    mDisableANGLE = true;
#endif

    if (mDisableANGLE) {
        for (int i = 0; i < mUi->set_glesBackendPrefComboBox->count();) {
            WinsysPreferredGlesBackend backendPreference =
                (WinsysPreferredGlesBackend)
                (mUi->set_glesBackendPrefComboBox->itemData(i).toInt());
            switch (backendPreference) {
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE:
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE9:
                    mUi->set_glesBackendPrefComboBox->removeItem(i);
                    break;
                default:
                    ++i;
                    break;
            }
        }
    }

    WinsysPreferredGlesBackend settings_glesbackend_pref =
        static_cast<WinsysPreferredGlesBackend>(
            settings.value(Ui::Settings::GLESBACKEND_PREFERENCE, 0).toInt());

    for (int i = 0; i < mUi->set_glesBackendPrefComboBox->count(); i++) {
        WinsysPreferredGlesBackend backendPreference =
            (WinsysPreferredGlesBackend)
            (mUi->set_glesBackendPrefComboBox->itemData(i).toInt());

        if ((int)settings_glesbackend_pref == backendPreference) {
            switch (settings_glesbackend_pref) {
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE:
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE9:
                    if (mDisableANGLE) break;
                case WINSYS_GLESBACKEND_PREFERENCE_AUTO:
                case WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER:
                case WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL:
                    mUi->set_glesBackendPrefComboBox->setCurrentIndex(i);
                    break;
                default:
                    fprintf(stderr,
                            "%s: warning: unknown GLES backend preference value 0x%x. "
                            "Setting to auto.\n",
                            __func__, (unsigned int)settings_glesbackend_pref);
                    mUi->set_glesBackendPrefComboBox->setCurrentIndex(
                            WINSYS_GLESBACKEND_PREFERENCE_AUTO);
                    break;
            }
        }
    }

    WinsysPreferredGlesApiLevel glesapilevel_pref =
        static_cast<WinsysPreferredGlesApiLevel>(
            settings.value(Ui::Settings::GLESAPILEVEL_PREFERENCE, 0).toInt());

    switch (glesapilevel_pref) {
    case WINSYS_GLESAPILEVEL_PREFERENCE_AUTO:
        mUi->set_glesApiLevelPrefComboBox->setCurrentIndex(
                WINSYS_GLESAPILEVEL_PREFERENCE_AUTO);
        break;
    case WINSYS_GLESAPILEVEL_PREFERENCE_MAX:
        mUi->set_glesApiLevelPrefComboBox->setCurrentIndex(
                WINSYS_GLESAPILEVEL_PREFERENCE_MAX);
        break;
    case WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT:
        mUi->set_glesApiLevelPrefComboBox->setCurrentIndex(
                WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT);
        break;
    default:
        fprintf(stderr,
                "%s: warning: unknown GLES API level preference value 0x%x. "
                "Setting to Auto.\n",
                __func__, (unsigned int)glesapilevel_pref);
        mUi->set_glesApiLevelPrefComboBox->setCurrentIndex(
                WINSYS_GLESAPILEVEL_PREFERENCE_AUTO);
        break;
    }

    const auto enableClipboardSharing =
            settings.value(Ui::Settings::CLIPBOARD_SHARING, true).toBool();
    mUi->set_clipboardSharing->setChecked(enableClipboardSharing);
    on_set_clipboardSharing_toggled(enableClipboardSharing);

    const auto disableMouseWheel =
        settings.value(Ui::Settings::DISABLE_MOUSE_WHEEL, false).toBool();
    mUi->set_disableMouseWheel->setChecked(disableMouseWheel);
    on_set_disableMouseWheel_toggled(disableMouseWheel);
}

SettingsPage::~SettingsPage() {
    proxyDtor();
}

void SettingsPage::setAdbInterface(android::emulation::AdbInterface* adb) {
    mAdb = adb;
}

void SettingsPage::setHaveClipboardSharing(bool haveSharing) {
    if (haveSharing) {
        // Make sure the saved value from settings reaches the clipboard agent.
        on_set_clipboardSharing_toggled(mUi->set_clipboardSharing->isChecked());
    } else {
        mUi->set_clipboardSharing->setEnabled(false);
        mUi->set_clipboardSharing->setChecked(false);
        mUi->set_clipboardSharingTitle->setEnabled(false);
    }
}

bool SettingsPage::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() != QEvent::FocusIn && event->type() != QEvent::FocusOut) {
        return false;
    }

    QSettings settings;

    if (object == mUi->set_saveLocBox) {
        QString savePath = settings.value(Ui::Settings::SAVE_PATH, "").toString();

        if (event->type() == QEvent::FocusIn) {
            mUi->set_saveLocBox->setText(savePath);
        } else if (event->type() == QEvent::FocusOut) {
            setElidedText(mUi->set_saveLocBox, savePath);
        }
    } else if (object == mUi->set_adbPathBox) {
        QString adbPath = settings.value(Ui::Settings::ADB_PATH, "").toString();

        if (event->type() == QEvent::FocusIn) {
            mUi->set_adbPathBox->setText(adbPath);
        } else if (event->type() == QEvent::FocusOut) {
            setElidedText(mUi->set_adbPathBox, adbPath);
        }
    }
    return false;
}

void SettingsPage::on_set_themeBox_currentIndexChanged(int index)
{
    // Select either the light or dark theme
    SettingsTheme theme = (SettingsTheme)index;

    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        // Out of range--ignore
        return;
    }
    QSettings settings;
    settings.setValue(Ui::Settings::UI_THEME, (int)theme);

    emit(themeChanged(theme));
}

void SettingsPage::on_set_saveLocFolderButton_clicked()
{
    QSettings settings;

    QString dirName = QFileDialog::getExistingDirectory(
            this, tr("Save location"),
            settings.value(Ui::Settings::SAVE_PATH, "").toString(),
            QFileDialog::ShowDirsOnly);

    if ( dirName.isEmpty() ) return; // Operation was canceled

    dirName = QDir::toNativeSeparators(dirName);

    if ( !directoryIsWritable(dirName) ) {
        QString errStr = tr("The path is not writable:<br>")
                         + dirName;
        showErrorDialog(errStr, tr("Save location"));
        return;
    }

    settings.setValue(Ui::Settings::SAVE_PATH, dirName);

    setElidedText(mUi->set_saveLocBox, dirName);
}

#ifndef SNAPSHOT_CONTROLS // TODO:jameskaye Remove this when Snapshot controls are fully enabled
void SettingsPage::on_set_saveSnapNowButton_clicked() {
    // Invoke the snapshot save function.
    // But don't run it on the UI thread.
    android::base::ThreadLooper::runOnMainLooper([]() {
        androidSnapshot_save(android::snapshot::kDefaultBootSnapshot);
    });
}

void SettingsPage::on_set_loadSnapNowButton_clicked() {
    // Invoke the snapshot load function.
    // But don't run it on the UI thread.
    android::base::ThreadLooper::runOnMainLooper([]() {
        androidSnapshot_load(android::snapshot::kDefaultBootSnapshot);
    });
}
#endif

void SettingsPage::on_set_adbPathButton_clicked() {
    QSettings settings;
    QString adbPath = settings.value(Ui::Settings::ADB_PATH, "").toString();

    // Repeat this dialog until the user is successful or cancels
    bool pathIsGood = false;
    do {
        adbPath = QFileDialog::getOpenFileName(this, tr("Backup ADB path"),
                                               adbPath);

        if (adbPath.isEmpty()) {
            break; // Operation was canceled
        }

        // We got a path. Make sure that the file both exists and is
        // executable.
        QFileInfo fileInfo(adbPath);
        pathIsGood = fileInfo.exists() && fileInfo.isExecutable() &&
                     fileInfo.fileName() ==
                             QString(android::base::PathUtils::toExecutableName(
                                             "adb")
                                             .c_str());

        if (pathIsGood) {
            // Save this selection
            settings.setValue(Ui::Settings::ADB_PATH, adbPath);

            adbPath = QDir::toNativeSeparators(adbPath);
            setElidedText(mUi->set_adbPathBox, adbPath);
            if (mAdb) {
                mAdb->setCustomAdbPath(adbPath.toStdString());
            }
        } else {
            // The path is not good. Force the user to cancel or try again.
            QString errStr = tr("This path does not point to "
                                "an ADB executable.<br><br>") +
                             QDir::toNativeSeparators(adbPath);
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Select backup ADB path"));
            msgBox.setText(errStr);
            msgBox.setInformativeText(tr("Do you want try again or cancel?"));
            msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int selection = msgBox.exec();
            if (selection == QMessageBox::Cancel) break;
        }
    } while ( !pathIsGood );
}

void SettingsPage::on_set_saveLocBox_textEdited(const QString&) {
    QSettings settings;
    mUi->set_saveLocBox->setText(
        settings.value(Ui::Settings::SAVE_PATH, "").toString());
}

void SettingsPage::on_set_adbPathBox_textEdited(const QString&) {
    QSettings settings;
    mUi->set_adbPathBox->setText(
            settings.value(Ui::Settings::ADB_PATH, "").toString());
}

void SettingsPage::on_set_forwardShortcutsToDevice_currentIndexChanged(int index) {
    QSettings settings;
    settings.setValue(Ui::Settings::FORWARD_SHORTCUTS_TO_DEVICE, (index != 0));
}

void SettingsPage::on_set_onTop_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::ALWAYS_ON_TOP, checked);

    emit(onTopChanged(checked));
}

void SettingsPage::on_set_frameAlways_toggled(bool checked) {
    QSettings settings;
    if (FramelessDetector::isFramelessOk()) {
        settings.setValue(Ui::Settings::FRAME_ALWAYS, checked);
        emit(frameAlwaysChanged(checked));
    }
}

void SettingsPage::on_set_autoFindAdb_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::AUTO_FIND_ADB, checked);

    if (mAdb) {
        if (checked) {
            mAdb->setCustomAdbPath(std::string());
        } else {
            mAdb->setCustomAdbPath(settings.value(Ui::Settings::ADB_PATH, "")
                                           .toString()
                                           .toStdString());
        }
    }

    mUi->set_adbPathBox->setHidden(checked);
    mUi->set_adbPathButton->setHidden(checked);
}

static void set_reportPref_to(Ui::Settings::CRASHREPORT_PREFERENCE_VALUE v) {
    QSettings settings;
    settings.setValue(Ui::Settings::CRASHREPORT_PREFERENCE, v);
}

void SettingsPage::on_set_crashReportPrefComboBox_currentIndexChanged(int index) {
    if (index == Ui::Settings::CRASHREPORT_COMBOBOX_ALWAYS) {
        set_reportPref_to(Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS);
    } else if (index == Ui::Settings::CRASHREPORT_COMBOBOX_NEVER) {
        set_reportPref_to(Ui::Settings::CRASHREPORT_PREFERENCE_NEVER);
    } else if (index == Ui::Settings::CRASHREPORT_COMBOBOX_ASK) {
        set_reportPref_to(Ui::Settings::CRASHREPORT_PREFERENCE_ASK);
    }
}

#ifndef SNAPSHOT_CONTROLS // TODO:jameskaye Remove this when Snapshot controls are fully enabled
void SettingsPage::on_set_saveSnapshotOnExit_currentIndexChanged(int uiIndex) {
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
    // Enable SAVE STATE NOW if we won't overwrite the state on exit
    mUi->set_saveSnapNowButton->setEnabled(preferenceValue != SaveSnapshotOnExit::Always);

    // Save for only this AVD
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        avdSpecificSettings.setValue(Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                                     static_cast<int>(preferenceValue));
    }
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
#endif

static void set_glesBackend_to(WinsysPreferredGlesBackend v) {
    QSettings settings;
    settings.setValue(Ui::Settings::GLESBACKEND_PREFERENCE, v);
}

static void set_glesApiLevel_to(WinsysPreferredGlesApiLevel v) {
    QSettings settings;
    settings.setValue(Ui::Settings::GLESAPILEVEL_PREFERENCE, v);
}

void SettingsPage::on_set_glesBackendPrefComboBox_currentIndexChanged(int index) {
    WinsysPreferredGlesBackend backendPreference =
        (WinsysPreferredGlesBackend)
        (mUi->set_glesBackendPrefComboBox->itemData(index).toInt());
    switch (backendPreference) {
    case WINSYS_GLESBACKEND_PREFERENCE_ANGLE:
    case WINSYS_GLESBACKEND_PREFERENCE_ANGLE9:
        if (mDisableANGLE) break;
    case WINSYS_GLESBACKEND_PREFERENCE_AUTO:
    case WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER:
    case WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL:
        set_glesBackend_to((WinsysPreferredGlesBackend)backendPreference);
        break;
    default:
        break;
    }
}

void SettingsPage::on_set_glesApiLevelPrefComboBox_currentIndexChanged(int index) {
    switch (index) {
    case WINSYS_GLESAPILEVEL_PREFERENCE_AUTO:
    case WINSYS_GLESAPILEVEL_PREFERENCE_MAX:
    case WINSYS_GLESAPILEVEL_PREFERENCE_COMPAT:
        set_glesApiLevel_to((WinsysPreferredGlesApiLevel)index);
        break;
    default:
        break;
    }
}

void SettingsPage::on_set_resetNotifications_pressed() {
    QSettings settings;
    settings.remove(Ui::Settings::SHOW_AVD_ARCH_WARNING);
    settings.remove(Ui::Settings::SHOW_GPU_WARNING);
    settings.remove(Ui::Settings::SHOW_ADB_WARNING);
    settings.remove(Ui::Settings::SHOW_VIRTUALSCENE_INFO);
}

void SettingsPage::on_perfstatsButton_pressed() {
    if (!mPerfStatsPage.get()) {
        const double densityFactor = devicePixelRatioF();
        QString styleString = Ui::fontStylesheet(densityFactor > 1.5);
        styleString += Ui::stylesheetForTheme(getSelectedTheme());
        mPerfStatsPage.reset(new PerfStatsPage(nullptr));
        mPerfStatsPage->setStyleSheet(styleString);
    }
    mPerfStatsPage->show();
    setFrameOnTop(mPerfStatsPage.get(), true);
}

void SettingsPage::on_set_clipboardSharing_toggled(bool checked) {
    // Save it only if the option is enabled - otherwise it means that the
    // feature isn't supported, so no need to overwrite valid saved value.
    if (mUi->set_clipboardSharing->isEnabled()) {
        QSettings settings;
        settings.setValue(Ui::Settings::CLIPBOARD_SHARING, checked);
    }

    emit enableClipboardSharingChanged(checked);
}

void SettingsPage::on_set_disableMouseWheel_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::DISABLE_MOUSE_WHEEL, checked);
    emit disableMouseWheelChanged(checked);
}
