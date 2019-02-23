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

#include "android/skin/qt/extended-pages/record-macro-page.h"

#include "android/automation/AutomationController.h"
#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/skin/qt/extended-pages/record-macro-saved-item.h"
#include "android/skin/qt/qt-settings.h"

#include <QMessageBox>
#include <iostream>
#include <sstream>

using namespace android::base;
using namespace android::automation;

RecordMacroPage::RecordMacroPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordMacroPage()) {
    mUi->setupUi(this);
    loadUi();
}

void RecordMacroPage::loadUi() {
    // Clear all items. Might need to optimize this and keep track of existing.
    mUi->macroList->clear();

    const std::string macrosPath = getMacrosDirectory();
    const std::vector<std::string> macroFileNames =
            System::get()->scanDirEntries(macrosPath);

    // For every macro, create a macroSavedItem with its name.
    for (const auto& macroName : macroFileNames) {
        RecordMacroSavedItem* macroSavedItem = new RecordMacroSavedItem();
        macroSavedItem->setName(macroName.c_str());
        macroSavedItem->setDisplayInfo("Preset macro");

        QListWidgetItem* listItem = new QListWidgetItem(mUi->macroList);
        listItem->setSizeHint(macroSavedItem->sizeHint());

        mUi->macroList->addItem(listItem);
        mUi->macroList->setItemWidget(listItem, macroSavedItem);
    }
}

void RecordMacroPage::on_playButton_clicked() {
    // Stop and reset automation.
    AutomationController::get().stopPlayback();

    // Get macro name from item widget.
    QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
    RecordMacroSavedItem* macroSavedItem = qobject_cast<RecordMacroSavedItem*>(
            mUi->macroList->itemWidget(listItem));
    const std::string macroName = macroSavedItem->getName();
    const std::string macroAbsolutePath =
            PathUtils::join(getMacrosDirectory(), macroName);

    auto result = AutomationController::get().startPlayback(macroAbsolutePath);
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("An error ocurred.");
        msgBox.setInformativeText(errString.str().c_str());
        msgBox.setDefaultButton(QMessageBox::Save);

        int ret = msgBox.exec();
    }
}

// TODO(dteran): We want to know the item that is clicked to eventually show a
// preview of the macro.
void RecordMacroPage::on_macroList_itemClicked(QListWidgetItem* item) {
    mUi->playButton->setEnabled(true);
}

std::string RecordMacroPage::getMacrosDirectory() {
    return PathUtils::join(System::get()->getLauncherDirectory(), "resources",
                           "macros");
}
