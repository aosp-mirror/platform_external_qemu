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
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

// Helper function to set the contents of a QLineEdit.
static void setElidedText(QLineEdit* line_edit, const QString& text) {
    QFontMetrics font_metrics(line_edit->font());
    line_edit->setText(
            font_metrics.elidedText(text, Qt::ElideRight, line_edit->width() * 0.9));
}

SettingsPage::SettingsPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::SettingsPage())
{
    mUi->setupUi(this);
    mUi->set_saveLocBox->installEventFilter(this);
    mUi->set_sdkPathBox->installEventFilter(this);

    QString savePath =
        QDir::toNativeSeparators(getScreenshotSaveDirectory());

    if (savePath.isEmpty()) {
        mUi->set_saveLocBox->setText(tr("None"));
    } else {
        setElidedText(mUi->set_saveLocBox, savePath);
    }

    // SDK path
    QSettings settings;
    QString sdkPath = settings.value(Ui::Settings::SDK_PATH, "").toString();
    sdkPath = QDir::toNativeSeparators(sdkPath);
    setElidedText(mUi->set_sdkPathBox, sdkPath);

    // Dark/Light theme
    SettingsTheme theme =
        static_cast<SettingsTheme>(settings.value(Ui::Settings::UI_THEME, 0).toInt());
    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        theme = static_cast<SettingsTheme>(0);
    }
    mUi->set_themeBox->setCurrentIndex(static_cast<int>(theme));

    mUi->set_allowKeyboardGrab->setChecked(
            settings.value(Ui::Settings::ALLOW_KEYBOARD_GRAB, false).toBool());

#ifdef __linux__
    // "Always on top" is not supported for Linux (see emulator-qt-window.cpp)
    // Make the control invisible
    mUi->set_onTopTitle->hide();
    mUi->set_onTop->hide();
#else // Windows or OSX
    bool onTopOnly = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    mUi->set_onTop->setCheckState(onTopOnly ? Qt::Checked : Qt::Unchecked);
#endif
}

bool SettingsPage::eventFilter (QObject* object, QEvent* event)
{
    QSettings settings;
    QString savePath = settings.value(Ui::Settings::SAVE_PATH, "").toString();
    QString sdkPath = settings.value(Ui::Settings::SDK_PATH, "").toString();

    if (object == mUi->set_saveLocBox) {
        if (event->type() == QEvent::FocusIn) {
            mUi->set_saveLocBox->setText(savePath);
        } else if (event->type() == QEvent::FocusOut) {
            setElidedText(mUi->set_saveLocBox, savePath);
        }
    } else if (object == mUi->set_sdkPathBox) {
        if (event->type() == QEvent::FocusIn) {
            mUi->set_sdkPathBox->setText(sdkPath);
        } else if (event->type() == QEvent::FocusOut) {
            setElidedText(mUi->set_sdkPathBox, sdkPath);
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
        this,
        tr("Save location"),
        settings.value(Ui::Settings::SDK_PATH, "").toString(),
        QFileDialog::ShowDirsOnly);

    if ( dirName.isEmpty() ) return; // Operation was canceled

    dirName = QDir::toNativeSeparators(dirName);

    // Check if this path is writable
    QFileInfo fInfo(dirName);
    if ( !fInfo.isDir() || !fInfo.isWritable() ) {
        QString errStr = tr("The path is not writable:<br>")
                         + dirName;
        showErrorDialog(errStr, tr("Save location"));
        return;
    }

    settings.setValue(Ui::Settings::SAVE_PATH, dirName);

    setElidedText(mUi->set_saveLocBox, dirName);
}

void SettingsPage::on_set_sdkPathButton_clicked()
{
    QSettings settings;
    QString dirName = settings.value(Ui::Settings::SDK_PATH, "").toString();

    // Repeat this dialog until the user is successful or cancels
    bool pathIsGood = false;
    do {
        dirName = QFileDialog::getExistingDirectory(
                                  this,
                                  tr("Android SDK location"),
                                  dirName,
                                  QFileDialog::ShowDirsOnly);

        if ( dirName.isEmpty() ) {
            break; // Operation was canceled
        }

        // We got a path. If it does not have an SDK, don't allow it.
        // (We simply test for the existence of "platforms/"; see
        // validateSdkPath() in //tools/adt/idea/android/src/com/
        // android/tools/idea/sdk/SdkPaths.java)
        QString platformsName = dirName + "/platforms";
        QFileInfo platformsInfo(platformsName);
        pathIsGood = platformsInfo.exists() && platformsInfo.isDir();

        if (pathIsGood) {
            // Save this selection
            settings.setValue(Ui::Settings::SDK_PATH, dirName);

            dirName = QDir::toNativeSeparators(dirName);
            setElidedText(mUi->set_sdkPathBox, dirName);
        } else {
            // The path is not good. Force the user to cancel or try again.
            QString errStr = tr("This path does not point to "
                                "an SDK installation<br><br>")
                             + QDir::toNativeSeparators(dirName);
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Select Android SDK path"));
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

void SettingsPage::on_set_sdkPathBox_textEdited(const QString&) {
    QSettings settings;
    mUi->set_sdkPathBox->setText(
        settings.value(Ui::Settings::SDK_PATH, "").toString());
}

void SettingsPage::on_set_allowKeyboardGrab_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::ALLOW_KEYBOARD_GRAB, checked);
}

void SettingsPage::on_set_onTop_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::ALWAYS_ON_TOP, checked);

    emit(onTopChanged(checked));
}
