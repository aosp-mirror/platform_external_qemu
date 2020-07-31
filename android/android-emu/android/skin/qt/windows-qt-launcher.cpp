/* Copyright (C) 2016 The Android Open Source Project
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
#include <windows.h>
#include <shellapi.h>
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/memory/ScopedPtr.h"

#include <vector>

using android::base::Win32UnicodeString;

extern "C" int qt_main(int, char**);

// For msvc build, qt calls main() instead of qMain (see
// qt-src/.../qtmain_win.cpp)
int main(int argc, char** argv) {
    // The arguments coming in here are encoded in whatever local code page
    // Windows is configured with but we need them to be UTF-8 encoded. So we
    // use GetCommandLineW and CommandLineToArgvW to get a UTF-16 encoded argv
    // which we then convert to UTF-8.
    //
    // According to the Qt documentation Qt itself doesn't really care about
    // these as it also uses GetCommandLineW on Windows so this shouldn't cause
    // problems for Qt. But the emulator uses argv[0] to determine the path of
    // the emulator executable so we need that to be encoded correctly.
    int numArgs = 0;
    const auto wideArgv = android::base::makeCustomScopedPtr(
            CommandLineToArgvW(GetCommandLineW(), &numArgs),
            [](wchar_t** ptr) { LocalFree(ptr); });
    if (!wideArgv) {
        // If this fails we can at least give it a try with the local code page
        // As long as there are only ANSI characters in the arguments this works
        return qt_main(argc, argv);
    }

    // Store converted strings and pointers to those strings, the pointers are
    // what will become the argv for qt_main.
    // Also reserve a slot for an array null-terminator as QEMU command line
    // parsing code relies on it (and it's a part of C Standard, actually).
    std::vector<std::string> arguments(numArgs);
    std::vector<char*> argumentPointers(numArgs + 1);

    for (int i = 0; i < numArgs; ++i) {
        arguments[i] = Win32UnicodeString::convertToUtf8(wideArgv.get()[i]);
        argumentPointers[i] = &arguments[i].front();
    }
    argumentPointers.back() = nullptr;

    return qt_main(numArgs, argumentPointers.data());
}