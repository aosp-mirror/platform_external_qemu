/* Copyright (C) 2022 The Android Open Source Project
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
#include <memory>
#include <functional>
#include "android/console.h"


using UIFunction = std::function<void(void)>;

void runOnEmuUiThread(UIFunction fn, bool waitUntilFinished) {
    struct UIFunctionHolder {
        UIFunction fn;
    };
    auto holder = new UIFunctionHolder({.fn = fn});
    getConsoleAgents()->emu->runOnUiThread(
            [](void* holder) {
                std::unique_ptr<UIFunctionHolder> marshalled(
                        reinterpret_cast<UIFunctionHolder*>(holder));
                marshalled->fn();
            },
            holder, waitUntilFinished);
}
