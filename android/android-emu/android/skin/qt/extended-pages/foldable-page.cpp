// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// ********************************************************************************
// Implementation note:
//
// This page has three controls:
//    A toggle "Foldable is open"
//    Radio buttons "Device configuration" (Open, Folded tall, Folded wide)
//    Radio buttons "Pixel density" (Normal, Less dense, More dense)
//
// The plan is that the toggle is the only control. At startup this code will
// send the "fold-area" command. After that, this code will only send the
// open/closed state to trigger the unfolded/folded behavior.
//
// As of 4 Feb 2019, the "fold-area" ADB command has not been implemented in
// the System Image, so the two radio buttons exist to see some behavior change
// while we wait. These controls should be removed when the "fold-area" feature
// is available.
//
// ********************************************************************************

#include "android/skin/qt/extended-pages/foldable-page.h"

#include "android/base/memory/LazyInstance.h"
#include "android/hw-events.h"
#include "android/skin/qt/qt-settings.h"

#include "android/globals.h"

using android::emulation::AdbInterface;

struct FoldableStatics {
    AdbInterface* adbInterface = nullptr;

    bool lidIsOpen = true;
    int  configIdx = 0;
    int  densityIdx = 0;
};

static android::base::LazyInstance<FoldableStatics> sStatics = LAZY_INSTANCE_INIT;

static void forwardGenericEventToEmulator(int type, int code, int value);
static void restoreSettings();
static void saveConfigIndex(int idx);
static void saveDensityIndex(int idx);
static void saveLidIsOpen(bool isOpen);
static void sendDisplayDensityByIndex();
static void sendDisplaySizeByIndex();
static void sendLidIsOpenState();

FoldablePage::FoldablePage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::FoldablePage())
{
    mUi->setupUi(this);

    mUi->isOpen->setChecked(sStatics->lidIsOpen);

    // Assign explicit IDs to the buttons. This allows
    // us to work with them generically.
    mUi->configurationButtonGroup->setId(mUi->config_0, 0);
    mUi->configurationButtonGroup->setId(mUi->config_1, 1);
    mUi->configurationButtonGroup->setId(mUi->config_2, 2);
    mUi->configurationButtonGroup->setId(mUi->config_3, 3);
    mUi->configurationButtonGroup->setId(mUi->config_4, 4);
    mUi->configurationButtonGroup->setId(mUi->config_5, 5);
    mUi->configurationButtonGroup->setId(mUi->config_6, 6);
    mUi->configurationButtonGroup->setId(mUi->config_7, 7);
    mUi->densityButtonGroup->setId(mUi->density_0, 0);
    mUi->densityButtonGroup->setId(mUi->density_1, 1);
    mUi->densityButtonGroup->setId(mUi->density_2, 2);
    mUi->densityButtonGroup->setId(mUi->density_3, 3);
    mUi->densityButtonGroup->setId(mUi->density_4, 4);
    mUi->densityButtonGroup->setId(mUi->density_5, 5);
    mUi->densityButtonGroup->setId(mUi->density_6, 6);
    mUi->densityButtonGroup->setId(mUi->density_7, 7);

    // TODO: Set the configuration strings based on the config.ini
    mUi->config_0->setText(tr("Open"));
    mUi->config_1->setText(tr("Folded tall"));
    mUi->config_2->setText(tr("Folded wide"));
    int numActiveConfigButtons = 3;

    for (QAbstractButton* configButton : mUi->configurationButtonGroup->buttons()) {
        int idx = mUi->configurationButtonGroup->id(configButton);
        if (idx < numActiveConfigButtons) {
            configButton->setChecked(sStatics->configIdx == idx);
        } else {
            configButton->hide();
        }
    }

    // TODO: Set the density strings based on the config.ini
    mUi->density_0->setText(tr("Normal"));
    mUi->density_1->setText(tr("Less dense"));
    mUi->density_2->setText(tr("More dense"));
    int numActiveDensityButtons = 3;

    for (QAbstractButton* densityButton : mUi->densityButtonGroup->buttons()) {
        int idx = mUi->densityButtonGroup->id(densityButton);
        if (idx < numActiveDensityButtons) {
            densityButton->setChecked(sStatics->densityIdx == idx);
        } else {
            densityButton->hide();
        }
    }
}

// static
void FoldablePage::earlyInitialization() {

    restoreSettings();

    // If this device is foldable, tell the system about
    // the display to use when the device is closed.
    int xOffset = android_hw->hw_displayRegion_0_1_xOffset;
    int yOffset = android_hw->hw_displayRegion_0_1_yOffset;
    int width   = android_hw->hw_displayRegion_0_1_width;
    int height  = android_hw->hw_displayRegion_0_1_height;

    if (xOffset < 0 || xOffset > 9999 ||
        yOffset < 0 || yOffset > 9999 ||
        width   < 1 || width   > 9999 ||
        height  < 1 || height  > 9999   )
    {
        printf("foldable-page.cpp: earlyInitialization() No display region is defined\n"); // ??
        return;
    }
    // A display region is defined for this device.
    // Send it to the device as the area that is active when
    // the device is folded.
    EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
    if (emuQtWindow == nullptr) return;

    sStatics->adbInterface = emuQtWindow->getAdbInterface();
    if (sStatics->adbInterface == nullptr) return;

#if 0 // ?? This part is not defined yet
    int density = android_hw->hw_displayRegion_0_1_density;

    if (density < 1 || density > 9999) return;
    // We have a density for the folded case.
    char densityStr[8];
    sprintf(densityStr, "%d", density);

    printf("foldable-page: earlyInitialization() Density %s\n", densityStr); // ??

    sStatics->adbInterface->enqueueCommand({ "shell", "wm", "fold-density", densityStr });
#endif // ??

    char xOffsetStr[8];
    char yOffsetStr[8];
    char widthStr[8];
    char heightStr[8];

    sprintf(xOffsetStr, "%d", xOffset);
    sprintf(yOffsetStr, "%d", yOffset);
    sprintf(widthStr,   "%d", width);
    sprintf(heightStr,  "%d", height);

    sStatics->adbInterface->enqueueCommand(
            { "shell", "wm", "fold-area",
              xOffsetStr, yOffsetStr,
              widthStr, heightStr },
            [](const android::emulation::OptionalAdbCommandResult& result) {
                if (result && result->exit_code == 0) {
                    printf("foldable-page: 'fold-area' command succeeded\n"); // ??
                    // The "fold-area" command has been successfully
                    // received by the system image. We can now set
                    // the lid as open or closed.
                    sendLidIsOpenState();
                }
                else printf("foldable-page: 'fold-area' command failed\n"); // ??
            });
}

void FoldablePage::on_isOpen_toggled(bool checked) {
    saveLidIsOpen(checked);
    sendLidIsOpenState();
}

void FoldablePage::on_configurationButtonGroup_buttonClicked(int idx) {
    saveConfigIndex(idx);
    sendDisplaySizeByIndex();
}

void FoldablePage::on_densityButtonGroup_buttonClicked(int idx) {
    saveDensityIndex(idx);
    sendDisplayDensityByIndex();
}

static void sendDisplaySizeByIndex() {

    if (!sStatics->adbInterface) return;

    // TODO: Use the dimensions from config.ini
    int width  = android_hw->hw_lcd_width;
    int height = android_hw->hw_lcd_height;

    switch (sStatics->configIdx) {
        default:
        case 0:
            // No adjustment
            break;
        case 1:
            width /= 2;
            break;
        case 2:
            height = (height * 3) / 4;
            break;
    }
    if (width <=     0  ||  height <=     0  ||
        width >= 10000  ||  height >  10000    )
    {
        return;
    }

    char sizeString[16];
    sprintf(sizeString, "%dx%d", width, height);

    printf("f-p: Sending size %s\n", sizeString); // ??
    sStatics->adbInterface->enqueueCommand({ "shell", "wm", "size", sizeString});
}

static void sendDisplayDensityByIndex() {
    if (sStatics->adbInterface == nullptr) return;

    int density  = android_hw->hw_lcd_density;

    // TODO: Use the densities from config.ini
    switch (sStatics->densityIdx) {
        default:
        case 0:
            // No adjustment
            break;
        case 1:
            // less
            density = (density * 3) / 4;
            break;
        case 2:
            // More
            density = (density * 3) / 2;
            break;
    }
    if (density <  100) density =  100;
    if (density > 1000) density = 1000;

    char densityString[16];
    sprintf(densityString, "%d", density);
    printf("f-p: Sending density %s\n", densityString); // ??

    sStatics->adbInterface->enqueueCommand({ "shell", "wm", "density", densityString});
}

static void restoreSettings() {
    bool lidIsOpen = true;
    int  configIdx = 0;
    int  densityIdx = 0;
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        lidIsOpen = avdSpecificSettings.value(
                Ui::Settings::PER_AVD_FOLDABLE_LID_IS_OPEN, true).toBool();
        configIdx = avdSpecificSettings.value(
                Ui::Settings::PER_AVD_FOLDABLE_CONFIGURATION_INDEX, 0).toInt();
        densityIdx = avdSpecificSettings.value(
                Ui::Settings::PER_AVD_FOLDABLE_DENSITY_INDEX, 0).toInt();
    }
    sStatics->lidIsOpen = lidIsOpen;
    sStatics->configIdx = configIdx;
    sStatics->densityIdx = densityIdx;
}

static void saveLidIsOpen(bool isOpen) {
    sStatics->lidIsOpen = isOpen;
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_FOLDABLE_LID_IS_OPEN, isOpen);
    }
}

static void saveConfigIndex(int idx) {
    sStatics->configIdx = idx;
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_FOLDABLE_CONFIGURATION_INDEX, idx);
    }
}

static void saveDensityIndex(int idx) {
    sStatics->densityIdx = idx;
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_FOLDABLE_DENSITY_INDEX, idx);
    }
}

static void sendLidIsOpenState() {
    printf("f-p: Sending lid %s\n", sStatics->lidIsOpen ? "OPEN" : "CLOSED"); // ??
    // Event SW_LID = 1 means shut, so invert 'lidIsOpen'
    forwardGenericEventToEmulator(EV_SW, SW_LID, !sStatics->lidIsOpen);
    forwardGenericEventToEmulator(EV_SYN, 0, 0);
}

static void forwardGenericEventToEmulator(int type, int code, int value) {
    EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
    if (emuQtWindow == nullptr) return;

    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = kEventGeneric;
    SkinEventGenericData& genericData = skin_event->u.generic_event;
    genericData.type = type;
    genericData.code = code;
    genericData.value = value;

    emuQtWindow->queueSkinEvent(skin_event);
}
