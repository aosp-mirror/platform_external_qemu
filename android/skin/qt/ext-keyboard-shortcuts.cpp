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

#include "android/skin/qt/extended-window.h"

#include "android/emulation/control/window_agent.h"
#include "android/skin/keyset.h"
#include "android/skin/ui.h"
#include "ui_extended.h"

static void addShortcutsTableRow(QTableWidget* table_widget,
                                 const QString& key_sequence,
                                 const QString& description) {
    int table_row = table_widget->rowCount();
    table_widget->insertRow(table_row);
    table_widget->setItem(table_row, 0, new QTableWidgetItem(description));
    table_widget->setItem(table_row, 1, new QTableWidgetItem(key_sequence));
}

void ExtendedWindow::initKbdShorts() {
    QTableWidget* table_widget = mExtendedUi->shortcutsTableWidget;
    if (mQtUIShortcuts) {
        for (auto key_sequence_and_command = mQtUIShortcuts->begin();
             key_sequence_and_command != mQtUIShortcuts->end();
             ++key_sequence_and_command) {
            addShortcutsTableRow(table_widget,
                                 key_sequence_and_command.key().toString(QKeySequence::NativeText),
                                 getQtUICommandDescription(key_sequence_and_command.value()));
        }
    }

    table_widget->sortItems(0);
}

