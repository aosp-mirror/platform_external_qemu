/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "ui_extended.h"

#include "android/settings-agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/qt-settings.h"
#include "extended-window.h"
#include "extended-window-styles.h"

#include <QFontMetrics>
#include <QSettings>
#include <QtWidgets>

// Helper function to set the contents of a QLineEdit.
static void setElidedText(QLineEdit* line_edit, const QString& text) {
    QFontMetrics font_metrics(line_edit->font());
    line_edit->setText(
            font_metrics.elidedText(text, Qt::ElideRight, line_edit->width() * 0.9));
}

void ExtendedWindow::initSettings()
{
    mExtendedUi->set_saveLocBox->installEventFilter(this);
}

void ExtendedWindow::completeSettingsInitialization()
{
    // Get the latest user selections from the
    // user-config code.

    // "Screen shot" and "Record screen" destination folder
    mSettingsState.mSavePath =
            QDir::toNativeSeparators(mToolWindow->getScreenshotSaveDirectory());

    if (mSettingsState.mSavePath.isEmpty()) {
        mExtendedUi->set_saveLocBox->setText(tr("None"));
    } else {
        setElidedText(mExtendedUi->set_saveLocBox, mSettingsState.mSavePath);
    }

    // Dark/Light theme
    QSettings settings;
    SettingsTheme theme = (SettingsTheme)settings.
                             value(Ui::Settings::UI_THEME, 0).toInt();
    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        theme = (SettingsTheme)0;
    }
    mSettingsState.mTheme = theme;
    // Set the theme to the initial selection
    // (by pretending the theme setting got changed)
    on_set_themeBox_currentIndexChanged(theme);

    mExtendedUi->set_allowKeyboardGrab->setChecked(
            settings.value(Ui::Settings::ALLOW_KEYBOARD_GRAB, false).toBool());
}

void ExtendedWindow::on_set_themeBox_currentIndexChanged(int index)
{
    // Select either the light or dark theme
    SettingsTheme theme = (SettingsTheme)index;

    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        // Out of range--ignore
        return;
    }

    // Set the UI widget state in case this function
    // was called artificially.
    mExtendedUi->set_themeBox->setCurrentIndex(index);

    // Save a valid selection
    mSettingsState.mTheme = theme;
    QSettings settings;
    settings.setValue(Ui::Settings::UI_THEME, (int)theme);

    // Switch to the icon images that are appropriate for this theme.
    switchAllIconsForTheme(theme);

    // Set the Qt style.

    // The first part is based on the display's pixel density.
    // Most displays give 1.0; high density displays give 2.0.
    double densityFactor = 1.0;
    if (skin_winsys_get_device_pixel_ratio(&densityFactor) != 0) {
        // Failed: use 1.0
        densityFactor = 1.0;
    }
    QString styleString = (densityFactor > 1.5) ? QT_FONTS(HI) : QT_FONTS(LO);


    // The second part is based on the theme
    // Set the style for this theme
    switch (theme) {
        case SETTINGS_THEME_DARK:
            styleString += QT_STYLE(DARK);
            break;
        case SETTINGS_THEME_LIGHT:
        default:
            styleString += QT_STYLE(LIGHT);
            break;
    }

    // Apply this style to the extended window (this),
    // and to the main tool-bar.
    this->setStyleSheet(styleString);
    mToolWindow->setStyleSheet(styleString);

    // Force a re-draw to make the new style take effect
    this->style()->unpolish(mExtendedUi->stackedWidget);
    this->style()->polish(mExtendedUi->stackedWidget);
    this->update();

    // Make the Settings pane active (still)
    adjustTabs(PANE_IDX_SETTINGS);
}

void ExtendedWindow::on_set_folderButton_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(
                                      this,
                                      tr("Save location"),
                                      mSettingsState.mSavePath,
                                      QFileDialog::ShowDirsOnly);

    if ( dirName.isEmpty() ) return; // Operation was canceled

    dirName = QDir::toNativeSeparators(dirName);

    // Check if this path is writable
    QFileInfo fInfo(dirName);
    if ( !fInfo.isDir() || !fInfo.isWritable() ) {
        QString errStr = tr("The path is not writable:<br>")
                         + dirName;
        mToolWindow->showErrorDialog(errStr, tr("Save location"));
        return;
    }

    // Everything looks OK
    mSettingsState.mSavePath = dirName;

    QSettings settings;
    settings.setValue(Ui::Settings::SAVE_PATH, dirName);

    setElidedText(mExtendedUi->set_saveLocBox, dirName);
}

// static member function
void ExtendedWindow::switchAllIconsForTheme(SettingsTheme theme)
{
    // Switch to the icon images that are appropriate for this theme.
    // Examine every widget.
    QWidgetList wList = QApplication::allWidgets();
    for (int idx = 0; idx < wList.size(); idx++) {
        QPushButton *pB = dynamic_cast<QPushButton*>(wList[idx]);
        if (pB && !pB->icon().isNull()) {
            setButtonEnabled(pB, theme, pB->isEnabled());
        }
    }
}

// static member function
void ExtendedWindow::setButtonEnabled(QPushButton*  theButton,
                                      SettingsTheme theme,
                                      bool          isEnabled)
{
    theButton->setEnabled(isEnabled);
    // If this button has icon image properties, reset its
    // image.
    QString enabledPropStr  = theButton->property("themeIconName"         ).toString();
    QString disabledPropStr = theButton->property("themeIconName_disabled").toString();

    // Get the resource name based on the light/dark theme
    QString resName = ":/";
    resName += (theme == SETTINGS_THEME_DARK) ? DARK_PATH : LIGHT_PATH;
    resName += "/";

    if (!isEnabled  &&  !disabledPropStr.isNull()) {
        // The button is disabled and has a specific
        // icon for when it is disabled.
        // Apply this icon and tell Qt to use it
        // directly (don't gray it out) even though
        // it is disabled.
        resName += disabledPropStr;
        QIcon theIcon(resName);
        theIcon.addPixmap(QPixmap(resName), QIcon::Disabled);
        theButton->setIcon(theIcon);
    } else if ( !enabledPropStr.isNull() ) {
        // Use the 'enabled' version of the icon
        // (Let Qt grey the icon if it wants to)
        resName += enabledPropStr;
        QIcon theIcon(resName);
        theButton->setIcon(theIcon);
    }
}


void ExtendedWindow::on_set_saveLocBox_textEdited(const QString&) {
    mExtendedUi->set_saveLocBox->setText(mSettingsState.mSavePath);
}

bool ExtendedWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::FocusIn && object == mExtendedUi->set_saveLocBox) {
        mExtendedUi->set_saveLocBox->setText(mSettingsState.mSavePath);
    } else if (event->type() == QEvent::FocusOut && object == mExtendedUi->set_saveLocBox) {
        setElidedText(mExtendedUi->set_saveLocBox, mSettingsState.mSavePath);
    }
    return false;
}

void ExtendedWindow::on_set_allowKeyboardGrab_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::ALLOW_KEYBOARD_GRAB, checked);
}
