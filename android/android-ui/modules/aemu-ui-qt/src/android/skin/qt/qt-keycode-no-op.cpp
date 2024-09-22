/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "android/skin/qt/qt-keycode.h"

namespace android {
namespace qt {

// QKeyEvent::key() will give back the Qt::Key that is already modified (e.g. pressing Shift + 2
// gives Qt::Key_At + Qt::Key_Shift instead of Qt::Key_2 + Qt::Key_Shift in US QWERTY layout). So
// `getUnmodifiedQtKey()` will try to extract the unmodified Qt::Key, or return std::nullopt if
// unable to.
std::optional<int> getUnmodifiedQtKey(const QKeyEvent& e) {
    // TODO(joshuaduong): mac has an implementation. Do for linux/windows and get rid of this file.
    return e.key();
}

}  // namespace qt
}  // namespace android