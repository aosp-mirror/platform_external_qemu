// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef QEMU_ANDROID_BASE_VERSION_H
#define QEMU_ANDROID_BASE_VERSION_H

#include "android/base/String.h"

namespace android {
namespace base {

class Version {
public:
    explicit Version(const char* ver);
    Version(unsigned int major, unsigned int minor, unsigned int micro);

    bool operator<(const Version& other) const;

    static const Version Invalid;

private:
    unsigned int mMajor;
    unsigned int mMinor;
    unsigned int mMicro;
};

}  // namespace android
}  // namespace base

#endif //QEMU_ANDROID_BASE_VERSION_H
