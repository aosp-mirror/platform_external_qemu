// Copyright (C) 2015 The Android Open Source Project
 
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include "android/skin/qt/qt-ui-commands.h"

#include <algorithm>
#include <utility>

bool parseQtUICommand(const QString& string, QtUICommand* command) {
    auto it = std::find_if(std::begin(NameToQtUICmd),
                           std::end(NameToQtUICmd),
                           [&string] (const std::pair<QString, QtUICommand>& value) {
                               return value.first == string;
                           });
    bool result = (it != std::end(NameToQtUICmd));
    if (result) {
        *command = it->second;
    }
    return result;
}


QString getQtUICommandDescription(QtUICommand command) {
    QString result;
    auto it = std::find_if(std::begin(QtUICmdToDesc),
                        std::end(QtUICmdToDesc),
                        [command](const std::pair<QtUICommand, QString>& value) {
                            return value.first == command;
                        });
    if (it != std::end(QtUICmdToDesc)) {
        result = it->second;
    }

    return result;
}
