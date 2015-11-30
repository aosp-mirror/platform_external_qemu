/* Copyright (C) 2007-2009 The Android Open Source Project
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

#include "android/utils/path.h"

#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/String.h"
#include "android/base/system/System.h"

using android::base::PathUtils;
using android::base::String;
using android::base::StringVector;
using android::base::System;

ABool path_exists(const char* path) {
    return System::get()->pathExists(path);
}

ABool path_is_regular(const char* path) {
    return System::get()->pathIsFile(path);
}

ABool path_is_dir(const char*  path) {
    return System::get()->pathIsDir(path);
}

ABool path_can_read(const char*  path) {
    return System::get()->pathCanRead(path);
}

ABool path_can_write(const char*  path) {
    return System::get()->pathCanWrite(path);
}

ABool path_can_exec(const char* path) {
    return System::get()->pathCanExec(path);
}

ABool path_is_absolute(const char* path) {
    return PathUtils::isAbsolute(path);
}

char* path_get_absolute(const char* path) {
    if (path_is_absolute(path)) {
        return ASTRDUP(path);
    }

    String currentDir = System::get()->getCurrentDirectory();
    StringVector currentItems = PathUtils::decompose(currentDir.c_str());
    StringVector pathItems = PathUtils::decompose(path);
    for (const auto& item : pathItems) {
        currentItems.push_back(item);
    }
    return PathUtils::recompose(currentItems).release();
}

int path_split(const char* path, char** dirname, char** basename) {
    String dir, file;
    if (!PathUtils::split(path, &dir, &file)) {
        return -1;
    }
    if (dirname) {
        *dirname = dir.release();
    }
    if (basename) {
        *basename = file.release();
    }
    return 0;
}

char* path_dirname(const char* path) {
    String dir;
    if (!PathUtils::split(path, &dir, nullptr)) {
        return nullptr;
    }
    return dir.release();
}

char* path_basename(const char* path) {
    String file;
    if (!PathUtils::split(path, nullptr, &file)) {
        return nullptr;
    }
    return file.release();
}
