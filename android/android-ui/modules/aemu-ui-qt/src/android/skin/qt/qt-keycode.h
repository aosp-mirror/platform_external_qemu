/* Copyright (C) 2024 The Android Open Source Project
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

#pragma once

#include <QKeyEvent>
#include <optional>

namespace android {
namespace qt {

// QKeyEvent::key() will give back the Qt::Key that is already modified (e.g. pressing Shift + 2
// gives Qt::Key_At + Qt::Key_Shift instead of Qt::Key_2 + Qt::Key_Shift in US QWERTY layout). So
// `getUnmodifiedQtKey()` will try to extract the unmodified Qt::Key, or return std::nullopt if
// unable to.
std::optional<int> getUnmodifiedQtKey(const QKeyEvent& e);

}  // namespace qt
}  // namespace android